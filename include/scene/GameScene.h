#pragma once
#include "scene/SceneBase.h"
#include "core/Ids.h"
#include "model/BitBoard.h"
#include "model/MatchResult.h"
#include "ai/AiTurnController.h"
#include "view/BoardView.h"

#include <cstdint>
#include <optional>
#include <string>

class GameScene : public SceneBase {
    enum class Phase {
        PlayerTurn,
        AiThinking,
        ApplyingAiMove,
        GameOver,
    };

    enum class ResultChoice {
        Rematch,
        Title,
    };

    bool end_ = false;
    SceneID next_ = SceneID::Game;
    BitBoard board_;
    BoardView boardView_;
    AiTurnController aiTurn_;
    Disc playerDisc_ = Disc::Black;
    Disc aiDisc_ = Disc::White;
    Disc turn_ = Disc::Black;
    Disc passed_ = Disc::Empty;
    bool gameOver_ = false;
    bool mouseLeftDown_ = false;
    int resultMouseX_ = 0;
    int resultMouseY_ = 0;
    std::uint64_t aiSearchedNodes_ = 0;
    int aiSearchDepth_ = 0;
    bool aiExactSearch_ = false;
    std::uint64_t aiTranspositionHits_ = 0;
    int aiIterations_ = 0;
    bool aiOpeningBook_ = false;
    std::string openingName_;
    Phase phase_ = Phase::PlayerTurn;
    ResultChoice resultChoice_ = ResultChoice::Rematch;

    void resetMatch();
    void handleBoardClick();
    void handleResultInput(bool clicked);
    void applyResultChoice();
    void drawResult(const MatchResult& result) const;
    void performAiMove(std::optional<OthelloAI::Move> move);
    void advanceTurn();
    void startAiSearch();
    void finishAiSearch();
    void cancelAiSearch();
    void updateOpeningName();

public:
    ~GameScene() override;
    void Start() override;
    void End() override;
    void Update() override;
    void Draw() override;
    bool IsEnd() const override;
    SceneID NextScene() const override;
};
