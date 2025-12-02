#pragma once
#include <memory>
#include "core/Ids.h"
#include "scene/SceneBase.h"
#include "scene/SceneFactory.h"

class SceneManager {
    std::unique_ptr<SceneBase> cur_;
    bool quit_ = false;

public:
    void startWith(SceneID first) {
        cur_ = CreateScene(first);
        if (cur_) cur_->Start();
        else quit_ = true;
    }

    bool running() const { return !quit_; }

    void updateAndDraw() {
        if (!cur_) { quit_ = true; return; }

        cur_->Update();
        cur_->Draw();

        if (cur_->IsEnd()) {
            SceneID next = cur_->NextScene();
            cur_->End();
            cur_.reset();

            if (next == SceneID::Quit) { quit_ = true; return; }

            cur_ = CreateScene(next);
            if (cur_) cur_->Start();
            else quit_ = true;
        }
    }
};
