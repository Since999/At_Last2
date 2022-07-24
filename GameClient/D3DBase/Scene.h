//-----------------------------------------------------------------------------
// File: Scene.h
//-----------------------------------------------------------------------------

#pragma once

#include "Shader.h"

class CObject;
class CGameFramework;
class CSunLight;
class CAnimationObject;

class CScene
{
public:
	CScene(ID3D12RootSignature* root_sig = NULL);
	~CScene();

	virtual bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseObjects();

	ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device* pd3dDevice);
	ID3D12RootSignature* GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }
	void SetGraphicsRootSignature(ID3D12GraphicsCommandList* pd3dCommandList) { pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature); }

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual bool ProcessInput(UCHAR* pKeysBuffer, HWND& hwnd);
	virtual void AnimateObjects(float fTimeElapsed);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL);
	virtual void ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList);

	void ReleaseUploadBuffers();

	virtual void AddObject(CGameObject* object);
	virtual void RemoveObject(CGameObject* object);

	virtual void StartEvent(){}
	virtual void StopEvent(){}
private:
	std::vector<CGameObject*> remove_list;
	std::list<CGameObject*> object_list;

protected:
	void RemoveObjects();
	CShader** m_ppShaders = NULL;
	int							m_nShaders = 0;

	ID3D12RootSignature* m_pd3dGraphicsRootSignature = NULL;
};

class CLobbyScene : public CScene {
public:
	CLobbyScene(ID3D12RootSignature* root_sig = NULL);
	~CLobbyScene();

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
};

class CSelectScene : public CScene {
public:
	CSelectScene(ID3D12RootSignature* root_sig = NULL);
	~CSelectScene(){}

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
};

class CMainGameScene : public CScene {
public:
	CMainGameScene(ID3D12RootSignature* root_sig = NULL);
	~CMainGameScene();

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseObjects();

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual bool ProcessInput(UCHAR* pKeysBuffer, HWND& hwnd);
	virtual void AnimateObjects(float fTimeElapsed);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL);
	virtual void ShadowMapRender(ID3D12GraphicsCommandList* pd3dCommandList);

	virtual void StartEvent();
	virtual void StopEvent();

private:
	CPlayer* m_pPlayer = NULL;
	CPlayer* m_pPlayer2 = NULL;
	CPlayer* m_pPlayer3 = NULL;
	CPlayer* client_player = NULL;
	CSunLight* sun_light = NULL;

	CAnimationObject* test_zombie = NULL;
};