#include "scene/GameScene.h"
#include "manager/InputManager.h"
#include "DxLib.h"

#include <cstdio>

namespace {
    Disc opponentOf(Disc disc) {
        return disc == Disc::Black ? Disc::White : Disc::Black;
    }

    const char* discName(Disc disc) {
        if (disc == Disc::Black) return "Black";
        if (disc == Disc::White) return "White";
        return "";
    }
}

GameScene::~GameScene() {
    cancelAiSearch();
}

void GameScene::Start() {
    end_ = false;
    next_ = SceneID::Game;
    board_.reset();
    boardView_.Start();
    boardView_.Reset(board_);
    turn_ = Disc::Black;
    passed_ = Disc::Empty;
    gameOver_ = false;
    mouseLeftDown_ = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;
    aiSearchedNodes_ = 0;
    aiSearchDepth_ = 0;
    aiExactSearch_ = false;
    aiTranspositionHits_ = 0;
    aiIterations_ = 0;
    phase_ = Phase::PlayerTurn;
    aiFinished_.store(false, std::memory_order_relaxed);
    pendingAiMove_.reset();
}

void GameScene::End() {
    cancelAiSearch();
    boardView_.End();
}

void GameScene::Update() {
    auto& input = InputManager::GetInstance();
    boardView_.Update();

    if (input.isPressed(KEY_INPUT_ESCAPE)) {
        cancelAiSearch();
        next_ = SceneID::Title;
        end_ = true;
        return;
    }

    const bool mouseLeft = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;
    const bool clicked = mouseLeft && !mouseLeftDown_;
    mouseLeftDown_ = mouseLeft;

    if (phase_ == Phase::AiThinking) {
        if (aiFinished_.load(std::memory_order_acquire)) finishAiSearch();
        return;
    }

    // A flip is part of applying the previous move. Do not accept another move
    // or start the AI until all three transition frames have been displayed.
    if (boardView_.IsAnimating()) return;

    if (gameOver_) return;

    if (turn_ == Disc::White) {
        startAiSearch();
        return;
    }

    if (clicked) {
        handleBoardClick();
    }
}

void GameScene::handleBoardClick() {
    int mouseX = 0;
    int mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);

    int row = 0;
    int col = 0;
    if (!boardView_.HitTest(mouseX, mouseY, row, col)) return;

    const BitBoard before = board_;
    if (board_.put(Disc::Black, row, col)) {
        boardView_.BeginMove(before, board_, Disc::Black, row, col);
        advanceTurn();
    }
}

void GameScene::performAiMove() {
    std::optional<OthelloAI::Move> move;
    {
        std::lock_guard lock(aiResultMutex_);
        move = std::move(pendingAiMove_);
        pendingAiMove_.reset();
    }
    if (!move.has_value()) {
        advanceTurn();
        return;
    }

    const BitBoard before = board_;
    if (board_.put(Disc::White, move->row, move->col)) {
        boardView_.BeginMove(
            before,
            board_,
            Disc::White,
            move->row,
            move->col
        );
        aiSearchedNodes_ = move->searchedNodes;
        aiSearchDepth_ = move->searchDepth;
        aiExactSearch_ = move->exactSearch;
        aiTranspositionHits_ = move->transpositionHits;
        aiIterations_ = move->completedIterations;
        advanceTurn();
    }
}

void GameScene::startAiSearch() {
    if (phase_ == Phase::AiThinking) return;

    cancelAiSearch();
    const BitBoard boardSnapshot = board_;
    aiProgress_.reset(0, false);
    aiFinished_.store(false, std::memory_order_relaxed);
    aiStartedAt_ = std::chrono::steady_clock::now();
    phase_ = Phase::AiThinking;

    aiThread_ = std::jthread(
        [this, boardSnapshot](std::stop_token stopToken) {
            auto result = ai_.chooseMove(
                boardSnapshot, Disc::White, stopToken, &aiProgress_
            );
            if (!stopToken.stop_requested()) {
                std::lock_guard lock(aiResultMutex_);
                pendingAiMove_ = std::move(result);
            }
            aiFinished_.store(true, std::memory_order_release);
        }
    );
}

