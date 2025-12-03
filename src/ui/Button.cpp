#include "template/SoundManager.h"
#include "ui/Button.h"
#include "Dxlib.h"

//(x,y)を中心とした縁付フォントで描画
void Button::BorderFontDraw(int x, int y, int fontsize, int fonthandle, int thick, std::string str, int Color, int ColorThick, int ColorBorder) {
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

Button::Button(int x_, int y_, int w_, int h_, std::vector<std::string> mes, int F, int FS) {
	x = x_;
	y = y_;
	width = w_;
	height = h_;
	count = 0;
	str = mes;
	FontH = F;
	FontSize = FS;
}

bool Button::IsOnMouse(int mx, int my) {
	return (mx >= x && mx < x + width && my >= y && my < y + height);
}

bool Button::Update() {
	int mx, my;
	GetMousePoint(&mx, &my);
	bool onmouse = IsOnMouse(mx, my);
	bool result = false;
	if (onmouse && MouseLeft()) {
		SoundManager::GetInstance().PlaySE(SELECT);
		result = true;
		count++;
		if (str.size() <= count)count = 0;
	}
	return result;
}

void Button::Draw() {
	int mx, my;
	GetMousePoint(&mx, &my);
	bool onmouse = IsOnMouse(mx, my);
	DrawBox(x, y, x + width, y + height, GetColor(44, 44, 44), TRUE);
	DrawBox(x + 1, y + 1, x + width - 1, y + height - 1, GetColor(onmouse ? 128 + 64 : 128, 0, 0), TRUE);
	DrawBox(x + 8, y + 8, x + width - 8, y + height - 8, onmouse ? GetColor(50 + 25, 200 + 50, 50 + 25) : GetColor(50, 200, 50), TRUE);
	DrawBox(x + 8, y + 8, x + width - 8, y + height - 8, GetColor(44, 44, 44), FALSE);
	BorderFontDraw(x + width / 2, y + height / 2, FontSize, FontH, 2, str[count], GetColor(44, 44, 44), GetColor(255, 255, 255), GetColor(0, 0, 0));
}

bool Button::MouseLeft() {
	int Mouse;
	static bool f;
	Mouse = GetMouseInput();
	if ((Mouse & MOUSE_INPUT_LEFT)) {
		if (!f) {
			f = true;
			return true;
		}
	}
	else {
		f = false;
		return false;
	}
	return false;
}

int Button::GetClickNum() { return count; }