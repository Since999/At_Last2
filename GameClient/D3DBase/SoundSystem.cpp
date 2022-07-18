#include "SoundSystem.h"
#include "XMLReader.h"

using namespace FMOD;

CSoundSystem* CSoundSystem::singleton = NULL;

CSoundSystem* CSoundSystem::GetInstance()
{
	if (singleton) return singleton;
	
	singleton = new CSoundSystem();
	return singleton;
}

#define MAX_CHANNEL 10

CSoundSystem::CSoundSystem()
{
	FMOD::System_Create(&system);

	system->init(MAX_CHANNEL, FMOD_INIT_NORMAL, NULL);
	CXMLReader::LoadSound(L"Resources/Sound/Sound.xml", this);
	Channel* channel = NULL;
	channel_pool.try_emplace(L"bgm", channel);
}

CSoundSystem::~CSoundSystem()
{
	system->release();
}

void CSoundSystem::SetSound(const wstring& name, const string& file_name, const wstring& channel, bool loop)
{
	Channel* channel_ptr = NULL;
	channel_pool.try_emplace(channel, channel_ptr);
	Sound* sound;
	FMOD_MODE mode = FMOD_DEFAULT;
	if (loop) {
		mode = mode | FMOD_LOOP_NORMAL;
	}
	else {
		mode = mode | FMOD_LOOP_OFF;
	}
	system->createSound(file_name.c_str(), mode, NULL, &sound);
	sound_pool.emplace(name, CSound(sound, channel));
}

void CSoundSystem::Play(const wstring& name)
{
	auto& found = sound_pool.find(name);
	if (found == sound_pool.end()) {
#ifdef _DEBUG
		wcout << "Error (CSoundSystem::Play): no such sound: " << name << endl;
#endif
		return;
	}
	CSound& sound = (*found).second;
	auto& found_channel = channel_pool.find(sound.channel);

	system->playSound(sound.sound, 0, false, &(*found_channel).second);
	
}

void CSoundSystem::StopBGM()
{
	auto& found = channel_pool.find(L"bgm");
	if (found == channel_pool.end()) {
#ifdef _DEBUG
		wcout << "Error (CSoundSystem::stopBGM): no bgm channel " << endl;
#endif
		return;
	}
	(*found).second->stop();
}