#pragma once
#include "stdafx.h"
#include <map>
#include <fmod/fmod.hpp>
#include <fmod/fmod_errors.h>

class CSound {
public:
	FMOD::Sound* sound = NULL;
	wstring channel;
	CSound(FMOD::Sound* sound, const wstring& channel) :
		sound(sound), channel(channel){}
	~CSound() {  }
};

class CSoundSystem
{
private:
	static CSoundSystem* singleton;
public:
	static CSoundSystem* GetInstance();

private:
	FMOD::System* system;
	map<wstring, CSound> sound_pool;
	vector<FMOD::Channel*> channels;
	unsigned int channel_idx = 0;
	map<wstring, FMOD::Channel*> channel_pool;

	FMOD::Channel* bgm_channel = NULL;

	FMOD::ChannelGroup* bgm_channel_group = NULL;
	FMOD::ChannelGroup* se_channel_group = NULL;
	FMOD::ChannelGroup* master_group = NULL;
public:
	CSoundSystem();
	~CSoundSystem();
	void Update();
	void SetSound(const wstring& name, const string& file_name, const wstring& channel, bool loop = false);
	void Play(const wstring& name);
	void PlayBGM(const wstring& name);
	void StopBGM();
	
	int in = 0;
private:
	//FMOD::Channel*& GetNextChannel();
};

