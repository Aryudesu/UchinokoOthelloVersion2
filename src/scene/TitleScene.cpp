#pragma once
#include "manager/InputManager.h"
#include "scene/SceneBase.h"
#include "scene/TitleScene.h"
#include "core/Ids.h"
#include "util/Log.h"
#include "DxLib.h"

void TitleScene::Start() {
    end_ = false;
    next_ = SceneID::Title;
}

void TitleScene::End() {
}

void TitleScene::Update() {
    auto& input = InputManager::GetInstance();

    if (input.isPressed(KEY_INPUT_ESCAPE)) {
        // ƒAƒvƒŠI—¹
        RequestQuit();
        return;
    }

    if (input.isPressed(KEY_INPUT_RETURN)) {
        // Game ‚Ö‘JˆÚ
        next_ = SceneID::Game;
        end_ = true;
        return;
    }
}

void TitleScene::Draw() {
    DrawString(32, 32, "Title: ENTER->Game, ESC->Quit", GetColor(255, 255, 255));
}

bool TitleScene::IsEnd() const { return end_; }
SceneID TitleScene::NextScene() const { return next_; }
