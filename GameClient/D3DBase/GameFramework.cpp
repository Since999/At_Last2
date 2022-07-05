//-----------------------------------------------------------------------------
// File: CGameFramework.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "GameFramework.h"
#include "ShadowMap.h"
#include "CLight.h"
#include "Object.h"
#include "Configuration.h"

DebugObject dd;

float bottom = -500.0f;
#define PRINT_RATE 10.0f
float renderTime = 0.0f;

CGameFramework* CGameFramework::singleton;

CGameFramework* CGameFramework::GetInstance() 
{
	if (singleton) return singleton;
	
	singleton = new CGameFramework();
	return singleton;
}

CGameFramework::CGameFramework()
{
	m_pdxgiFactory = NULL;
	m_pdxgiSwapChain = NULL;
	m_pd3dDevice = NULL;

	for (int i = 0; i < m_nSwapChainBuffers; i++) m_ppd3dSwapChainBackBuffers[i] = NULL;
	m_nSwapChainBufferIndex = 0;

	m_pd3dCommandAllocator = NULL;
	m_pd3dCommandQueue = NULL;
	m_pd3dCommandList = NULL;

	m_pd3dRtvDescriptorHeap = NULL;
	m_pd3dDsvDescriptorHeap = NULL;

	m_nRtvDescriptorIncrementSize = 0;
	m_nDsvDescriptorIncrementSize = 0;

	m_hFenceEvent = NULL;
	m_pd3dFence = NULL;
	for (int i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_nWndClientWidth = FRAME_BUFFER_WIDTH;
	m_nWndClientHeight = FRAME_BUFFER_HEIGHT;

	m_pScene = NULL;
	m_pPlayer = NULL;

	_tcscpy_s(m_pszFrameRate, _T("At Last... ("));
}

CGameFramework::~CGameFramework()
{
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	DEVICEMANAGER.pd3dCommandList = m_pd3dCommandList;
	DEVICEMANAGER.pd3dDevice = m_pd3dDevice;
	CreateRtvAndDsvDescriptorHeaps();

	CreateShadowMap();

	CreateSwapChain();
#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
	CreateRenderTargetViews();
#endif
	CreateDepthStencilView();

	BuildObjects();

	return(true);
}

void CGameFramework::CreateShadowMap()
{
	m_ShadowMap = std::make_unique<ShadowMap>(m_pd3dDevice, 2048, 2048);

	//make shadow srv
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&ShadowMapSrvDescriptorHeap)));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	//m_pd3dDevice->CreateShaderResourceView(nullptr, &srvDesc, ShadowMapSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto mCbvSrvUavDescriptorSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	ThrowIfFailed(m_pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&NullSrvDescriptorHeap)));
	
	auto nullSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(NullSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, mCbvSrvUavDescriptorSize);
	mNullSrv = CD3DX12_GPU_DESCRIPTOR_HANDLE(NullSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), 0, mCbvSrvUavDescriptorSize);
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	m_pd3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

	//make shadow dsv
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	m_pd3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&ShadowMapDsvDescriptorHeap));

	//make shadow ~~
	m_ShadowMap->BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE(ShadowMapSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()), 
		CD3DX12_GPU_DESCRIPTOR_HANDLE(ShadowMapSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(ShadowMapDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()));
}

//#define _WITH_SWAPCHAIN

void CGameFramework::CreateSwapChain()
{
	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);
	m_nWndClientWidth = rcClient.right - rcClient.left;
	m_nWndClientHeight = rcClient.bottom - rcClient.top;

#ifdef _WITH_CREATE_SWAPCHAIN_FOR_HWND
	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	dxgiSwapChainDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.Scaling = DXGI_SCALING_NONE;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS
	dxgiSwapChainDesc.Flags = 0;
#else
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
#endif

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapChainFullScreenDesc;
	::ZeroMemory(&dxgiSwapChainFullScreenDesc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
	dxgiSwapChainFullScreenDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainFullScreenDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Windowed = TRUE;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue, m_hWnd, &dxgiSwapChainDesc, &dxgiSwapChainFullScreenDesc, NULL, (IDXGISwapChain1**)&m_pdxgiSwapChain);
#else
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.BufferDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.BufferDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.OutputWindow = m_hWnd;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.Windowed = TRUE;
#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS
	dxgiSwapChainDesc.Flags = 0;
#else
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
#endif

	HRESULT hResult = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue, &dxgiSwapChainDesc, (IDXGISwapChain**)&m_pdxgiSwapChain);
