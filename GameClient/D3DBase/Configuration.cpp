#include "Configuration.h"



float CConfiguration::s;
std::array<std::string, 3> CConfiguration::player_models;
std::array<std::wstring, 3> CConfiguration::player_textures;
std::array<std::string, 5> CConfiguration::player_anims;
float CConfiguration::bottom = -580.0f;

void CConfiguration::Init()
{
	ifstream in{ CONFIG_FILE_NAME };
	for (int i = 0; i < 3; ++i) {
		in >> player_models[i];
		std::string tmp;
		in >> tmp;
		player_textures[i] = std::wstring{tmp.begin(), tmp.end()};
	}

	for (auto& anim : player_anims) {
		in >> anim;
	}
	make_dir();
}

string CConfiguration::MakePath(const string& file_name, const string& path)
{
	string result = file_name;
	result.insert(0, path);
	return result;
}

wstring CConfiguration::MakePath(const wstring& file_name, const wstring& path)
{
	wstring result = file_name;
	result.insert(0, path);
	return result;
}

void CConfiguration::make_dir()
{
	for (int i = 0; i < 3; ++i) {
		player_models[i].insert(0, MODEL_DIR);
		//player_textures[i].insert(0, TEXTURE_DIR);
	}
	/*for (auto& anim : player_anims) {
		anim.insert(0, ANIMATION_DIR);
	}*/
}