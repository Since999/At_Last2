#pragma once
#include "stdafx.h"

class UISystem;
class ParticleSystem;
class CMarkup;
class CSoundSystem;
class CConfiguration;

class CXMLReader
{
public:
	static bool GetUISetting(const string& file_name, UISystem* ui);
	static void LoadParticle(const wstring& file_name, ParticleSystem* sys);
	static void LoadSound(const wstring& file_name, CSoundSystem* sys);
	static void LoadPlayerModelName(const wstring& file_name);
private:
	static void GetNumberUI(CMarkup& xml, UISystem* ui);
	static const map<wstring, int*>&  GetVariable_map();
	static map<wstring, int*> variable_map;
};

