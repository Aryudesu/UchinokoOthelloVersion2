#pragma once
#include <memory>
#include "core/Ids.h"
#include "scene/SceneBase.h"
#include "scene/SceneFactory.h"

class SceneManager {
    std::unique_ptr<SceneBase> cur_;
    bool quit_ = false;

    // シーン切り替え専用の内部関数
    void changeScene(SceneID next) {
        if (cur_) {
            cur_->End();
            cur_.reset();
        }

        if (next == SceneID::Quit) {
            quit_ = true;
            return;
        }

        cur_ = CreateScene(next);
        if (cur_) {
            cur_->Start();
        }
        else {
            // 次のシーンが作れなかったら終了
            quit_ = true;
        }
    }

public:
    void startWith(SceneID first) {
        quit_ = false;
        cur_ = CreateScene(first);
        if (cur_) {
            cur_->Start();
        }
        else {
            quit_ = true;
        }
    }

    bool running() const { return !quit_; }

    // --- 更新フェーズ ---
    void update() {
        if (quit_) return;
        if (!cur_) { quit_ = true; return; }

        cur_->Update();

        if (cur_->IsEnd()) {
            SceneID next = cur_->NextScene();
            changeScene(next);
        }
    }

    // --- 描画フェーズ ---
    void draw() const {
        if (quit_) return;
        if (!cur_) return;

        cur_->Draw();
    }
};
