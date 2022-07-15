#pragma once
#include "stdafx.h"

class UISystem;
class ParticleSystem;
class CMarkup;

class CXMLReader
{
public:
	static bool GetUISetting(const string& file_name, UISystem* ui);
	static void LoadParticle(const wstring& file_name, ParticleSystem* sys);
private:
	static void GetNumberUI(CMarkup& xml, UISystem* ui);
	static const map<wstring, int*>&  GetVariable_map();
	static map<wstring, int*> variable_map;
};

