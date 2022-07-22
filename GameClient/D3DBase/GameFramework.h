#pragma once

#define FRAME_BUFFER_WIDTH		1280
#define FRAME_BUFFER_HEIGHT		720

#include "Timer.h"
#include "Player.h"
#include "Scene.h"

class ShadowMap;
class CSunLight;
class UISystem;
class ParticleSystem;

class CGameFramework
{
private:
	static CGameFramework* singleton;
	CGameFramework();
	
public:
	static CGameFramework* GetInstance();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();

	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();

	void CreateRtvAndDsvDescriptorHeaps();

	void CreateRenderTargetViews();
	void CreateDepthStencilView();

	void ChangeSwapChainState();

	void CreateShadowMap();

	void BuildObjects();
	void ReleaseObjects();

	void ProcessInput();
	void AnimateObjects();
	void UpdateShaderVariables();
	void FrameAdvance();
	void ShadowMapRender();
	void Render();

	void WaitForGpuComplete();
	void MoveToNextFrame();

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	CScene* GetCurruntScene() const { return m_pScene; }

	CCamera* GetCamera() const { return m_pCamera; }

	void ChangeScene(CScene* scene);
	void ChangeUI(UISystem* ui_sys);

	void StartEvent();
private:
	HINSTANCE					m_hInstance;
	HWND						m_hWnd;

	int							m_nWndClientWidth;
	int							m_nWndClientHeight;

	IDXGIFactory4* m_pdxgiFactory = NULL;
	IDXGISwapChain3* m_pdxgiSwapChain = NULL;
	ID3D12Device* m_pd3dDevice = NULL;

	bool						m_bMsaa4xEnable = false;
	UINT						m_nMsaa4xQualityLevels = 0;

	static const UINT			m_nSwapChainBuffers = 2;
	UINT						m_nSwapChainBufferIndex;

	ID3D12Resource* m_ppd3dSwapChainBackBuffers[m_nSwapChainBuffers];
	ID3D12DescriptorHeap* m_pd3dRtvDescriptorHeap = NULL;
	UINT						m_nRtvDescriptorIncrementSize;

	ID3D12Resource* m_pd3dDepthStencilBuffer = NULL;
	ID3D12DescriptorHeap* m_pd3dDsvDescriptorHeap = NULL;
	UINT						m_nDsvDescriptorIncrementSize;

	ID3D12CommandAllocator* m_pd3dCommandAllocator = NULL;
	ID3D12CommandQueue* m_pd3dCommandQueue = NULL;
	ID3D12GraphicsCommandList* m_pd3dCommandList = NULL;

	ID3D12Fence* m_pd3dFence = NULL;
	UINT64						m_nFenceValues[m_nSwapChainBuffers];
	HANDLE						m_hFenceEvent;

	unique_ptr<ShadowMap> m_ShadowMap;
	ID3D12DescriptorHeap* ShadowMapDsvDescriptorHeap = NULL;
	ComPtr<ID3D12DescriptorHeap> ShadowMapSrvDescriptorHeap = NULL;

	ID3D12DescriptorHeap* NullSrvDescriptorHeap;
	ID3D12Resource* cb_shadow_map = NULL;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;


#if defined(_DEBUG)
	ID3D12Debug* m_pd3dDebugController;
#endif

	CGameTimer					m_GameTimer;



	POINT						m_ptOldCursorPos;

	_TCHAR						m_pszFrameRate[50];
public:

	CScene* m_pScene = NULL;
	CCamera* m_pCamera = NULL;

	UISystem* ui_system = NULL;
	ParticleSystem* particle_system = NULL;
};

