#pragma once
#include "core/Ids.h"

class SceneBase {
public:
	virtual ~SceneBase() = default;

	virtual void Start() {}
	virtual void End() {}
	virtual void Update() = 0;
	virtual void Draw() = 0;

	// シーン切り替え関係
	virtual bool IsEnd() const = 0;
	virtual SceneID NextScene() const = 0;

	// アプリ終了フラグ
	bool WantsQuit() const noexcept { return wantsQuit_; }

protected:
	// 派生クラスから呼ぶ用
	void RequestQuit() noexcept { wantsQuit_ = true; }

private:
	bool wantsQuit_ = false;
};
