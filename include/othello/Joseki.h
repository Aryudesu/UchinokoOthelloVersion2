#pragma once
#include <vector>
#include <string>
typedef std::vector<std::vector<std::vector<int>>> PList;
typedef std::vector<std::string> NList;
typedef std::vector<std::vector<std::string>> JList;
typedef unsigned long long BBoard;
class Joseki {
private:
	PList JosekiList;				//定石一覧
	NList JName;				//定石名一覧
	bool SJosekiF;				//定石一覧に現在の手が存在するか
	std::vector<bool> JosekiF;	//定石になる可能性有無
	int RJoseki;
	int JosekiNum;
	JList GetJosekiFu();
	BBoard XY2BB(int x, int y);
	bool BB2XY(BBoard B, int& x, int& y);

public:
	void SearchJoseki(BBoard b,int PutCount);	//定石検索
	void GetPList();								//定石一覧持ってくる
	bool GetFlag();									//定石探索フラグ
	BBoard GetRandJoseki(int PutCount);
	Joseki();
	int GetJosekiNum();
	std::string GetJosekiName(int Num);
};