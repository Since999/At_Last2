//------------------------------------------------------- ----------------------
// File: Mesh.h
//-----------------------------------------------------------------------------

#pragma once

#include "stdafx.h"
class CFloat4 {
	XMFLOAT4 a;
public:
	CFloat4() { a = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); }
	CFloat4(XMFLOAT4& other) { a = other; }
	float& operator[] (unsigned int index) {

	}
	XMFLOAT4& Get() { return a; }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CVertex
{
public:
	XMFLOAT3						m_xmf3Position;
	XMFLOAT3						Normal;
public:
	CVertex() { m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f); }
	CVertex(XMFLOAT3 xmf3Position) { m_xmf3Position = xmf3Position; }
	~CVertex() { }
};

class CDiffusedVertex : public CVertex
{
public:
	XMFLOAT4						m_xmf4Diffuse;

public:
	CDiffusedVertex() { m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f); m_xmf4Diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); }
	CDiffusedVertex(float x, float y, float z, XMFLOAT4 xmf4Diffuse) { m_xmf3Position = XMFLOAT3(x, y, z); m_xmf4Diffuse = xmf4Diffuse; }
	CDiffusedVertex(XMFLOAT3 xmf3Position, XMFLOAT4 xmf4Diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)) { m_xmf3Position = xmf3Position; m_xmf4Diffuse = xmf4Diffuse; }
	~CDiffusedVertex() { }
};

class CTexturedVertex : public CVertex
{
public:
	XMFLOAT2						m_xmf2TexCoord;

public:
	CTexturedVertex() { m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f); m_xmf2TexCoord = XMFLOAT2(0.0f, 0.0f); }
	CTexturedVertex(float x, float y, float z, XMFLOAT2 xmf2TexCoord) { m_xmf3Position = XMFLOAT3(x, y, z); m_xmf2TexCoord = xmf2TexCoord; }
	CTexturedVertex(XMFLOAT3 xmf3Position, XMFLOAT2 xmf2TexCoord = XMFLOAT2(0.0f, 0.0f)) { m_xmf3Position = xmf3Position; m_xmf2TexCoord = xmf2TexCoord; }
	~CTexturedVertex() { }
};

class CAnimationVertex {
public:
	XMFLOAT3						m_xmf3Position;
	XMFLOAT3						Normal;
	XMFLOAT2						m_xmf2TexCoord;
	XMFLOAT4						boneWeights;
	XMUINT4							boneIndex;

	float GetWeight(unsigned int index) const {
		switch (index) {
		case(0):
			return boneWeights.x;
		case(1):
			return boneWeights.y;
		case (2):
			return boneWeights.z;
		case (3):
			return boneWeights.w;
		}
		return 0.0f;
	}
	int GetIndex(unsigned int index) const {
		switch (index) {
		case(0):
			return boneIndex.x;
		case(1):
			return boneIndex.y;
		case (2):
			return boneIndex.z;
		case (3):
			return boneIndex.w;
		}
	}

	void SetIndexWeight(unsigned int index, int index_value, float weight) {
		switch (index) {
		case(0):
			boneIndex.x = index_value;
			boneWeights.x = weight;
			break;
		case(1):
			boneIndex.y = index_value;
			boneWeights.y = weight;
			break;
		case (2):
			boneIndex.z = index_value;
			boneWeights.z = weight;
			break;
		case (3):
			boneIndex.w = index_value;
			boneWeights.w = weight;
			break;
		}
	}

	void SetWeight(unsigned int index, float v) {
		switch (index) {
		case(0):
			boneWeights.x = v;
			break;
		case(1):
			boneWeights.y = v;
			break;
		case (2):
			boneWeights.z = v;
			break;
		case (3):
			boneWeights.w = v;
			break;
		}
	}
	void SetIndex(unsigned int index, int v) {
		switch (index) {
		case(0):
			boneIndex.x = v;
			break;
		case(1):
			boneIndex.y = v;
			break;
		case (2):
			boneIndex.z = v;
			break;
		case (3):
			boneIndex.w = v;
			break;
		}
	}
};

