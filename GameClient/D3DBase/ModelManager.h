#pragma once
#include "stdafx.h"
#include <map>
class SkinModel;
class CTexture;

enum MODEL {
	ORC = 0,
	ZOMBIE = 1
};

class ModelManager {
	static map<std::string, SkinModel*> models;
public:
	ModelManager();
	~ModelManager();

	static SkinModel* GetModel(const std::string& model_name, const std::wstring& texture_name);
	
	//void LoadCharacters();

	static std::string make_model_dir(const std::string& name);
};