#endif

	if (!m_pdxgiSwapChain)
	{
		MessageBox(NULL, L"Swap Chain Cannot be Created.", L"Error", MB_OK);
		::PostQuitMessage(0);
		return;
	}

	hResult = m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
}

void CGameFramework::CreateDirect3DDevice()
{
	HRESULT hResult;

	UINT nDXGIFactoryFlags = 0;
#if defined(_DEBUG)
	ID3D12Debug* pd3dDebugController = NULL;
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&pd3dDebugController);
	if (pd3dDebugController)
	{
		pd3dDebugController->EnableDebugLayer();
		pd3dDebugController->Release();
	}
	nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	hResult = ::CreateDXGIFactory2(nDXGIFactoryFlags, __uuidof(IDXGIFactory4), (void**)&m_pdxgiFactory);

	IDXGIAdapter1* pd3dAdapter = NULL;

	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); i++)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void**)&m_pd3dDevice))) break;
	}

	if (!m_pd3dDevice)
	{
		hResult = m_pdxgiFactory->EnumWarpAdapter(_uuidof(IDXGIAdapter1), (void**)&pd3dAdapter);
		hResult = D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), (void**)&m_pd3dDevice);
	}

	if (!m_pd3dDevice)
	{
		MessageBox(NULL, L"Direct3D 12 Device Cannot be Created.", L"Error", MB_OK);
		::PostQuitMessage(0);
		return;
	}

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	hResult = m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;

	hResult = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_pd3dFence);
	for (UINT i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 1;
	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	::gnCbvSrvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (pd3dAdapter) pd3dAdapter->Release();
}

void CGameFramework::CreateCommandQueueAndList()
{
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, _uuidof(ID3D12CommandQueue), (void**)&m_pd3dCommandQueue);

	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_pd3dCommandAllocator);

	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_pd3dCommandList);
	hResult = m_pd3dCommandList->Close();
}

void CGameFramework::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dRtvDescriptorHeap);
	m_nRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dDsvDescriptorHeap);
	m_nDsvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CGameFramework::CreateRenderTargetViews()
{
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < m_nSwapChainBuffers; i++)
	{
		m_pdxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&m_ppd3dSwapChainBackBuffers[i]);
		m_pd3dDevice->CreateRenderTargetView(m_ppd3dSwapChainBackBuffers[i], NULL, d3dRtvCPUDescriptorHandle);
		d3dRtvCPUDescriptorHandle.ptr += m_nRtvDescriptorIncrementSize;
	}
}

void CGameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue, __uuidof(ID3D12Resource), (void**)&m_pd3dDepthStencilBuffer);

	D3D12_DEPTH_STENCIL_VIEW_DESC d3dDepthStencilViewDesc;
	::ZeroMemory(&d3dDepthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
	d3dDepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dDepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	d3dDepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, NULL, d3dDsvCPUDescriptorHandle);
	//	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, &d3dDepthStencilViewDesc, d3dDsvCPUDescriptorHandle);
}

void CGameFramework::ChangeSwapChainState()
{
	WaitForGpuComplete();

	BOOL bFullScreenState = FALSE;
	m_pdxgiSwapChain->GetFullscreenState(&bFullScreenState, NULL);
	m_pdxgiSwapChain->SetFullscreenState(!bFullScreenState, NULL);

	DXGI_MODE_DESC dxgiTargetParameters;
	dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiTargetParameters.Width = m_nWndClientWidth;
	dxgiTargetParameters.Height = m_nWndClientHeight;
	dxgiTargetParameters.RefreshRate.Numerator = 60;
	dxgiTargetParameters.RefreshRate.Denominator = 1;
	dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	m_pdxgiSwapChain->ResizeTarget(&dxgiTargetParameters);

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_ppd3dSwapChainBackBuffers[i]) m_ppd3dSwapChainBackBuffers[i]->Release();
#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
#else
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(m_nSwapChainBuffers, m_nWndClientWidth, m_nWndClientHeight, dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);
#endif
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	CreateRenderTargetViews();
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_pScene) m_pScene->OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		::SetCapture(hWnd);
		::GetCursorPos(&m_ptOldCursorPos);
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		::ReleaseCapture();
		break;
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}
}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_pScene) m_pScene->OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
	switch (nMessageID)
	{
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_ESCAPE:
			::PostQuitMessage(0);
			break;
		case VK_RETURN:
			break;
		case VK_F1:
		case VK_F2:
		case VK_F3:
			//m_pCamera = m_pPlayer->ChangeCamera((DWORD)(wParam - VK_F1 + 1), m_GameTimer.GetTimeElapsed());
			break;
		case VK_F11:
			ChangeSwapChainState();
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) == WA_INACTIVE)
			m_GameTimer.Stop();
		else
			m_GameTimer.Start();
		break;
	}
	case WM_SIZE:
		break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
		OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
		break;
	}
	return(0);
}

