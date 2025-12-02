#pragma once
#include "InputManager.h"
#include "SceneBase.h"
#include "TitleScene.h"
#include "Ids.h"
#include "DxLib.h"
#include "Logger.h"

void TitleScene::Start() {
}

void TitleScene::End() {
}

void TitleScene::Update() {
    if (InputManager::GetInstance().isPressed(KEY_INPUT_ESCAPE)) next_ = SceneID::Quit;
    if (InputManager::GetInstance().isPressed(KEY_INPUT_RETURN)) next_ = SceneID::Game;
}

void TitleScene::Draw() {
    DrawString(32, 32, "Title: ENTER->Game, ESC->Quit", GetColor(255, 255, 255));
}

bool TitleScene::IsEnd() const { return next_ != SceneID::Title; }
SceneID TitleScene::NextScene() { return next_; }
