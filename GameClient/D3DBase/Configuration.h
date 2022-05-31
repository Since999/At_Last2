#pragma once
#include "stdafx.h"
#include <map>

#define CONFIG_FILE_NAME "config.txt"
#define MODEL_DIR "Resources/Model/"
#define TEXTURE_DIR L"Resources/Texture/"
#define ANIMATION_DIR "Resources/Model/Animation/"

class CConfiguration
{
public:
	static float s;
	static std::array<std::string, 3> player_models;
	static std::array<std::wstring, 3> player_textures;
	static std::array<std::string, 5> player_anims;
	static float bottom;
public:
	static void Init();
	static string MakePath(const string& file_name, const string& path);
	static wstring MakePath(const wstring& file_name, const wstring& path);
private:
	static void make_dir();
	
};