void CGameFramework::OnDestroy()
{
	ReleaseObjects();

	::CloseHandle(m_hFenceEvent);

#if defined(_DEBUG)
	if (m_pd3dDebugController) m_pd3dDebugController->Release();
#endif

	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer->Release();
	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap->Release();

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_ppd3dSwapChainBackBuffers[i]) m_ppd3dSwapChainBackBuffers[i]->Release();
	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap->Release();

	if (m_pd3dCommandAllocator) m_pd3dCommandAllocator->Release();
	if (m_pd3dCommandQueue) m_pd3dCommandQueue->Release();
	if (m_pd3dCommandList) m_pd3dCommandList->Release();

	if (m_pd3dFence) m_pd3dFence->Release();

	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);
	if (m_pdxgiSwapChain) m_pdxgiSwapChain->Release();
	if (m_pd3dDevice) m_pd3dDevice->Release();
	if (m_pdxgiFactory) m_pdxgiFactory->Release();
}
#include "2DShader.h"
void CGameFramework::BuildObjects()
{
	m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	CConfiguration::Init();

	m_pScene = new CScene();
	m_pScene->BuildObjects(m_pd3dDevice, m_pd3dCommandList);
	ui_system = new UISystem(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
	particle_system = ParticleSystem::InitInstance(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());

	m_pScene->m_pPlayer = m_pPlayer = new CMainGamePlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), NULL, 10, 
		CConfiguration::player_models[0], CConfiguration::player_textures[0].c_str());
	m_pScene->m_pPlayer2 = m_pPlayer2 = new CMainGamePlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), NULL, 10, 
		CConfiguration::player_models[1], CConfiguration::player_textures[1].c_str());
	m_pScene->m_pPlayer3 = m_pPlayer3 = new CMainGamePlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), NULL, 10, 
		CConfiguration::player_models[2], CConfiguration::player_textures[2].c_str());

	bottom = -580.0;
#ifdef ENABLE_NETWORK
	m_pPlayer->SetPosition({ (network.g_client[network.my_id].Get_Client_X() - 550.0f)*(-100.0f), 00.0f, (network.g_client[network.my_id].Get_Client_Z() - 210.0f)*(-100.0f)});

	m_pPlayer->SetPosition({ 50500.0f, bottom, 14000.0f });
	m_pPlayer2->SetPosition({ (network.g_client[network.other_client_id1].Get_Client_X() - 550.0f)* (-100.0f), 00.0f, (network.g_client[network.other_client_id1].Get_Client_Z() - 210.0f)* (-100.0f) });
	m_pPlayer3->SetPosition({ (network.g_client[network.other_client_id2].Get_Client_X() - 550.0f) * (-100.0f), 00.0f, (network.g_client[network.other_client_id2].Get_Client_Z() - 210.0f) * (-100.0f) });
	
	client_player = m_pPlayer;
#else
	m_pPlayer->SetPosition({ 50500.0f, bottom, 14000.0f });
	m_pPlayer2->SetPosition({ 50500.0f, bottom, 14000.0f });
	m_pPlayer3->SetPosition({ 50500.0f, bottom, 14000.0f });
	client_player = m_pPlayer;
#endif
	((CMainGamePlayer*)m_pPlayer)->SetPlayerInfo(&network.g_client[network.my_id]);
	((CMainGamePlayer*)m_pPlayer2)->SetPlayerInfo(&network.g_client[network.other_client_id1]);
	((CMainGamePlayer*)m_pPlayer3)->SetPlayerInfo(&network.g_client[network.other_client_id2]);

	client_player->ChangeCamera(THIRD_PERSON_CAMERA, 0.0f);
	m_pCamera = client_player->GetCamera();

	m_pCamera->CreateShaderVariables(m_pd3dDevice, m_pd3dCommandList);

	sun_light = new CSunLight(m_pPlayer);
