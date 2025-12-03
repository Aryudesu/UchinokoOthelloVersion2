#include <string>
#include "template/SoundManager.h"
#include "scene/OthelloTitle.h"
#include "Dxlib.h"

using namespace std;

OthelloTitle::OthelloTitle() {
	FontH = LoadFontDataToHandle("dat/font/07にくまるフォント32.dft", 0);
	LFontH = LoadFontDataToHandle("dat/font/07にくまるフォント80.dft", 0);
	SoundManager::GetInstance().SetBGM(BGM1, 12666, "dat/sound/WallStreetRag.wav");
	SoundManager::GetInstance().ChangeBGMVolume(0);
	SoundManager::GetInstance().PlayBGM(BGM1);
	SoundManager::GetInstance().SetSE(SELECT, "dat/sound/oku.wav");
	SoundManager::GetInstance().ChangeVolume(0);
	Title = new Button(640 / 2 - 32 * 7, 32, 32 * 14, 32 * 5, { "OTHELLO" }, LFontH, 80);
	Sente = new Button(640 / 4 - 32 * 2, 480 * 4 / 5, 32 * 6, 32 * 2, { "先手" }, FontH, 32);
	Gote = new Button(640 * 3 / 4 - 32 * 4, 480 * 4 / 5, 32 * 6, 32 * 2, { "後手" }, FontH, 32);
	Music = new Button(640 / 2 - 32 * 7, 32 * 7, 32 * 14, 32 * 2, { "Something Doing", "Original Rags", "Pineapple Rag", "Peacherine Rag", "Elite Syncopations"}, FontH, 32);
	Difficality = new Button(640 / 2 - 32 * 7, 32 * 9+16, 32 * 14, 32 * 2, { "Easy", "Normal", "Hard"}, FontH, 32);
}

OthelloTitle::~OthelloTitle() {
	DeleteFontToHandle(FontH);
	DeleteFontToHandle(LFontH);
	delete Title;
	delete Sente;
	delete Gote;
	delete Music;
	delete Difficality;
	SoundManager::GetInstance().StopBGM(BGM1);
	SoundManager::GetInstance().DeleteBGM(BGM1);
}

int OthelloTitle::update() {
	if (Sente->Update()) {
		return 1 + 10 * Difficality->GetClickNum() + 100 * Music->GetClickNum();
	}
	if (Gote->Update()) {
		return 2 + 10 * Difficality->GetClickNum() + 100 * Music->GetClickNum();
	}
	Music->Update();
	Difficality->Update();
	return 0;
}

void OthelloTitle::draw() {
	Title->Draw();
	Sente->Draw();
	Gote->Draw();
	Music->Draw();
	Difficality->Draw();
}

//(x,y)を中心とした縁付フォントで描画
void OthelloTitle::BorderFontDraw(int x, int y, int fontsize, int fonthandle, int thick, std::string str, int Color, int ColorThick, int ColorBorder) {
	int width = GetDrawStringWidthToHandle(str.c_str(), str.size(), fonthandle);
	int x_ = x - width / 2;
	int y_ = y - fontsize / 2;
	for (int i = -thick - 1; i <= thick + 1; i++) {
		for (int j = -thick - 1; j <= thick + 1; j++) {
			DrawStringToHandle(x_ + i, y_ + j, str.c_str(), ColorBorder, fonthandle);
		}
	}
	for (int i = -thick; i <= thick; i++) {
		for (int j = -thick; j <= thick; j++) {
			DrawStringToHandle(x_ + i, y_ + j, str.c_str(), ColorThick, fonthandle);
		}
	}
	DrawStringToHandle(x_, y_, str.c_str(), Color, fonthandle);
}
