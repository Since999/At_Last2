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
	map<wstring, FMOD::Channel*> channel_pool;

public:
	CSoundSystem();
	~CSoundSystem();
	void SetSound(const wstring& name, const string& file_name, const wstring& channel, bool loop = false);
	void Play(const wstring& name);
};