#ifdef SHADOW_TEXTURE_RENDER
	vector<DebugVertex> dv;
	DebugVertex tmp;
	float z = 0.9f;
	float size = 0.5f;
	float one = 1.0f;
	tmp.m_xmf3Position = { one, one, z };
	tmp.m_xmf2TexCoord = { one, 0.0f };
	dv.push_back(tmp);
	tmp.m_xmf3Position = { one, size, z };
	tmp.m_xmf2TexCoord = { 1.0f, 1.0f };
	dv.push_back(tmp);
	tmp.m_xmf3Position = { size, size, z };
	tmp.m_xmf2TexCoord = { 0.0f, 1.0f };
	dv.push_back(tmp);

	tmp.m_xmf3Position = { size, size, z };
	tmp.m_xmf2TexCoord = { 0.0f, 1.0f };
	dv.push_back(tmp);
	tmp.m_xmf3Position = { size, one, z };
	tmp.m_xmf2TexCoord = { 0.0f, 0.0f };
	dv.push_back(tmp);
	tmp.m_xmf3Position = { one, one, z };
	tmp.m_xmf2TexCoord = { 1.0f, 0.0f };
	dv.push_back(tmp);

	DebugMesh* dm = new DebugMesh(m_pd3dDevice, m_pd3dCommandList, dv);

	dd.SetMesh(0, dm);
	auto t = new ShadowMapDebugShader();
	t->CreateShader(m_pd3dDevice, m_pScene->GetGraphicsRootSignature());
	dd.SetShader(t);
#endif

	m_pd3dCommandList->Close();
	ID3D12CommandList *ppd3dCommandLists[] ={ m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();
	if (m_pScene) m_pScene->ReleaseUploadBuffers();

	m_GameTimer.Reset();
}

void CGameFramework::ReleaseObjects()
{
	if (m_pPlayer) delete m_pPlayer;
	if (m_pPlayer2) delete m_pPlayer2;
	if (m_pPlayer3) delete m_pPlayer3;

	if (sun_light) delete sun_light;

	if (m_pScene) m_pScene->ReleaseObjects();
	if (m_pScene) delete m_pScene;
}

