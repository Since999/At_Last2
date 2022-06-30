#pragma once
#include "stdafx.h"

class UISystem;
class ParticleSystem;

class CXMLReader
{
public:
	static bool GetUISetting(const string& file_name, UISystem* ui);
	static CParticleBuilder LoadParticle(const wstring& file_name, ParticleSystem* sys);
private:

};

