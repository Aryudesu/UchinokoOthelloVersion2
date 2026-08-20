#define NOMINMAX
#include "scene/GameScene.h"
#include "core/GameSettings.h"
#include "manager/InputManager.h"
#include "DxLib.h"

#include <cstdio>

namespace {
    Disc opponentOf(Disc disc) {
        return disc == Disc::Black ? Disc::White : Disc::Black;
    }

    std::string utf8ToLocal(std::u8string_view text) {
        return std::string(
            reinterpret_cast<const char*>(text.data()),
            text.size()
        );
    }


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
    character_.Start(
        "data/config/character.ini",
        GameSettings::GetInstance().difficultyIndex()
    );
    resetMatch();
}

void GameScene::End() {
    cancelAiSearch();
    character_.End();
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
        aiTurn_.Update();
        if (aiTurn_.HasCompleted()) finishAiSearch();
        return;
    }

    // A flip is part of applying the previous move. Do not accept another move
    // or start the AI until all three transition frames have been displayed.
    if (boardView_.IsAnimating()) return;

    if (gameOver_) {
        handleResultInput(clicked);
        return;
    }

    if (turn_ == aiDisc_) {
        startAiSearch();
        return;
    }

    if (clicked) {
        handleBoardClick();
    }
}

void GameScene::resetMatch() {
    cancelAiSearch();

    const auto& settings = GameSettings::GetInstance();
    const DifficultyProfile& difficulty = settings.difficulty();
    playerDisc_ = settings.playerDisc();
    aiDisc_ = settings.aiDisc();
    aiTurn_.Configure(difficulty);
    character_.Show(CharacterReaction::Start);

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
    phase_ = turn_ == playerDisc_
        ? Phase::PlayerTurn
        : Phase::ApplyingAiMove;

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
    if (board_.put(playerDisc_, row, col)) {
        boardView_.BeginMove(before, board_, playerDisc_, row, col);
        updateOpeningName();
        advanceTurn();
    }
}