void CGameFramework::ProcessInput()
{
	static UCHAR pKeysBuffer[256];
	bool bProcessedByScene = false;
	if (GetKeyboardState(pKeysBuffer) && m_pScene) bProcessedByScene = m_pScene->ProcessInput(pKeysBuffer);
	if (!bProcessedByScene)
	{
		DWORD dwDirection = 0;

		if ((pKeysBuffer['W'] & 0xF0)) dwDirection |= DIR_FORWARD;
		if ((pKeysBuffer['S'] & 0xF0)) dwDirection |= DIR_BACKWARD;
		if ((pKeysBuffer['A'] & 0xF0)) dwDirection |= DIR_LEFT;
		if ((pKeysBuffer['D'] & 0xF0)) dwDirection |= DIR_RIGHT;
		if (pKeysBuffer[VK_PRIOR] & 0xF0) dwDirection |= DIR_UP;
		if (pKeysBuffer[VK_NEXT] & 0xF0) dwDirection |= DIR_DOWN;

		if (pKeysBuffer[VK_OEM_PLUS] & 0xF0 || pKeysBuffer[VK_ADD] & 0xF0) {
			if (m_pCamera) m_pCamera->AddDistance(50.0f);
		}
		if (pKeysBuffer[VK_OEM_MINUS] & 0xF0 || pKeysBuffer[VK_SUBTRACT] & 0xF0) {
			if (m_pCamera) m_pCamera->AddDistance(-50.0f);
		}
#ifdef ENABLE_NETWORK
		if ((pKeysBuffer['E'] & 0xF0) && network.key_down_state == false) {
			network.key_down_state = true;
			network.Send_request_packet(MsgType::CS_PLAYER_INTERATION);
		}
		// 채팅용 키 엔터 또는 등등을 만들어야 할 수도 있음 현재는 만들지 않음

		if ((pKeysBuffer['R'] & 0xF0) && network.key_down_state == false) {
			network.key_down_state = true;
			network.Send_request_packet(MsgType::CS_PLAYER_RELOAD_REQUEST);
		}

		if ((pKeysBuffer[VK_LSHIFT] & 0XF0) && network.key_down_state == false)
		{
			network.key_down_state = true;
			network.Send_request_packet(MsgType::CS_PLAYER_SPECIAL);
		}

		if ((pKeysBuffer['C'] & 0xF0) && network.g_client[network.my_id].special_skill_key == true)
		{
			network.g_client[network.my_id].special_skill_key = false;
			network.Send_commander_special_req_packet(network.g_client[network.my_id].special_id);
		}

		if ((pKeysBuffer['V'] & 0xF0) && network.g_client[network.my_id].special_skill_key == true)
		{
			network.g_client[network.my_id].special_skill_key = false;
			network.Send_commander_special_req_packet(network.g_client[network.my_id].special_id);
		}

#endif

		float cxDelta = 0.0f, cyDelta = 0.0f;
		POINT ptCursorPos;
		//if (GetCapture() == m_hWnd)
		{
			//SetCursor(NULL);
			GetCursorPos(&ptCursorPos);
			ScreenToClient(m_hWnd, &ptCursorPos);
			//cxDelta = (float)(ptCursorPos.x - m_ptOldCursorPos.x) / 3.0f;
			//cyDelta = (float)(ptCursorPos.y - m_ptOldCursorPos.y) / 3.0f;
			//SetCursorPos(m_ptOldCursorPos.x, m_ptOldCursorPos.y);
			RECT rect;
			GetClientRect(m_hWnd, &rect);
			int width = rect.right - rect.left;
			int height = rect.bottom - rect.top;
			XMVECTOR cursor_direction = { ptCursorPos.x - (width / 2), ptCursorPos.y - (height / 2) };
			cursor_direction = XMVector2Normalize(cursor_direction);
			network.g_client[network.my_id].mx = cursor_direction.m128_f32[0];
			network.g_client[network.my_id].mz = cursor_direction.m128_f32[1];
		}

#ifdef ENABLE_NETWORK
		if (network.mouse_state == false)
		{
			network.mouse_state = true;
			network.Send_rotate_packet(network.g_client[network.my_id].mx, network.g_client[network.my_id].mz);
		}

		if (GetCapture() == m_hWnd && network.attack_state == false)
		{
			network.attack_state = true;
			int m_x = ptCursorPos.x - 640;
			int m_z = ptCursorPos.y - 360;
			network.Send_attack_packet(m_x, m_z);
		}
#endif
		if ((dwDirection != 0) || (cxDelta != 0.0f) || (cyDelta != 0.0f))
		{
			if (cxDelta || cyDelta)
			{
				//if (pKeysBuffer[VK_RBUTTON] & 0xF0)
					//m_pPlayer->Rotate(cyDelta, 0.0f, -cxDelta);
				//else
					//m_pPlayer->Rotate(cyDelta, cxDelta, 0.0f);
			}
			if (dwDirection) client_player->Move(dwDirection, 200.0f * m_GameTimer.GetTimeElapsed(), true);
		}
		//direction

		//if (dwDirection)
		{
			XMVECTOR direction = {0.0f, 0.0f};
		
			if (dwDirection & DIR_FORWARD) direction += {1.0f, 0.0f};
			if (dwDirection & DIR_BACKWARD) direction += {-1.0f, 0.0f};
			if (dwDirection & DIR_RIGHT) direction += {0.0f, -1.0f};
			if (dwDirection & DIR_LEFT) direction += {0.0f, 1.0f};
			direction = XMVector2Normalize(direction);

			XMFLOAT2 dest;
			XMStoreFloat2(&dest, direction);
			network.g_client[network.my_id].ProcessInput(dest.x, dest.y);
			/*network.g_client[network.my_id].t_x = dest.x;
			network.g_client[network.my_id].t_z = dest.y;*/
		}
		//else {
			/*network.g_client[network.my_id].t_x = 0;
			network.g_client[network.my_id].t_z = 0;*/
		//}
	}
	
}

void CGameFramework::AnimateObjects()
{
	TIMEMANAGER.SetTimeElapsed(m_GameTimer.GetTimeElapsed());

	Network::Update(m_GameTimer.GetTimeElapsed());

	if (m_pScene) m_pScene->AnimateObjects(TIMEMANAGER.GetTimeElapsed());

	m_pPlayer2->Update(m_GameTimer.GetTimeElapsed());
	m_pPlayer3->Update(m_GameTimer.GetTimeElapsed());
	m_pPlayer->Update(m_GameTimer.GetTimeElapsed());

	sun_light->Update(XMFLOAT3(), m_GameTimer.GetTimeElapsed());

	particle_system->AnimateObjects(m_GameTimer.GetTimeElapsed());
	ui_system->AnimateObjects(m_GameTimer.GetTimeElapsed());
}

