#pragma once
#include "scene/SceneBase.h"
#include "core/Ids.h"
#include "model/BitBoard.h"
#include "ai/OthelloAI.h"

#include <cstdint>
#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <thread>

class GameScene : public SceneBase {
    enum class Phase {
        PlayerTurn,
        AiThinking,
        ApplyingAiMove,
        GameOver,
    };

    bool end_ = false;
    SceneID next_ = SceneID::Game;
    BitBoard board_;
    OthelloAI ai_{ 5 };
    Disc turn_ = Disc::Black;
    Disc passed_ = Disc::Empty;
    bool gameOver_ = false;
    bool mouseLeftDown_ = false;
    std::uint64_t aiSearchedNodes_ = 0;
    int aiSearchDepth_ = 0;
    bool aiExactSearch_ = false;
    std::uint64_t aiTranspositionHits_ = 0;
    int aiIterations_ = 0;
    Phase phase_ = Phase::PlayerTurn;
    OthelloAI::SearchProgress aiProgress_;
    std::optional<OthelloAI::Move> pendingAiMove_;
    std::mutex aiResultMutex_;
    std::atomic<bool> aiFinished_{ false };
    std::chrono::steady_clock::time_point aiStartedAt_;
    std::jthread aiThread_;

    static constexpr int BoardSize = BitBoard::Size;
    static constexpr int CellSize = 32;
    static constexpr int BoardLeft = 200;
    static constexpr int BoardTop = 80;

    void handleBoardClick();
    void performAiMove();
    void advanceTurn();
    void startAiSearch();
    void finishAiSearch();
    void cancelAiSearch();

public:
    ~GameScene() override;
    void Start() override;
    void End() override;
    void Update() override;
    void Draw() override;
    bool IsEnd() const override;
    SceneID NextScene() const override;
};
