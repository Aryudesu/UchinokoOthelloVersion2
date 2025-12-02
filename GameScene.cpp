#pragma once
#include "InputManager.h"
#include "SceneBase.h"
#include "GameScene.h"
#include "Ids.h"
#include "DxLib.h"
#include "Logger.h"

void GameScene::Start() {
}

void GameScene::End() {
}

void GameScene::Update() {
    if (InputManager::GetInstance().isPressed(KEY_INPUT_ESCAPE)) next_ = SceneID::Quit;
    if (InputManager::GetInstance().isPressed(KEY_INPUT_RETURN)) next_ = SceneID::Title;
}

void GameScene::Draw() {
    DrawString(32, 32, "Game: ENTER->Title, ESC->Quit", GetColor(255, 255, 255));
}

bool GameScene::IsEnd() const { return next_ != SceneID::Game; }
SceneID GameScene::NextScene() { return next_; }
