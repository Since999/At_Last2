#include "TexturePool.h"
#include "Object.h"
#include "Configuration.h"
CTexturePool* CTexturePool::singleton;

CTexturePool* CTexturePool::GetInstance() {
	if (singleton) return singleton;

	singleton = new CTexturePool();
	return singleton;
}

CTexturePool::~CTexturePool(){
	for (auto tex : texture_map) {
		delete tex.second;
	}
	if (descriptor_heap) {
		descriptor_heap->Release();
	}
}

CTexture* CTexturePool::GetTexture(const std::wstring& texture_name) {
	auto found_texture = texture_map.find(texture_name);
	//if exist return
	if (found_texture != texture_map.end()) return found_texture->second;
	
	//else make one

	wstring path = texture_name;
	if (texture_name.find(L"/") == wstring::npos && texture_name.find(L"\\") == wstring::npos) {
		path = CConfiguration::MakePath(texture_name, TEXTURE_DIR);
	}

	CTexture* tex = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	tex->LoadTextureFromDDSFile(DEVICEMANAGER.pd3dDevice, DEVICEMANAGER.pd3dCommandList, path.c_str(), RESOURCE_TEXTURE2D, 0);
	CreateShaderResourceViews(DEVICEMANAGER.pd3dDevice, tex, 0, (int)ROOT_PARAMATER_INDEX::TEXTURE);

	texture_map.insert({ texture_name, tex });
	return tex;
}


//private--------------------------------------------------------------------------------------

CTexturePool::CTexturePool() {
	CreateDescriptorHeaps(DEVICEMANAGER.pd3dDevice, 100);
}

void CTexturePool::CreateDescriptorHeaps(ID3D12Device* pd3dDevice, int num)
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	d3dDescriptorHeapDesc.NumDescriptors = num; //
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&descriptor_heap);

	CPU_start = descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	GPU_start = descriptor_heap->GetGPUDescriptorHandleForHeapStart();

	CPU_next = CPU_start;
	GPU_next = GPU_start;
}

void CTexturePool::CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex)
{
	CPU_next.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);
	GPU_next.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);

	int nTextures = pTexture->GetTextures();
	for (int i = 0; i < nTextures; i++)
	{
		ID3D12Resource* pShaderResource = pTexture->GetResource(i);
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(i);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, CPU_next);
		CPU_next.ptr += ::gnCbvSrvDescriptorIncrementSize;
		pTexture->SetGpuDescriptorHandle(i, GPU_next);
		GPU_next.ptr += ::gnCbvSrvDescriptorIncrementSize;
	}
	int nRootParameters = pTexture->GetRootParameters();
	for (int i = 0; i < nRootParameters; i++) pTexture->SetRootParameterIndex(i, nRootParameterStartIndex + i);
}

void CTexturePool::SetDecriptorHeap(ID3D12GraphicsCommandList* cmd_list)
{
	cmd_list->SetDescriptorHeaps(1, &descriptor_heap);
}