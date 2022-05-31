//-----------------------------------------------------------------------------
// File: CGameObject.cpp
//-----------------------------------------------------------------------------

#include "Mesh.h"

CMesh::CMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
}

CMesh::~CMesh()
{
	if (m_pd3dVertexBuffer) m_pd3dVertexBuffer->Release();
	if (m_pd3dIndexBuffer) m_pd3dIndexBuffer->Release();
	if (m_pd3dVertexUploadBuffer) m_pd3dVertexUploadBuffer->Release();
	if (m_pd3dIndexUploadBuffer) m_pd3dIndexUploadBuffer->Release();
}

void CMesh::ReleaseUploadBuffers()
{
	if (m_pd3dVertexUploadBuffer) m_pd3dVertexUploadBuffer->Release();
	if (m_pd3dIndexUploadBuffer) m_pd3dIndexUploadBuffer->Release();
	m_pd3dVertexUploadBuffer = NULL;
	m_pd3dIndexUploadBuffer = NULL;
};

void CMesh::Render(ID3D12GraphicsCommandList* pd3dCommandList) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 1, &m_d3dVertexBufferView);
	if (m_pd3dIndexBuffer)
	{
		pd3dCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(m_nVertices, 1, m_nOffset, 0);
	}
}


CLoadedMesh::CLoadedMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::vector<CTexturedVertex>& v) : CMesh(pd3dDevice, pd3dCommandList)
{
	m_nVertices = (UINT)v.size();
	m_nStride = sizeof(CTexturedVertex);
	m_nOffset = 0;
	m_nSlot = 0;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_pd3dVertexBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, &(v[0]), m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);
	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;
}

CLoadedMesh::CLoadedMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::vector<CAnimationVertex>& v, const std::wstring& tex_dir) : CMesh(pd3dDevice, pd3dCommandList)
{
	m_nVertices = (UINT)v.size();
	m_nStride = sizeof(CAnimationVertex);
	m_nOffset = 0;
	m_nSlot = 0;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_pd3dVertexBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, &(v[0]), m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);

	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;
	texture_dir = tex_dir;
}

CLoadedMesh::CLoadedMesh(CLoadedMesh&& other)
{
	m_pd3dVertexBuffer = other.m_pd3dVertexBuffer;
	m_pd3dVertexUploadBuffer = other.m_pd3dVertexUploadBuffer;

	m_pd3dIndexBuffer = other.m_pd3dIndexBuffer;
	m_pd3dIndexUploadBuffer = other.m_pd3dIndexUploadBuffer;

	m_d3dVertexBufferView = other.m_d3dVertexBufferView;
	m_d3dIndexBufferView = other.m_d3dIndexBufferView;

	m_d3dPrimitiveTopology = other.m_d3dPrimitiveTopology;
	m_nSlot = other.m_nSlot;
	m_nVertices = other.m_nVertices;
	m_nStride = other.m_nStride;
	m_nOffset = other.m_nOffset;

	m_nIndices = other.m_nIndices;
	m_nStartIndex = other.m_nStartIndex;
	m_nBaseVertex = other.m_nBaseVertex;



	other.m_pd3dVertexBuffer = NULL;
	other.m_pd3dVertexUploadBuffer = NULL;

	other.m_pd3dIndexBuffer = NULL;
	other.m_pd3dIndexUploadBuffer = NULL;

	//other.m_d3dVertexBufferView;
	//other.m_d3dIndexBufferView;
	texture_dir = other.texture_dir;
}

CLoadedMesh::~CLoadedMesh()
{

}

DebugMesh::DebugMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::vector<DebugVertex>& v) : CMesh(pd3dDevice, pd3dCommandList)
{
	m_nVertices = (UINT)v.size();
	m_nStride = sizeof(DebugVertex);
	m_nOffset = 0;
	m_nSlot = 0;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_pd3dVertexBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, &(v[0]), m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);
	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;
}