#pragma once
#include "Button.h"

class OthelloTitle {
	Button *Title;
	Button *Sente;
	Button *Gote;
	Button *Music;
	Button *Difficality;
	int FontH, LFontH;

	void BorderFontDraw(int x, int y, int fontsize, int fonthandle, int thick, std::string str, int Color, int ColorThick, int ColorBorder);
	bool MouseLeft();
public:
	OthelloTitle();
	~OthelloTitle();
	int update();
	void draw();
};