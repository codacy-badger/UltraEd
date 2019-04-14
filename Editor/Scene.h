#pragma once

#include <map>
#include "deps/DXSDK/include/d3d8.h"
#include "deps/DXSDK/include/d3dx8.h"
#include "vendor/cJSON.h"
#include "Camera.h"
#include "Common.h"
#include "Debug.h"
#include "Gizmo.h"
#include "Grid.h"
#include "GameObject.h"

namespace UltraEd
{
	struct BuildFlag
	{
		enum Value { _, Run, Load };
	};

	class CScene
	{
	public:
		CScene();
		~CScene();
		bool Create(HWND windowHandle);
		void Delete();
		void Duplicate();
		void SetScript(string script);
		string GetScript();
		void Render();
		void Resize();
		void OnMouseWheel(short zDelta);
		void OnNew();
		void OnSave();
		void OnLoad();
		void OnAddCamera();
		void OnApplyTexture();
		void OnImportModel();
		void OnBuildROM(BuildFlag::Value flag);
		bool Pick(POINT mousePoint);
		void ReleaseResources(GameObjectRelease::Value type);
		void CheckInput(float);
		void ScreenRaycast(POINT screenPoint, D3DXVECTOR3 *origin, D3DXVECTOR3 *dir);
		void SetCameraView(CameraView::Value view);
		void SetGizmoModifier(GizmoModifierState state);
		CCamera *GetActiveCamera();
		bool ToggleMovementSpace();
		bool ToggleFillMode();
		bool ToggleSnapToGrid();

	private:
		HWND GetWndHandle();
		void SetTitle(string title);
		void UpdateViewMatrix();
		void ResetCameras();

	private:
		D3DLIGHT8 m_worldLight;
		D3DMATERIAL8 m_defaultMaterial;
		D3DMATERIAL8 m_selectedMaterial;
		D3DFILLMODE m_fillMode;
		CGizmo m_gizmo;
		CCamera m_cameras[4];
		IDirect3DDevice8 *m_device;
		IDirect3D8 *m_d3d8;
		D3DPRESENT_PARAMETERS m_d3dpp;
		map<GUID, CGameObject> m_gameObjects;
		CGrid m_grid;
		vector<GUID> selectedGameObjectIds;
		float mouseSmoothX, mouseSmoothY;
		CameraView::Value m_activeCameraView;
		CGameObject m_cameraEditorObject;
	};
}
