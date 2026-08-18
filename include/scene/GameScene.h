#pragma once
#include "scene/SceneBase.h"
#include "core/Ids.h"
#include "model/BitBoard.h"
#include "model/MatchResult.h"
#include "ai/OthelloAI.h"
#include "view/BoardView.h"

#include <cstdint>
#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <thread>
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
    OthelloAI ai_{ 5 };
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
    OthelloAI::SearchProgress aiProgress_;
    std::optional<OthelloAI::Move> pendingAiMove_;
    std::mutex aiResultMutex_;
    std::atomic<bool> aiFinished_{ false };
    std::chrono::steady_clock::time_point aiStartedAt_;
    std::jthread aiThread_;

    void resetMatch();
    void handleBoardClick();
    void handleResultInput(bool clicked);
    void applyResultChoice();
    void drawResult(const MatchResult& result) const;
    void performAiMove();
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