class CAnimationVertex_1 : public CTexturedVertex
{
public:
	XMFLOAT4 boneWeights;
	XMFLOAT4 boneWeights1;
	XMUINT4 boneIndex;
	XMUINT4 boneIndex1;
	CAnimationVertex_1() {
		m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f); m_xmf2TexCoord = XMFLOAT2(0.0f, 0.0f);
		boneWeights = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);  boneIndex = XMUINT4(0, 0, 0, 0);
		boneWeights1 = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);  boneIndex1 = XMUINT4(0, 0, 0, 0);
	}
	float GetWeight(unsigned int index) const {
		switch (index) {
		case(0):
			return boneWeights.x;
		case(1):
			return boneWeights.y;
		case (2):
			return boneWeights.z;
		case (3):
			return boneWeights.w;
		case (4):
			return boneWeights1.x;
		case (5):
			return boneWeights1.y;
		case (6):
			return boneWeights1.z;
		case (7):
			return boneWeights1.w;
		}
		return 0.0f;
	}
	int GetIndex(unsigned int index) const {
		switch (index) {
		case(0):
			return boneIndex.x;
		case(1):
			return boneIndex.y;
		case (2):
			return boneIndex.z;
		case (3):
			return boneIndex.w;
		}
	}

	void SetIndexWeight(unsigned int index, int index_value, float weight) {
		switch (index) {
		case(0):
			boneIndex.x = index_value;
			boneWeights.x = weight;
			break;
		case(1):
			boneIndex.y = index_value;
			boneWeights.y = weight;
			break;
		case (2):
			boneIndex.z = index_value;
			boneWeights.z = weight;
			break;
		case (3):
			boneIndex.w = index_value;
			boneWeights.w = weight;
			break;
		case(4):
			boneIndex1.x = index_value;
			boneWeights1.x = weight;
			break;
		case(5):
			boneIndex1.y = index_value;
			boneWeights1.y = weight;
			break;
		case (6):
			boneIndex1.z = index_value;
			boneWeights1.z = weight;
			break;
		case (7):
			boneIndex1.w = index_value;
			boneWeights1.w = weight;
			break;
		}
	}

	void SetWeight(unsigned int index, float v) {
		switch (index) {
		case(0):
			boneWeights.x = v;
			break;
		case(1):
			boneWeights.y = v;
			break;
		case (2):
			boneWeights.z = v;
			break;
		case (3):
			boneWeights.w = v;
			break;
		}
	}
	void SetIndex(unsigned int index, int v) {
		switch (index) {
		case(0):
			boneIndex.x = v;
			break;
		case(1):
			boneIndex.y = v;
			break;
		case (2):
			boneIndex.z = v;
			break;
		case (3):
			boneIndex.w = v;
			break;
		}
	}
};

class CDiffusedTexturedVertex : public CDiffusedVertex
{
public:
	XMFLOAT2						m_xmf2TexCoord;

public:
	CDiffusedTexturedVertex() { m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f); m_xmf4Diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); m_xmf2TexCoord = XMFLOAT2(0.0f, 0.0f); }
	CDiffusedTexturedVertex(float x, float y, float z, XMFLOAT4 xmf4Diffuse, XMFLOAT2 xmf2TexCoord) { m_xmf3Position = XMFLOAT3(x, y, z); m_xmf4Diffuse = xmf4Diffuse; m_xmf2TexCoord = xmf2TexCoord; }
	CDiffusedTexturedVertex(XMFLOAT3 xmf3Position, XMFLOAT4 xmf4Diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), XMFLOAT2 xmf2TexCoord = XMFLOAT2(0.0f, 0.0f)) { m_xmf3Position = xmf3Position; m_xmf4Diffuse = xmf4Diffuse; m_xmf2TexCoord = xmf2TexCoord; }
	~CDiffusedTexturedVertex() { }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CMesh
{
public:
	CMesh() {}
	CMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~CMesh();

private:
	int								m_nReferences = 0;

public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

	void ReleaseUploadBuffers();

	const std::wstring GetTextureDir() { return texture_dir; }
protected:
	ID3D12Resource* m_pd3dVertexBuffer = NULL;
	ID3D12Resource* m_pd3dVertexUploadBuffer = NULL;

	ID3D12Resource* m_pd3dIndexBuffer = NULL;
	ID3D12Resource* m_pd3dIndexUploadBuffer = NULL;

	D3D12_VERTEX_BUFFER_VIEW		m_d3dVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW			m_d3dIndexBufferView;

	D3D12_PRIMITIVE_TOPOLOGY		m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UINT							m_nSlot = 0;
	UINT							m_nVertices = 0;
	UINT							m_nStride = 0;
	UINT							m_nOffset = 0;

	UINT							m_nIndices = 0;
	UINT							m_nStartIndex = 0;
	int								m_nBaseVertex = 0;
	std::wstring						texture_dir;
public:
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList) const;
};

class CLoadedMesh : public CMesh
{
public:
	CLoadedMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::vector<CTexturedVertex>& v);
	CLoadedMesh(CLoadedMesh&& other);
	CLoadedMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::vector<CAnimationVertex>& v, const std::wstring& tex_dir = {});
	virtual ~CLoadedMesh();
};

class DebugVertex {
public:
	XMFLOAT3						m_xmf3Position;
	XMFLOAT2						m_xmf2TexCoord;
};

class DebugMesh : public CMesh
{
public:
	DebugMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::vector<DebugVertex>& v);
};