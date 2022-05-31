#include "ModelManager.h"

#include "ModelLoader.h"
#include "Object.h"
#include "Configuration.h"
#include "TexturePool.h"

map<std::string, SkinModel*> ModelManager::models;

ModelManager::ModelManager() 
{
	
}

ModelManager::~ModelManager()
{
	for (auto model : models) { 
		if (model.second)delete model.second; 
	}
}

SkinModel* ModelManager::GetModel(const std::string& model_name, const std::wstring& texture_name)
{
	auto found = models.find(model_name);
	if (found != models.end()) return (*found).second;

	std::string path = make_model_dir(model_name);
	SkinModel* model = ModelLoader::LoadModel(path.c_str());

	CTexture* tex = CTexturePool::GetInstance()->GetTexture(texture_name);
	model->SetTexture(tex);
	models.emplace(model_name, model);
	model->PlayAni(0);
	return model;
}

std::string ModelManager::make_model_dir(const std::string& name)
{
	std::string result{ name };
	result.insert(0, MODEL_DIR);
	return result;
}


/*
void ModelManager::LoadCharacters()
{
	{
		auto tmp_model = ModelLoader::LoadModel("Resources/Model/PMC_Reload.fbx");
		if (true) {
		//if (tmp_model->GetMaterialList().empty()) {
			CTexture* tex = CTexturePool::GetInstance()->GetTexture(L"Resources/Texture/PMC.png");
			textures.push_back(tex);
			tmp_model->SetTexture(tex);
		}
		else {
			auto path = tmp_model->GetMaterialList()[0];
			
			CTexture* tex = CTexturePool::GetInstance()->GetTexture(path);
			textures.push_back(tex);
			tmp_model->SetTexture(tex);
		}
		models.insert(make_pair(ORC, tmp_model));
	}
	{
		auto tmp_model = ModelLoader::LoadModel("Resources/Model/PMC_Walk.fbx");
		if (true) {
		//if (tmp_model->GetMaterialList().empty()) {
			CTexture* tex = CTexturePool::GetInstance()->GetTexture(L"Resources/Texture/PMC.png");
			textures.push_back(tex);
			tmp_model->SetTexture(tex);
		}
		else {
			auto path = tmp_model->GetMaterialList()[0];
			CTexture* tex = CTexturePool::GetInstance()->GetTexture(path);
			textures.push_back(tex);
			tmp_model->SetTexture(tex);
		}
		models.insert(make_pair(ZOMBIE, tmp_model));
	}
}

*/