void GameScene::performAiMove(
    std::optional<OthelloAI::Move> move
) {
    if (!move.has_value()) {
        advanceTurn();
        return;
    }

    const BitBoard before = board_;
    if (board_.put(aiDisc_, move->row, move->col)) {
        boardView_.BeginMove(
            before,
            board_,
            aiDisc_,
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

    aiTurn_.Start(board_, aiDisc_);
    character_.Show(CharacterReaction::Thinking);
    phase_ = Phase::AiThinking;
}

void GameScene::finishAiSearch() {
    phase_ = Phase::ApplyingAiMove;
    performAiMove(aiTurn_.TakeMove());
    if (!gameOver_) {
        phase_ = turn_ == aiDisc_
            ? Phase::ApplyingAiMove
            : Phase::PlayerTurn;
    }
}

void GameScene::cancelAiSearch() {
    aiTurn_.Cancel();
}

void GameScene::updateOpeningName() {
    const Disc nextTurn = opponentOf(turn_);
    const std::u8string_view name =
        aiTurn_.CompletedOpeningName(board_, nextTurn);
    if (!name.empty()) openingName_ = utf8ToLocal(name);
}

void GameScene::advanceTurn() {
    const Disc movedDisc = turn_;
    const Disc next = opponentOf(turn_);
    passed_ = Disc::Empty;

    if (board_.hasAnyMove(next)) {
        turn_ = next;
        updateCharacterReaction(movedDisc);
        return;
    }

    if (board_.hasAnyMove(turn_)) {
        passed_ = next;
        character_.Show(
            next == playerDisc_
                ? CharacterReaction::PlayerPass
                : CharacterReaction::AiPass
        );
        return;
    }

    gameOver_ = true;
    phase_ = Phase::GameOver;

    const MatchResult result = MatchResult::From(board_);
    const MatchWinner aiWinner = aiDisc_ == Disc::Black
        ? MatchWinner::Black
        : MatchWinner::White;
    if (result.winner == MatchWinner::Draw) {
        character_.Show(CharacterReaction::Draw);
    } else if (result.winner == aiWinner) {
        character_.Show(CharacterReaction::Win);
    } else {
        character_.Show(CharacterReaction::Lose);
    }
}

void GameScene::updateCharacterReaction(Disc movedDisc) {
    constexpr int AdvantageThreshold = 4;
    const int difference =
        board_.count(aiDisc_) - board_.count(playerDisc_);

    if (difference >= AdvantageThreshold) {
        character_.Show(CharacterReaction::Advantage);
    } else if (difference <= -AdvantageThreshold) {
        character_.Show(CharacterReaction::Disadvantage);
    } else {
        character_.Show(
            movedDisc == aiDisc_
                ? CharacterReaction::AiMove
                : CharacterReaction::PlayerMove
        );
    }
}

void GameScene::Draw() {
    const int whiteColor = GetColor(255, 255, 255);
    const int noticeColor = GetColor(255, 220, 80);

    const BitBoard::Bits legalMoves =
        !gameOver_ && turn_ == playerDisc_
        ? board_.legalMoves(playerDisc_)
        : 0;

    boardView_.Draw(
        board_,
        legalMoves,
        !boardView_.IsAnimating() && phase_ == Phase::PlayerTurn
    );

    const int blackCount = board_.count(Disc::Black);
    const int whiteCount = board_.count(Disc::White);
    const int playerCount = board_.count(playerDisc_);
    const int aiCount = board_.count(aiDisc_);

    char score[80];
    std::snprintf(
        score, sizeof(score),
        "You (%s): %d  AI (%s): %d",
        discName(playerDisc_),
        playerCount,
        discName(aiDisc_),
        aiCount
    );

    char status[64];
    if (gameOver_) {
        if (playerCount > aiCount) {
            std::snprintf(status, sizeof(status), "Game Over: You win");
        } else if (aiCount > playerCount) {
            std::snprintf(status, sizeof(status), "Game Over: AI wins");
        } else {
            std::snprintf(status, sizeof(status), "Game Over: Draw");
        }
    } else if (phase_ == Phase::AiThinking) {
        const auto elapsed = aiTurn_.ElapsedMilliseconds();
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
        ? aiTurn_.Progress().completedDepth.load(std::memory_order_relaxed)
        : aiSearchDepth_;
    const int targetDepth = thinking
        ? aiTurn_.Progress().targetDepth.load(std::memory_order_relaxed)
        : aiSearchDepth_;
    const auto displayNodes = thinking
        ? aiTurn_.Progress().searchedNodes.load(std::memory_order_relaxed)
        : aiSearchedNodes_;
    const auto displayHits = thinking
        ? aiTurn_.Progress().transpositionHits.load(std::memory_order_relaxed)
        : aiTranspositionHits_;
    const bool displayExact = thinking
        ? aiTurn_.Progress().exactSearch.load(std::memory_order_relaxed)
        : aiExactSearch_;
    if (!thinking && aiOpeningBook_) {
        std::snprintf(aiInfo, sizeof(aiInfo), "AI: OPENING BOOK");
    } else {
        std::snprintf(
            aiInfo,
            sizeof(aiInfo),
            "AI depth: %d/%d  Nodes: %llu  TT: %llu%s",
            displayDepth,
            targetDepth > 0 ? targetDepth : aiTurn_.ConfiguredDepth(),
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

    character_.Draw();

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

    const MatchWinner playerWinner = playerDisc_ == Disc::Black
        ? MatchWinner::Black
        : MatchWinner::White;
    const MatchWinner aiWinner = aiDisc_ == Disc::Black
        ? MatchWinner::Black
        : MatchWinner::White;

    const char* headline = "DRAW";
    if (result.winner == playerWinner) headline = "YOU WIN!";
    if (result.winner == aiWinner) headline = "AI WINS";
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
