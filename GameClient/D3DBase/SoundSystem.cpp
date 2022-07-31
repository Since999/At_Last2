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

#define MAX_CHANNEL 100
#define CHANNEL_VECTOR_SIZE 50

CSoundSystem::CSoundSystem()
{
	FMOD::System_Create(&system);

	system->init(MAX_CHANNEL, FMOD_INIT_NORMAL, NULL);
	CXMLReader::LoadSound(L"Resources/Sound/Sound.xml", this);
	Channel* channel = NULL;
	channel_pool.try_emplace(L"bgm", channel);
	channels.reserve(CHANNEL_VECTOR_SIZE);
	for (int i = 0; i < CHANNEL_VECTOR_SIZE; ++i) {
		channels.push_back(channel);
	}
	system->createChannelGroup("bgm", &bgm_channel_group);
	
	system->getMasterChannelGroup(&master_group);
	master_group->addGroup(bgm_channel_group);
	master_group->setVolume(0.1f);
	se_channel_group->setVolume(1.0f);
	//iter = channels.begin();
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

void CSoundSystem::Update()
{
	system->update();
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

	if (channels.size() <= channel_idx) {
		channel_idx = 0;
	}
	system->playSound(sound.sound, se_channel_group, false, &channels[channel_idx++]);
}

void CSoundSystem::PlayBGM(const wstring& name)
{
	StopBGM();
	auto& found = sound_pool.find(name);
		if (found == sound_pool.end()) {
	#ifdef _DEBUG
			wcout << "Error (CSoundSystem::Play): no such sound: " << name << endl;
	#endif
			return;
		}
		CSound& sound = (*found).second;
	system->playSound(sound.sound, bgm_channel_group, false, &bgm_channel);
}

void CSoundSystem::StopBGM()
{
	bgm_channel->stop();
}

//FMOD::Channel*& CSoundSystem::GetNextChannel()
//{
//	/*++iter;
//	if (iter == channels.end()) {
//		iter = channels.begin();
//	}
//	return *iter;*/
//	
//}