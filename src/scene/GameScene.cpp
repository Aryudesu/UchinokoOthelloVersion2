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
}

void GameScene::Update() {
    auto& input = InputManager::GetInstance();

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

    const int boardRight = BoardLeft + CellSize * BoardSize;
    const int boardBottom = BoardTop + CellSize * BoardSize;
    if (mouseX < BoardLeft || boardRight <= mouseX ||
        mouseY < BoardTop || boardBottom <= mouseY) {
        return;
    }

    const int col = (mouseX - BoardLeft) / CellSize;
    const int row = (mouseY - BoardTop) / CellSize;

    if (board_.put(Disc::Black, row, col)) {
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

    if (board_.put(Disc::White, move->row, move->col)) {
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
        if (turn_ == Disc::White) startAiSearch();
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
    const int boardRight = BoardLeft + CellSize * BoardSize;
    const int boardBottom = BoardTop + CellSize * BoardSize;
    const int boardColor = GetColor(0, 120, 0);
    const int lineColor = GetColor(0, 0, 0);
    const int blackColor = GetColor(0, 0, 0);
    const int whiteColor = GetColor(255, 255, 255);
    const int legalMoveColor = GetColor(160, 220, 160);
    const int noticeColor = GetColor(255, 220, 80);

    DrawBox(BoardLeft, BoardTop, boardRight, boardBottom, boardColor, TRUE);

    for (int i = 0; i <= BoardSize; ++i) {
        const int x = BoardLeft + i * CellSize;
        const int y = BoardTop + i * CellSize;
        DrawLine(x, BoardTop, x, boardBottom, lineColor);
        DrawLine(BoardLeft, y, boardRight, y, lineColor);
    }

    const BitBoard::Bits legalMoves =
        !gameOver_ && turn_ == Disc::Black
        ? board_.legalMoves(Disc::Black)
        : 0;

    for (int row = 0; row < BoardSize; ++row) {
        for (int col = 0; col < BoardSize; ++col) {
            const float centerX = BoardLeft + (col + 0.5f) * CellSize;
            const float centerY = BoardTop + (row + 0.5f) * CellSize;
            const Disc disc = board_.discAt(row, col);

            if (disc == Disc::Black) {
                DrawCircleAA(centerX, centerY, CellSize * 0.4f, 32, blackColor, TRUE);
            } else if (disc == Disc::White) {
                DrawCircleAA(centerX, centerY, CellSize * 0.4f, 32, whiteColor, TRUE);
            } else if ((legalMoves & BitBoard::bitAt(row, col)) != 0) {
                DrawCircleAA(centerX, centerY, CellSize * 0.12f, 16, legalMoveColor, TRUE);
            }
        }
    }

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
