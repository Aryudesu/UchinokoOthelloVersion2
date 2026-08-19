#include "manager/InputManager.h"
#include "scene/SceneBase.h"
#include "scene/TitleScene.h"
#include "core/GameSettings.h"
#include "core/Ids.h"
#include "util/Log.h"
#include "DxLib.h"

#include <cstdio>

void TitleScene::Start() {
    end_ = false;
    next_ = SceneID::Title;
    selectedRow_ = 0;
    GameSettings::GetInstance().Load("data/config/game.ini");
}

void TitleScene::End() {
}

void TitleScene::Update() {
    auto& input = InputManager::GetInstance();
    auto& settings = GameSettings::GetInstance();

    if (input.isPressed(KEY_INPUT_ESCAPE)) {
        RequestQuit();
        return;
    }

    if (
        input.isPressed(KEY_INPUT_UP) ||
        input.isPressed(KEY_INPUT_DOWN)
    ) {
        selectedRow_ = 1 - selectedRow_;
    }

    if (
        input.isPressed(KEY_INPUT_LEFT) ||
        input.isPressed(KEY_INPUT_RIGHT)
    ) {
        if (selectedRow_ == 0) {
            settings.TogglePlayerOrder();
        } else if (input.isPressed(KEY_INPUT_LEFT)) {
            settings.PreviousDifficulty();
        } else {
            settings.NextDifficulty();
        }
    }

    if (input.isPressed(KEY_INPUT_RETURN)) {
        next_ = SceneID::Game;
        end_ = true;
    }
}

void TitleScene::Draw() {
    const auto& settings = GameSettings::GetInstance();
    const DifficultyProfile& difficulty = settings.difficulty();
    const int white = GetColor(255, 255, 255);
    const int selected = GetColor(255, 220, 80);
    const int secondary = GetColor(170, 180, 200);

    DrawString(32, 32, "UCHINOKO OTHELLO", white);
    DrawString(32, 64, "UP/DOWN: Select  LEFT/RIGHT: Change", secondary);
    DrawString(32, 88, "ENTER: Start  ESC: Quit", secondary);

    char order[64];
    std::snprintf(
        order, sizeof(order),
        "PLAYER: %s (%s)",
        settings.playerDisc() == Disc::Black ? "FIRST" : "SECOND",
        settings.playerDisc() == Disc::Black ? "BLACK" : "WHITE"
    );

    char level[64];
    std::snprintf(
        level, sizeof(level),
        "DIFFICULTY: %s",
        difficulty.name.c_str()
    );

    DrawString(48, 136, order, selectedRow_ == 0 ? selected : white);
    DrawString(48, 176, level, selectedRow_ == 1 ? selected : white);

    char details[128];
    std::snprintf(
        details, sizeof(details),
        "Depth %d  Exact %d empties  Limit %.1fs",
        difficulty.searchDepth,
        difficulty.exactEndgameEmpty,
        difficulty.timeLimitMs / 1000.0
    );
    DrawString(64, 208, details, secondary);
}

bool TitleScene::IsEnd() const { return end_; }
SceneID TitleScene::NextScene() const { return next_; }
