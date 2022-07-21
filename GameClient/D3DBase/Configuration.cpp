#include "Configuration.h"
#include "XMLReader.h"


float CConfiguration::s;
std::vector<ModelInfo> CConfiguration::player_models;
std::array<std::string, 5> CConfiguration::player_anims;
float CConfiguration::bottom = -580.0f;

void CConfiguration::Init()
{
	CXMLReader::LoadPlayerModelName(L"Resources/Model/PlayerModel.xml");
	//make_dir();
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
	
}

void CConfiguration::SetModel(const string& model, const wstring& tex)
{
	player_models.push_back({ model, tex });
}