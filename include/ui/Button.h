#pragma once
#include <vector>
#include <string>

class Button {
	int x, y;
	int width, height;
	int FontH, FontSize;
	std::vector<std::string> str;
	int count;
	void BorderFontDraw(int x, int y, int fontsize, int fonthandle, int thick, std::string str, int Color, int ColorThick, int ColorBorder);
	bool MouseLeft();
public:
	Button(int x_, int y_, int w_, int h_, std::vector<std::string> mes, int F, int FS);
	bool IsOnMouse(int mx, int my);
	bool Update();
	void Draw();
	int GetClickNum();
};
