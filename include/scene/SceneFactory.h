#pragma once
#include <memory>
#include "core/Ids.h"
#include "scene/SceneBase.h"

// ここで各具体シーンのヘッダをinclude
#include "TitleScene.h"
#include "GameScene.h"

inline std::unique_ptr<SceneBase> CreateScene(SceneID id) {
    switch (id) {
    case SceneID::Title:  return std::make_unique<TitleScene>();
    case SceneID::Game:   return std::make_unique<GameScene>();
    //case SceneID::Result: return std::make_unique<ResultScene>();
    default:              return nullptr;
    }
}
