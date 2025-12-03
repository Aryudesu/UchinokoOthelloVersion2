#include "OthelloAI.h"
//盤面評価関数を定義．


//盤面評価．bとかwとかついているけど，bが評価している側でwが相手側．
int OthelloAI::CalcValue(BBoard b, BBoard w) {
	if (DIFFICALTY == VERYEASY)return GetMyNum(b) - GetMyNum(w);
	if (DIFFICALTY == EASY)return CalcPos(b, w);
	if (DIFFICALTY == NORMAL)return Noob * CalcPos(b,w) * 2 + PutAbleNum(b, w) * 7 + Noob * CountKakutei(b, w) * 80 + Noob * (Mount(b, w) - Wing(b, w)) / 2;
}

//位置による評価
int OthelloAI::CalcPos(BBoard b, BBoard w) {
	int Result = 0;
	int* p = &BoadEval[0];
	//盤面評価値の値をどんどん足していく．
	for (BBoard m = 0x8000000000000000; m != 0; m >>= 1) {
		if ((m & b) != 0)Result += *p;
		else if ((m & w) != 0)Result -= *p;
		p++;
	}
	return Result;
}

//着手可能数
int OthelloAI::PutAbleNum(BBoard b, BBoard w) {
	BBoard BList = GetSelectList(b, w);
	BBoard WList = GetSelectList(w, b);
	//自身と相手の着手可能数の差を計算する．
	return GetMyNum(BList) - GetMyNum(WList);
}

//確定石数える．
//端的に言うと，端の辺が一列揃ってるとその石全て確定石．
//一列揃ってない場合，端から順に数えて，同じ色が連続してたらそれが確定石．連続が終わったら確定石じゃなくなる．

//●●●○●空○空　の場合
//確確確ーーーーー
//になる．これだけの探索でも十分強い．

//大体辺の確定石だけ考えても強いからそれでも良いかなと思う．
int OthelloAI::CountKakutei(BBoard b, BBoard w) {
	//確定石を探索する．
	BBoard Tmp = GetKakutei(b, w);
	//自身と相手の確定石の差を計算する．
	Value1 = GetMyNum(b & Tmp);
	Value2 = GetMyNum(w & Tmp);
	return Value1 - Value2;
}

//開放度（少ないほうが良い）
//基本的に自身の石の周りに空欄がある個数を数えている．
//これも自身と相手の差を計算している．
int OthelloAI::CalcKaihoudo(BBoard b, BBoard w) {
	BBoard tmp1 = (b | (b >> 1) | (b << 1) | (b >> 8) | (b << 8) | (b >> 7) | (b >> 9) | (b << 7) | (b << 9)) & (~(b | w));
	BBoard tmp2 = (w | (w >> 1) | (w << 1) | (w >> 8) | (w << 8) | (w >> 7) | (w >> 9) | (w << 7) | (w << 9)) & (~(b | w));
	//少ないほうが良いからマイナスで返す．
	return -(GetMyNum(tmp1) - GetMyNum(tmp2));
}

//山の判定．
//端の辺のみで考えている．
//空●●●●●●空
//のような形．
//この形は強いらしい．
int OthelloAI::Mount(BBoard b, BBoard w) {
	int Num = 0;
	if (((b && 0xFF00000000000000) ^ 0x7E00000000000000) == 0)Num++;
	if (((w && 0xFF00000000000000) ^ 0x7E00000000000000) == 0)Num--;
	if (((b && 0x00000000000000FF) ^ 0x000000000000007E) == 0)Num++;
	if (((w && 0x00000000000000FF) ^ 0x000000000000007E) == 0)Num--;
	if (((b && 0x0101010101010101) ^ 0x0001010101010100) == 0)Num++;
	if (((w && 0x0101010101010101) ^ 0x0001010101010100) == 0)Num--;
	if (((b && 0x8080808080808080) ^ 0x0080808080808000) == 0)Num++;
	if (((w && 0x8080808080808080) ^ 0x0080808080808000) == 0)Num--;
	return Num;
}

//ウィングの判定．
//端の辺のみで考えている．
//空●●●●●空空
//もしくは
//空空●●●●●空
//のような形
//自身と相手の個数の差をとっている．
//少ないほうが良い．
int OthelloAI::Wing(BBoard b, BBoard w) {
	int Num = 0;
	if (((b && 0xFF00000000000000) ^ 0x3E00000000000000) != 0)Num++;
	if (((w && 0xFF00000000000000) ^ 0x3E00000000000000) != 0)Num--;
	if (((b && 0xFF00000000000000) ^ 0x7C00000000000000) != 0)Num++;
	if (((w && 0xFF00000000000000) ^ 0x7C00000000000000) != 0)Num--;
	if (((b && 0x00000000000000FF) ^ 0x000000000000003E) != 0)Num++;
	if (((w && 0x00000000000000FF) ^ 0x000000000000003E) != 0)Num--;
	if (((b && 0x00000000000000FF) ^ 0x000000000000007C) != 0)Num++;
	if (((w && 0x00000000000000FF) ^ 0x000000000000007C) != 0)Num--;
	if (((b && 0x0101010101010101) ^ 0x0000010101010100) != 0)Num++;
	if (((w && 0x0101010101010101) ^ 0x0000010101010100) != 0)Num--;
	if (((b && 0x0101010101010101) ^ 0x0001010101010000) != 0)Num++;
	if (((w && 0x0101010101010101) ^ 0x0001010101010000) != 0)Num--;
	if (((b && 0x8080808080808080) ^ 0x0000808080808000) != 0)Num++;
	if (((w && 0x8080808080808080) ^ 0x0000808080808000) != 0)Num--;
	if (((b && 0x8080808080808080) ^ 0x0080808080800000) != 0)Num++;
	if (((w && 0x8080808080808080) ^ 0x0080808080800000) != 0)Num--;
	return Num;
}