void CGameFramework::UpdateShaderVariables()
{
	if (m_pScene) m_pScene->UpdateShaderVariables(m_pd3dCommandList);
	m_pPlayer2->UpdateShaderVariables(m_pd3dCommandList);
	m_pPlayer3->UpdateShaderVariables(m_pd3dCommandList);
	m_pPlayer->UpdateShaderVariables(m_pd3dCommandList);
}

void CGameFramework::WaitForGpuComplete()
{
	const UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
	//m_nSwapChainBufferIndex = (m_nSwapChainBufferIndex + 1) % m_nSwapChainBuffers;

	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

//#define _WITH_PLAYER_TOP

void CGameFramework::FrameAdvance()
{
	m_GameTimer.Tick(0.0f);

	ProcessInput();

	AnimateObjects();


	HRESULT hResult = m_pd3dCommandAllocator->Reset();
	hResult = m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	m_pScene->SetGraphicsRootSignature(m_pd3dCommandList);

	UpdateShaderVariables();
	
	ShadowMapRender();
	Render();

	hResult = m_pd3dCommandList->Close();

	ID3D12CommandList *ppd3dCommandLists[] ={ m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

#ifdef _WITH_PRESENT_PARAMETERS
	DXGI_PRESENT_PARAMETERS dxgiPresentParameters;
	dxgiPresentParameters.DirtyRectsCount = 0;
	dxgiPresentParameters.pDirtyRects = NULL;
	dxgiPresentParameters.pScrollRect = NULL;
	dxgiPresentParameters.pScrollOffset = NULL;
	m_pdxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);
#else
#ifdef _WITH_SYNCH_SWAPCHAIN
	m_pdxgiSwapChain->Present(1, 0);
#else
	m_pdxgiSwapChain->Present(0, 0);
#endif
#endif

	renderTime += m_GameTimer.GetTimeElapsed();
	if (renderTime >= PRINT_RATE) {
		DirectX::Image a;	
	}

	//	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
	MoveToNextFrame();

	m_GameTimer.GetFrameRate(m_pszFrameRate + 12, 37);
	::SetWindowText(m_hWnd, m_pszFrameRate);
}

void CGameFramework::ShadowMapRender()
{
	m_pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ShadowMap->Resource(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	m_pd3dCommandList->ClearDepthStencilView(m_ShadowMap->Dsv(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	m_pd3dCommandList->OMSetRenderTargets(0, nullptr, false, &m_ShadowMap->Dsv());

	//light shader update
	m_pScene->SetGraphicsRootSignature(m_pd3dCommandList);

	m_pd3dCommandList->RSSetViewports(1, &m_ShadowMap->Viewport());
	m_pd3dCommandList->RSSetScissorRects(1, &m_ShadowMap->ScissorRect());

	sun_light->UpdateShaderVariables(m_pd3dCommandList);

	//Render
	m_pScene->ShadowMapRender(m_pd3dCommandList);
	m_pPlayer->ShadowMapRender(m_pd3dCommandList);
	m_pPlayer2->ShadowMapRender(m_pd3dCommandList);
	m_pPlayer3->ShadowMapRender(m_pd3dCommandList);

	m_pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ShadowMap->Resource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ ));
}

void CGameFramework::Render()
{

	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex];
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += (m_nSwapChainBufferIndex * m_nRtvDescriptorIncrementSize);

	float pfClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	m_pd3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, pfClearColor/*Colors::Azure*/, 0, NULL);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	m_pd3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE, &d3dDsvCPUDescriptorHandle);
	//m_pd3dCommandList->SetDescriptorHeaps(1, &NullSrvDescriptorHeap);
	//m_pd3dCommandList->SetGraphicsRootDescriptorTable(6, mNullSrv);
	
	m_pd3dCommandList->SetDescriptorHeaps(1, 
		ShadowMapSrvDescriptorHeap.GetAddressOf());
	m_pd3dCommandList->SetGraphicsRootDescriptorTable(6, m_ShadowMap->Srv());
	
	m_pScene->Render(m_pd3dCommandList, m_pCamera);

#ifdef _WITH_PLAYER_TOP
	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
#endif
	m_pPlayer->Render(m_pd3dCommandList, m_pCamera);
	m_pPlayer2->Render(m_pd3dCommandList, m_pCamera);
	m_pPlayer3->Render(m_pd3dCommandList, m_pCamera);

	particle_system->Render(m_pd3dCommandList, m_pCamera);

#ifdef SHADOW_TEXTURE_RENDER
	dd.Render(m_pd3dCommandList);
#endif

	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	ui_system->Render(m_pd3dCommandList, NULL);
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

}