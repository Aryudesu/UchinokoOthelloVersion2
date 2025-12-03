#pragma once
#include "manager/InputManager.h"
#include "scene/SceneBase.h"
#include "scene/GameScene.h"
#include "core/Ids.h"
#include "util/Log.h"
#include "DxLib.h"

void GameScene::Start() {
}

void GameScene::End() {
}

void GameScene::Update() {
    if (InputManager::GetInstance().isPressed(KEY_INPUT_RETURN)) next_ = SceneID::Title;
}

void GameScene::Draw() {
    DrawString(32, 32, "Game: ENTER->Title, ESC->Quit", GetColor(255, 255, 255));
}

bool GameScene::IsEnd() const { return next_ != SceneID::Game; }
SceneID GameScene::NextScene() { return next_; }