void GameScene::finishAiSearch() {
    if (aiThread_.joinable()) aiThread_.join();
    phase_ = Phase::ApplyingAiMove;
    performAiMove();
    if (!gameOver_) {
        phase_ = turn_ == Disc::White
            ? Phase::ApplyingAiMove
            : Phase::PlayerTurn;
    }
}

void GameScene::cancelAiSearch() {
    if (aiThread_.joinable()) {
        aiThread_.request_stop();
        aiThread_.join();
    }
    aiFinished_.store(false, std::memory_order_relaxed);
    std::lock_guard lock(aiResultMutex_);
    pendingAiMove_.reset();
}

void GameScene::advanceTurn() {
    const Disc next = opponentOf(turn_);
    passed_ = Disc::Empty;

    if (board_.hasAnyMove(next)) {
        turn_ = next;
        return;
    }

    if (board_.hasAnyMove(turn_)) {
        passed_ = next;
        return;
    }

    gameOver_ = true;
    phase_ = Phase::GameOver;
}

void GameScene::Draw() {
    const int whiteColor = GetColor(255, 255, 255);
    const int noticeColor = GetColor(255, 220, 80);

    const BitBoard::Bits legalMoves =
        !gameOver_ && turn_ == Disc::Black
        ? board_.legalMoves(Disc::Black)
        : 0;

    boardView_.Draw(
        board_,
        legalMoves,
        !boardView_.IsAnimating() && phase_ == Phase::PlayerTurn
    );

    const int blackCount = board_.count(Disc::Black);
    const int whiteCount = board_.count(Disc::White);

    char score[64];
    std::snprintf(score, sizeof(score), "You (Black): %d  AI (White): %d", blackCount, whiteCount);

    char status[64];
    if (gameOver_) {
        if (blackCount > whiteCount) {
            std::snprintf(status, sizeof(status), "Game Over: You win");
        } else if (whiteCount > blackCount) {
            std::snprintf(status, sizeof(status), "Game Over: AI wins");
        } else {
            std::snprintf(status, sizeof(status), "Game Over: Draw");
        }
    } else if (phase_ == Phase::AiThinking) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - aiStartedAt_
        ).count();
        const int dots = static_cast<int>((elapsed / 300) % 4);
        std::snprintf(status, sizeof(status), "AI is thinking%.*s", dots, "...");
    } else if (boardView_.IsAnimating()) {
        std::snprintf(status, sizeof(status), "Flipping discs...");
    } else {
        std::snprintf(status, sizeof(status), "Your turn");
    }

    char aiInfo[128];
    const bool thinking = phase_ == Phase::AiThinking;
    const int displayDepth = thinking
        ? aiProgress_.completedDepth.load(std::memory_order_relaxed)
        : aiSearchDepth_;
    const int targetDepth = thinking
        ? aiProgress_.targetDepth.load(std::memory_order_relaxed)
        : aiSearchDepth_;
    const auto displayNodes = thinking
        ? aiProgress_.searchedNodes.load(std::memory_order_relaxed)
        : aiSearchedNodes_;
    const auto displayHits = thinking
        ? aiProgress_.transpositionHits.load(std::memory_order_relaxed)
        : aiTranspositionHits_;
    const bool displayExact = thinking
        ? aiProgress_.exactSearch.load(std::memory_order_relaxed)
        : aiExactSearch_;
    std::snprintf(
        aiInfo,
        sizeof(aiInfo),
        "AI depth: %d/%d  Nodes: %llu  TT: %llu%s",
        displayDepth,
        targetDepth > 0 ? targetDepth : ai_.depth(),
        static_cast<unsigned long long>(displayNodes),
        static_cast<unsigned long long>(displayHits),
        displayExact ? "  EXACT" : ""
    );

    DrawString(32, 32, "Othello vs AI", whiteColor);
    DrawString(32, 56, "Click a legal move / ESC : Back to Title", whiteColor);
    DrawString(32, 80, score, whiteColor);
    DrawString(32, 104, status, gameOver_ ? noticeColor : whiteColor);
    DrawString(32, 128, aiInfo, whiteColor);

    if (passed_ != Disc::Empty) {
        char passMessage[64];
        std::snprintf(passMessage, sizeof(passMessage), "%s passes", discName(passed_));
        DrawString(32, 152, passMessage, noticeColor);
    }
}

bool GameScene::IsEnd() const {
    return end_;
}

SceneID GameScene::NextScene() const {
    return next_;
}
