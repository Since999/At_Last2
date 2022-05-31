#pragma once
#include "stdafx.h"
#include <map>

class CTexture;

class CTexturePool
{
private:
	static CTexturePool* singleton;

public:
	static CTexturePool* GetInstance();

private:
	CTexturePool();

public:
	void SetDecriptorHeap(ID3D12GraphicsCommandList* command_list);
	CTexture* GetTexture(const std::wstring& texture_name);
	~CTexturePool();
private:
	ID3D12DescriptorHeap* descriptor_heap = NULL;

	D3D12_CPU_DESCRIPTOR_HANDLE			CPU_start;
	D3D12_GPU_DESCRIPTOR_HANDLE			GPU_start;

	D3D12_CPU_DESCRIPTOR_HANDLE			CPU_next;
	D3D12_GPU_DESCRIPTOR_HANDLE			GPU_next;

	std::map<std::wstring, CTexture*>		texture_map;

private:
	void CreateDescriptorHeaps(ID3D12Device* pd3dDevice, int num);
	void CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex);
};

