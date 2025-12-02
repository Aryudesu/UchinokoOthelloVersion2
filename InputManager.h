#pragma once
#include "DxLib.h"
#include "Singleton.h"
#include <array>
#include <cstdint>

constexpr int KEY_NUM = 256;

// キー入力管理
class InputManager : public Singleton<InputManager> {
    friend class Singleton<InputManager>;

private:
    std::array<uint8_t, KEY_NUM> tmp_{};     // GetHitKeyStateAllの生配列
    std::array<uint32_t, KEY_NUM> hold_{};   // 押下継続フレーム数
public:
    // 毎フレーム先頭で呼ぶ
    int Update() {
        char raw[KEY_NUM];
        GetHitKeyStateAll(raw);
        for (int i = 0; i < KEY_NUM; ++i) {
            tmp_[i] = (raw[i] != 0) ? 1 : 0;
            if (tmp_[i]) {
                if (hold_[i] < 0xFFFFFFFFu) ++hold_[i];
            } else {
                hold_[i] = 0;
            }
        }
        return 0;
    }

    // 指定キーの継続フレーム数（0=押してない, 1~）
    int ReturnKey(int key) const {
        if (key < 0 || key >= KEY_NUM) return 0;
        return static_cast<int>(hold_[key]);
    }

    bool isDown(int key) const { return (key >= 0 && key < KEY_NUM) ? tmp_[key] != 0 : false; }
    bool isPressed(int key) const { return (key >= 0 && key < KEY_NUM) ? (hold_[key] == 1) : false; }
    bool isReleased(int key) const { return (key >= 0 && key < KEY_NUM) ? (!tmp_[key] && hold_[key] == 0) : false; }
};
