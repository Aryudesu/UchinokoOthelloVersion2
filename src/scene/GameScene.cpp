#define NOMINMAX
#include "scene/GameScene.h"
#include "manager/InputManager.h"
#include "DxLib.h"
#include <Windows.h>

#include <cstdio>

namespace {
    Disc opponentOf(Disc disc) {
        return disc == Disc::Black ? Disc::White : Disc::Black;
    }

    std::string utf8ToLocal(std::u8string_view text) {
        if (text.empty()) return {};

        const char* bytes = reinterpret_cast<const char*>(text.data());
        const int byteCount = static_cast<int>(text.size());
        const int wideCount = MultiByteToWideChar(
            CP_UTF8, 0, bytes, byteCount, nullptr, 0
        );
        if (wideCount <= 0) return {};

        std::wstring wide(static_cast<std::size_t>(wideCount), L'\0');
        MultiByteToWideChar(
            CP_UTF8, 0, bytes, byteCount, wide.data(), wideCount
        );

        const int localCount = WideCharToMultiByte(
            CP_ACP, 0, wide.data(), wideCount, nullptr, 0, nullptr, nullptr
        );
        if (localCount <= 0) return {};

        std::string local(static_cast<std::size_t>(localCount), '\0');
        WideCharToMultiByte(
            CP_ACP, 0, wide.data(), wideCount,
            local.data(), localCount, nullptr, nullptr
        );
        return local;
    }


    // Keep instant opening-book moves and shallow searches from feeling abrupt.
    // Searches that take longer than this are applied as soon as they finish.
    constexpr auto MinimumAiThinkingTime = std::chrono::milliseconds(700);

    constexpr int ResultPanelLeft = 180;
    constexpr int ResultPanelTop = 150;
    constexpr int ResultPanelRight = 540;
    constexpr int ResultPanelBottom = 350;
    constexpr int RematchLeft = 220;
    constexpr int RematchRight = 340;
    constexpr int TitleLeft = 380;
    constexpr int TitleRight = 500;
    constexpr int ButtonTop = 295;
    constexpr int ButtonBottom = 330;

    bool insideRect(
        int x, int y,
        int left, int top, int right, int bottom
    ) noexcept {
        return left <= x && x < right && top <= y && y < bottom;
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
    boardView_.Start();
    resetMatch();
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
        const bool searchFinished =
            aiFinished_.load(std::memory_order_acquire);
        const bool minimumTimeElapsed =
            std::chrono::steady_clock::now() - aiStartedAt_ >=
            MinimumAiThinkingTime;
        if (searchFinished && minimumTimeElapsed) finishAiSearch();
        return;
    }

    // A flip is part of applying the previous move. Do not accept another move
    // or start the AI until all three transition frames have been displayed.
    if (boardView_.IsAnimating()) return;

    if (gameOver_) {
        handleResultInput(clicked);
        return;
    }

    if (turn_ == Disc::White) {
        startAiSearch();
        return;
    }

    if (clicked) {
        handleBoardClick();
    }
}

void GameScene::resetMatch() {
    cancelAiSearch();

    end_ = false;
    next_ = SceneID::Game;
    board_.reset();
    boardView_.Reset(board_);
    turn_ = Disc::Black;
    passed_ = Disc::Empty;
    gameOver_ = false;
    mouseLeftDown_ = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;
    GetMousePoint(&resultMouseX_, &resultMouseY_);
    aiSearchedNodes_ = 0;
    aiSearchDepth_ = 0;
    aiExactSearch_ = false;
    aiTranspositionHits_ = 0;
    aiIterations_ = 0;
    aiOpeningBook_ = false;
    openingName_.clear();
    resultChoice_ = ResultChoice::Rematch;
    phase_ = Phase::PlayerTurn;
    aiProgress_.reset(0, false);
    aiFinished_.store(false, std::memory_order_relaxed);
}

