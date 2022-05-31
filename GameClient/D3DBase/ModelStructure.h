//==================================================================
//		## 모델에 필요한 구조체 정보 ## 
//==================================================================
#pragma once

//정점 정보
struct Vertex
{
	vector<XMFLOAT3> position;
	vector<XMFLOAT4> color;
	vector<XMFLOAT2> uv;
	vector<XMFLOAT3> normal;
	vector<XMFLOAT3> tangent;
	vector<XMFLOAT3> bitangent;
	vector<XMUINT4>	 boneidx;
	vector<XMFLOAT4> weight;
};

////재질 정보
//struct Material
//{
//	wstring name			= L"";
//
//	Texture* diffuseMap		= NULL;
//	Texture* alphaMap		= NULL;
//	Texture* normalMap		= NULL;
//	Texture* sepcularMap	= NULL;
//	Texture* lightMap		= NULL;
//};


//기본 메쉬, 인덱스 정보를 가짐.
struct BaseMesh {
	wstring	name	= L"None";		//Object Name Info

	UINT start		= 0;			//Object Start Idx or Vert Info
	UINT count		= 0;			//Object Idx or Vert Count Info

	UINT startVert	= 0;			//메쉬의 정점 정보가 버퍼의 몇번째 부터인지 표기.

	bool isDraw		= true;			//check to Render

};


//오브젝트의 위치 정보를 가진 노드
struct NodeInfo {
	int			depth	= 0;

	wstring		name	= L"none";
	NodeInfo*	parent = NULL;

	//TM info
	XMMATRIX localTM	= XMMatrixIdentity();
	XMMATRIX worldTM	= XMMatrixIdentity();

	NodeInfo(NodeInfo* parent, wstring& name, XMMATRIX& tm, int depth = 0) 
		: parent(parent), name(name), depth(depth), localTM(tm), worldTM(tm) 
	{
		//부모노드가 있으면 월드 행렬 변경
		if (parent)
			this->worldTM = localTM * parent->worldTM;
	}
};


//Offset Info
struct BoneInfo {
	NodeInfo*	linkNode	= NULL;
	XMMATRIX	matOffset	= XMMatrixIdentity();
};


//계층 정보를 가진 메쉬
struct HierarchyMesh : public BaseMesh {
	int matIdx = -1;
	NodeInfo* linkNode = NULL;
	vector<BoneInfo> boneList;

	HierarchyMesh(wstring name, int start, int count, int matIdx, int startVert)
	{
		if (name != L"")
			this->name = name;

		this->start = start, this->count = count, this->matIdx = matIdx, this->startVert =startVert;
	};
};

