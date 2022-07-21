#pragma once
#include "stdafx.h"
#include <map>

#define CONFIG_FILE_NAME "config.txt"
#define MODEL_DIR "Resources/Model/"
#define TEXTURE_DIR L"Resources/Texture/"
#define ANIMATION_DIR "Resources/Model/Animation/"

struct ModelInfo {
	string model;
	wstring texture;
};

class CConfiguration
{
public:
	static float s;
	static std::vector<ModelInfo> player_models;
	static std::array<std::string, 5> player_anims;
	static float bottom;
public:
	static void Init();
	static string MakePath(const string& file_name, const string& path);
	static wstring MakePath(const wstring& file_name, const wstring& path);
	static void SetModel(const string& model, const wstring& tex);
private:
	static void make_dir();
	
};