void GameScene::handleResultInput(bool clicked) {
    auto& input = InputManager::GetInstance();

    const bool changedByKeyboard =
        input.isPressed(KEY_INPUT_LEFT) ||
        input.isPressed(KEY_INPUT_RIGHT) ||
        input.isPressed(KEY_INPUT_UP) ||
        input.isPressed(KEY_INPUT_DOWN);
    if (changedByKeyboard) {
        resultChoice_ = resultChoice_ == ResultChoice::Rematch
            ? ResultChoice::Title
            : ResultChoice::Rematch;
    }

    int mouseX = 0;
    int mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);

    const bool mouseMoved =
        mouseX != resultMouseX_ || mouseY != resultMouseY_;
    resultMouseX_ = mouseX;
    resultMouseY_ = mouseY;

    const bool overRematch = insideRect(
        mouseX, mouseY,
        RematchLeft, ButtonTop, RematchRight, ButtonBottom
    );
    const bool overTitle = insideRect(
        mouseX, mouseY,
        TitleLeft, ButtonTop, TitleRight, ButtonBottom
    );

    if (clicked && (overRematch || overTitle)) {
        resultChoice_ = overRematch
            ? ResultChoice::Rematch
            : ResultChoice::Title;
        applyResultChoice();
        return;
    }

    // A stationary cursor must not overwrite a keyboard selection on the
    // following frame. Hover selection changes only when the mouse moves.
    if (mouseMoved && !changedByKeyboard) {
        if (overRematch) resultChoice_ = ResultChoice::Rematch;
        if (overTitle) resultChoice_ = ResultChoice::Title;
    }

    if (input.isPressed(KEY_INPUT_RETURN)) applyResultChoice();
}

void GameScene::applyResultChoice() {
    if (resultChoice_ == ResultChoice::Rematch) {
        resetMatch();
        return;
    }

    next_ = SceneID::Title;
    end_ = true;
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
        updateOpeningName();
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
        aiOpeningBook_ = move->openingBook;
        updateOpeningName();
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

void GameScene::updateOpeningName() {
    const Disc nextTurn = opponentOf(turn_);
    const std::u8string_view name =
        ai_.completedOpeningName(board_, nextTurn);
    if (!name.empty()) openingName_ = utf8ToLocal(name);
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
    if (!thinking && aiOpeningBook_) {
        std::snprintf(aiInfo, sizeof(aiInfo), "AI: OPENING BOOK");
    } else {
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
    }

    DrawString(32, 32, "Othello vs AI", whiteColor);
    DrawString(32, 56, "Click a legal move / ESC : Back to Title", whiteColor);
    DrawString(32, 80, score, whiteColor);
    DrawString(32, 104, status, gameOver_ ? noticeColor : whiteColor);
    DrawString(32, 128, aiInfo, whiteColor);

    if (!openingName_.empty()) {
        const std::string openingInfo = "Opening: " + openingName_;
        DrawString(32, 152, openingInfo.c_str(), noticeColor);
    }

    if (passed_ != Disc::Empty) {
        char passMessage[64];
        std::snprintf(passMessage, sizeof(passMessage), "%s passes", discName(passed_));
        DrawString(32, openingName_.empty() ? 152 : 176, passMessage, noticeColor);
    }

    if (gameOver_ && !boardView_.IsAnimating()) {
        drawResult(MatchResult::From(board_));
    }
}

void GameScene::drawResult(const MatchResult& result) const {
    const int white = GetColor(255, 255, 255);
    const int accent = GetColor(255, 220, 80);
    const int panel = GetColor(20, 24, 36);
    const int selected = GetColor(70, 110, 180);
    const int idle = GetColor(45, 52, 70);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
    DrawBox(
        ResultPanelLeft, ResultPanelTop,
        ResultPanelRight, ResultPanelBottom,
        panel, TRUE
    );
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    const char* headline = "DRAW";
    if (result.winner == MatchWinner::Black) headline = "YOU WIN!";
    if (result.winner == MatchWinner::White) headline = "AI WINS";
    DrawString(310, 175, headline, accent);

    char finalScore[64];
    std::snprintf(
        finalScore, sizeof(finalScore),
        "BLACK %d  -  %d WHITE",
        result.blackCount, result.whiteCount
    );
    DrawString(270, 210, finalScore, white);

    char difference[64];
    std::snprintf(
        difference, sizeof(difference),
        result.difference == 0
            ? "EVEN GAME"
            : "DIFFERENCE: %d",
        result.difference
    );
    DrawString(300, 240, difference, white);
    DrawString(260, 268, "Select with mouse or arrow keys", white);

    DrawBox(
        RematchLeft, ButtonTop, RematchRight, ButtonBottom,
        resultChoice_ == ResultChoice::Rematch ? selected : idle,
        TRUE
    );
    DrawBox(
        TitleLeft, ButtonTop, TitleRight, ButtonBottom,
        resultChoice_ == ResultChoice::Title ? selected : idle,
        TRUE
    );
    DrawString(246, 305, "REMATCH", white);
    DrawString(420, 305, "TITLE", white);
}

bool GameScene::IsEnd() const {
    return end_;
}

SceneID GameScene::NextScene() const {
    return next_;
}
