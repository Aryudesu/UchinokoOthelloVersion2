#pragma once
#include "core/Singleton.h"
#include "Dxlib.h"
#include <vector>
#include <string>


class SoundManager : public Singleton<SoundManager> {
	std::vector<int> SE;
	std::vector<int> BGM;
	int Volume;
	int SEVolume;
	int VolumeCount;
	int SEVolumeCount;
	int DrawCount;
	int SoftSoundHandle;
public:
	SoundManager();
	void SetSE(int Num, std::string FileName);
	void PlaySE(int Num);
	void SetBGM(int Num, int LoopPoint, std::string FileName);
	int  SetSSBGM(int Num, int LoopPoint, std::string FileName);
	void PlayBGM(int Num);
	void PlaySSBGM(int Num);
	void StopBGM(int Num);
	void StopSSBGM(int Num);
	void DeleteBGM(int Num);
	void DeleteSSBGM(int Num);
	void ChangeVolume(int V);
	void ChangeBGMVolume(int V);
	void ConfBGMVolume();
	void ConfVolume();
	void Draw();
	int  GetSoftSoundHandle();
	int  GetSSBGMHandle();
	int  GetVolume();
};