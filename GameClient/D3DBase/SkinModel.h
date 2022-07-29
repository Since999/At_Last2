//====================================================================================
//		## SkinModel ## (하드웨어 스키닝 모델 클레스)
//====================================================================================
#pragma once

#include "Animation.h"
#include "Mesh.h"
#include "ModelStructure.h"
#include "AnimationBlend.h"
class CTexture;

class SkinModel
{
	wstring name;
	CMesh* mesh;
	CTexture* texture;
public:
	SkinModel();
	virtual ~SkinModel();

	void					Release();
	

	virtual array<XMMATRIX, 96>&		GetBoneMat(float& animation_time, float time_elapsed);
	virtual array<XMMATRIX, 96>&		GetBoneMat();
	virtual array<XMMATRIX, 96>&		GetBoneMat(float& animation_time, float time_elapsed, Animation* anim);
	virtual array<XMMATRIX, 96>&		GetBoneMatLastFrame(int ani_index = -1);

	void					Render(ID3D12GraphicsCommandList* commandList);
	void					ShadowRender(ID3D12GraphicsCommandList* commandList);

	void					SetTexture(CTexture* texture);
	void					SetName(const wstring& name) { this->name = name; }
	bool					CreateModel(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, Vertex& vertices, vector<unsigned long>& indices);
	
	CTexture* GetTexture() const { return texture; }

	//Animation Function
	void					PlayAni(int idx);
	void					StopAni()			{	_aniList[_playAniIdx].Stop();	}
	void					PauseAni()			{	_aniList[_playAniIdx].Pause();	}

	void					SetAniRepeat(unsigned int model_index, bool is_repeat);

	//when Completed Load
	virtual void					CompleteLoad();

	//Get Function
	vector<HierarchyMesh*>&	GetMeshList()		{	return _meshList;				}
	vector<NodeInfo*>&		GetNodeList()		{	return _nodeList;				}

	vector<wstring>&		GetMaterialList()	{	return _materialList;			}
	vector<Animation>&		GetAnimationList()	{	return _aniList;				}

public:
	bool UpdateNodeTM(float& time, float time_elapsed, int index = -1);
	bool UpdateNodeTM(float& time, float time_elapsed, Animation* anim);

protected:
	int						_playAniIdx		= -1;

	vector<wstring>		_materialList;
	vector<Animation>		_aniList;
	vector<NodeInfo*>		_nodeList;
	vector<HierarchyMesh*>	_meshList;
	array<XMMATRIX, 96>		calcBoneList;

	//마테리얼 단위 렌더링을 위한 변수, 편의상 메쉬 정보와 분리 해 둠.
	vector<vector<HierarchyMesh*>> _meshByMaterial;
};

class CBlendSkinModel : public SkinModel {
private:
	CPlayerBlend blend;
public:
	CBlendSkinModel();
	virtual void					CompleteLoad();
	const virtual array<XMMATRIX, 96>& GetBoneMat(const float& animation_time, float factor1, float factor2);
};
