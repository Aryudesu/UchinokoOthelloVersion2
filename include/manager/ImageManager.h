#pragma once
#include "core/Singleton.h"
#include "Dxlib.h"
#include <vector>
#include <string>


//画像管理用クラス
//とりあえず領域確保と画像読み込み、描画、データ消しまで
class ImageManager : public Singleton<ImageManager> {
private:
	std::vector<std::vector<int>> imgs_;
public:
	friend class Singleton < ImageManager >;

	ImageManager() = default;
	~ImageManager();

	static void safeDelete(int& h) {
		if (h != -1) { DeleteGraph(h); h = -1; }
	}

public:

	//透過色設定
	void SetTrans(int R, int G, int B);

	//画像読み込み
	//ID : オブジェクトID
	//sizex,sizey　縦横切り取るピクセルサイズ
	//CutX,CutY カット数
	//FileName　画像ファイル名
	void Load(int ID, int sizex, int sizey, int CutX, int CutY, std::string FileName);

	//画像読み込み
	//ID : オブジェクトID
	//FileName　画像ファイル名
	void Load(int ID, std::string FileName);

	//画像読み込み
	//ID : オブジェクトID
	//CutX,CutY カット数
	//FileName　画像ファイル名
	void LoadSheet(int ID, int CutX, int CutY, std::string FileName);

	//画像サイズ
	void Size(int ID, int num, int& width, int& height);
	void Size(int ID, int& width, int& height);

	//画像描画
	//x,y　画面位置
	//ID,num　オブジェクトIDと描画番号
	//TransFlag　透過するかしないか　TRUE/FALSE
	void Draw(float x, float y, int ID, int num, int TransFlag, int TurnY = FALSE);

	//画像描画（0,0に描画）
	//ID　オブジェクトID
	//TransFlag　透過するかしないか　TRUE/FALSE
	void Draw(int ID, int TransFlag);

	//画像描画
	//x,y　画面位置
	//ID,num　オブジェクトIDと描画番号
	//TransFlag　透過するかしないか　TRUE/FALSE
	//TurnY　上下反転
	void Draw(float x, float y, int ID, int TransFlag);

	//IDのオブジェクトの画像破棄
	void Destroy(int ID);

	void DeleteAll();
};