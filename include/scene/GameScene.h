#pragma once
#include "scene/SceneBase.h"
#include "core/Ids.h"
#include "model/BitBoard.h"
#include "ai/OthelloAI.h"

#include <cstdint>

class GameScene : public SceneBase {
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

    static constexpr int BoardSize = BitBoard::Size;
    static constexpr int CellSize = 32;
    static constexpr int BoardLeft = 200;
    static constexpr int BoardTop = 80;

    void handleBoardClick();
    void performAiMove();
    void advanceTurn();

public:
    void Start() override;
    void End() override;
    void Update() override;
    void Draw() override;
    bool IsEnd() const override;
    SceneID NextScene() const override;
};
