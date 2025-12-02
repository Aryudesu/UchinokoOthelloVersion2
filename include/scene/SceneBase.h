#pragma once
#include "core/Ids.h"

class SceneBase {
	public:
	virtual ~SceneBase() = default;
	// シーンの開始処理（リソース読み込み等）
	virtual void Start() = 0;
	// シーンの終了処理（リソース破棄等）
	virtual void End() = 0;
	// シーンの終了判定
	virtual bool IsEnd() const = 0;
	// シーンの更新処理
	virtual void Update() = 0;
	// シーンの描画処理
	virtual void Draw() = 0;
	// 次のシーンIDを取得
	virtual SceneID NextScene() = 0;
};