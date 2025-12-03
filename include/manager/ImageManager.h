#pragma once
#include <vector>
#include <string>
#include "core/Ids.h"  // enum class ImageID

class ImageManager {
public:
    // シングルトンインスタンス取得
    static ImageManager& GetInstance() {
        static ImageManager inst;
        return inst;
    }

    // 透過色設定（DxLib の SetTransColor ラッパ）
    void SetTrans(int r, int g, int b);

    // 単一画像読み込み
    void Load(ImageID id, const std::string& filename);

    // 分割画像読み込み（任意サイズ）
    void LoadDiv(ImageID id, int sizex, int sizey,
        int cutX, int cutY,
        const std::string& filename);

    // 分割画像読み込み（32x32 固定など、シート用の簡易版）
    void LoadSheet(ImageID id, int cutX, int cutY, const std::string& filename);

    // サイズ取得（num 指定）
    void Size(ImageID id, int num, int& width, int& height) const;
    // 最初の要素のサイズ
    void Size(ImageID id, int& width, int& height) const;

    // 描画（スプライト）
    void Draw(float x, float y,
        ImageID id, int num = 0,
        bool transFlag = true, bool turnY = false) const;

    // 画面左上に描画
    void Draw(ImageID id, bool transFlag = true) const {
        Draw(0.0f, 0.0f, id, 0, transFlag);
    }

    // ID の画像をすべて破棄
    void Destroy(ImageID id);

    // 全画像破棄
    void DeleteAll();

private:
    ImageManager();
    ~ImageManager();

    ImageManager(const ImageManager&) = delete;
    ImageManager& operator=(const ImageManager&) = delete;

    using Handle = int;
    std::vector<std::vector<Handle>> imgs_;

    // id からインデックスに変換し、必要なら拡張
    std::vector<Handle>& getSlot(ImageID id);
    const std::vector<Handle>& getSlot(ImageID id) const;

    // 安全な削除
    static void safeDelete(Handle& h);
};
