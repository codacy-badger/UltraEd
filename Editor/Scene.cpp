#include "build.h"
#include "Scene.h"
#include "Settings.h"
#include "FileIO.h"
#include "Dialog.h"
#include "Util.h"

namespace UltraEd
{
	CScene::CScene()
	{
		ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
		m_d3d8 = 0;
		m_device = 0;
		m_fillMode = D3DFILL_SOLID;

		ZeroMemory(&m_defaultMaterial, sizeof(D3DMATERIAL8));
		m_defaultMaterial.Diffuse.r = m_defaultMaterial.Ambient.r = 1.0f;
		m_defaultMaterial.Diffuse.g = m_defaultMaterial.Ambient.g = 1.0f;
		m_defaultMaterial.Diffuse.b = m_defaultMaterial.Ambient.b = 1.0f;
		m_defaultMaterial.Diffuse.a = m_defaultMaterial.Ambient.a = 1.0f;

		ZeroMemory(&m_selectedMaterial, sizeof(D3DMATERIAL8));
		m_selectedMaterial.Ambient.r = m_selectedMaterial.Emissive.r = 0.0f;
		m_selectedMaterial.Ambient.g = m_selectedMaterial.Emissive.g = 1.0f;
		m_selectedMaterial.Ambient.b = m_selectedMaterial.Emissive.b = 0.0f;
		m_selectedMaterial.Ambient.a = m_selectedMaterial.Emissive.a = 1.0f;

		ZeroMemory(&m_worldLight, sizeof(D3DLIGHT8));
		m_worldLight.Type = D3DLIGHT_DIRECTIONAL;
		m_worldLight.Diffuse.r = 1.0f;
		m_worldLight.Diffuse.g = 1.0f;
		m_worldLight.Diffuse.b = 1.0f;
		m_worldLight.Direction = D3DXVECTOR3(0, 0, 1);
	}

	CScene::~CScene()
	{
		ReleaseResources(ModelRelease::AllResources);
		if (m_device) m_device->Release();
		if (m_d3d8) m_d3d8->Release();
	}

	bool CScene::Create(HWND windowHandle)
	{
		if ((m_d3d8 = Direct3DCreate8(D3D_SDK_VERSION)) == NULL)
		{
			return false;
		}

		D3DDISPLAYMODE d3ddm;
		if (FAILED(m_d3d8->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm)))
		{
			return false;
		}

		m_d3dpp.Windowed = TRUE;
		m_d3dpp.SwapEffect = D3DSWAPEFFECT_COPY_VSYNC;
		m_d3dpp.BackBufferFormat = d3ddm.Format;
		m_d3dpp.EnableAutoDepthStencil = TRUE;
		m_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

		if (FAILED(m_d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, windowHandle,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_d3dpp, &m_device)))
		{
			return false;
		}

		// Setup the new scene.
		OnNew();
		Resize();

		return true;
	}

	void CScene::OnNew()
	{
		SetTitle("New");
		selectedActorIds.clear();
		ReleaseResources(ModelRelease::AllResources);
		m_actors.clear();
		ResetViews();
	}

	void CScene::OnSave()
	{
		vector<CSavable*> savables;

		// Save all editor views.
		for (int i = 0; i < 4; i++) savables.push_back(&m_views[i]);

		// Save all of the actors in the scene.
		for (auto actor : m_actors)
		{
			savables.push_back(actor.second.get());
		}

		string savedName;
		if (CFileIO::Save(savables, savedName))
		{
			SetTitle(savedName);
		}
	}

	void CScene::OnLoad()
	{
		cJSON *root = NULL;
		string loadedName;
		if (CFileIO::Load(&root, loadedName))
		{
			OnNew();
			SetTitle(loadedName);

			// Restore editor views.
			int count = 0;
			cJSON *views = cJSON_GetObjectItem(root, "views");
			cJSON *viewItem = NULL;
			cJSON_ArrayForEach(viewItem, views)
			{
				m_views[count++].Load(m_device, viewItem);
			}

			// Restore saved actor.
			cJSON *actors = cJSON_GetObjectItem(root, "actors");
			cJSON *actor = NULL;
			cJSON_ArrayForEach(actor, actors)
			{
				switch (CActor::GetType(actor))
				{
					case ActorType::Model:
					{
						auto model = make_shared<CModel>();
						model->Load(m_device, actor);
						m_actors[model->GetId()] = model;
						break;
					}
					case ActorType::Camera:
					{
						auto camera = make_shared<CCamera>();
						camera->Load(m_device, actor);
						m_actors[camera->GetId()] = camera;
						break;
					}
				}
			}

			cJSON_Delete(root);
		}
	}

	void CScene::OnImportModel()
	{
		string file;
		if (CDialog::Open("Add Model",
			"3D Studio (*.3ds)\0*.3ds\0Blender (*.blend)\0*.blend\0Autodesk (*.fbx)\0*.fbx\0"
			"Collada (*.dae)\0*.dae\0DirectX (*.x)\0*.x\0Stl (*.stl)\0*.stl\0"
			"VRML (*.wrl)\0*.wrl\0Wavefront (*.obj)\0*.obj", file))
		{
			auto model = make_shared<CModel>(file.c_str());
			m_actors[model->GetId()] = model;
			char buffer[1024];
			sprintf(buffer, "Actor %d", m_actors.size());
			m_actors[model->GetId()]->SetName(string(buffer));
		}
	}

	void CScene::OnBuildROM(BuildFlag::Value flag)
	{
		vector<CActor*> actors;

		// Gather all of the actors in the scene.
		for (auto actor : m_actors)
		{
			actors.push_back(actor.second.get());
		}

		if (CBuild::Start(actors))
		{
			if (flag & BuildFlag::Run)
			{
				CBuild::Run();
			}
			else if (flag & BuildFlag::Load)
			{
				if (CBuild::Load())
				{
					MessageBox(NULL, "The ROM has been successfully loaded to the cart!", "Success", MB_OK);
				}
				else
				{
					MessageBox(NULL, "Could not load ROM onto cart. Make sure your cart is connected via USB.", "Error", MB_OK);
				}
			}
			else
			{
				MessageBox(NULL, "The ROM has been successfully built!", "Success", MB_OK);
			}
		}
		else
		{
			MessageBox(NULL, "The ROM build has failed. Make sure the build tools have been installed.", "Error", MB_OK);
		}
	}

	void CScene::OnApplyTexture()
	{
		string file;

		if (selectedActorIds.empty())
		{
			MessageBox(NULL, "An object must be selected first.", "Error", MB_OK);
			return;
		}

		for (auto selectedActorId : selectedActorIds)
		{
			if (CDialog::Open("Select a texture",
				"PNG (*.png)\0*.png\0JPEG (*.jpg)\0"
				"*.jpg\0BMP (*.bmp)\0*.bmp\0TGA (*.tga)\0*.tga", file))
			{
				if (m_actors[selectedActorId]->GetType() != ActorType::Model) continue;
				if (!dynamic_cast<CModel*>(m_actors[selectedActorId].get())->LoadTexture(m_device, file.c_str()))
				{
					MessageBox(NULL, "Texture could not be loaded.", "Error", MB_OK);
				}
			}
		}
	}

	bool CScene::Pick(POINT mousePoint)
	{
		D3DXVECTOR3 orig, dir;
		ScreenRaycast(mousePoint, &orig, &dir);
		bool gizmoSelected = m_gizmo.Select(orig, dir);
		float closestDist = FLT_MAX;

		// When just selecting the gizmo don't check any actors.
		if (gizmoSelected) return true;

		// Check all actors to see which poly might have been picked.
		for (auto actor : m_actors)
		{
			// Only choose the closest actors to the view.
			float pickDist = 0;
			if (actor.second->Pick(orig, dir, &pickDist) && pickDist < closestDist)
			{
				closestDist = pickDist;
				vector<GUID>::iterator found = find(selectedActorIds.begin(), selectedActorIds.end(), actor.first);
				if (found == selectedActorIds.end())
				{
					if (!GetAsyncKeyState(VK_SHIFT)) selectedActorIds.clear();
					selectedActorIds.push_back(actor.first);
				}
				else
				{
					// Shift clicking an already selected actors so unselect it.
					if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
					{
						selectedActorIds.erase(found);
					}
					else
					{
						// Unselected everything and only select what was clicked.
						selectedActorIds.clear();
						selectedActorIds.push_back(actor.first);
					}
				}
			}
		}

		if (closestDist != FLT_MAX) return true;
		if (!gizmoSelected) selectedActorIds.clear();
		return false;
	}

	void CScene::Resize()
	{
		if (m_device)
		{
			ReleaseResources(ModelRelease::VertexBufferOnly);
			m_device->Reset(&m_d3dpp);
			UpdateViewMatrix();
		}
	}

	void CScene::UpdateViewMatrix()
	{
		D3DXMATRIX viewMat;
		if (m_activeViewType == ViewType::Perspective)
		{
			RECT rect;
			GetClientRect(GetWndHandle(), &rect);

			float aspect = (float)rect.right / (float)rect.bottom;
			float fov = D3DX_PI / 2.0f;

			D3DXMatrixPerspectiveFovLH(&viewMat, fov, aspect, 0.1f, 1000.0f);
		}
		else
		{
			float size = D3DXVec3Length(&GetActiveView()->GetPosition());
			D3DXMatrixOrthoLH(&viewMat, size, size, -1000.0f, 1000.0f);
		}

		m_device->SetTransform(D3DTS_PROJECTION, &viewMat);
	}

	void CScene::Render()
	{
		// Calculate the frame rendering speed.
		static float lastTime = (float)timeGetTime();
		float currentTime = (float)timeGetTime();
		float deltaTime = (currentTime - lastTime) * 0.001f;

		CheckInput(deltaTime);

		if (m_device)
		{
			ID3DXMatrixStack *stack;
			if (!SUCCEEDED(D3DXCreateMatrixStack(0, &stack))) return;
			stack->LoadMatrix(&GetActiveView()->GetViewMatrix());

			m_device->SetTransform(D3DTS_WORLD, stack->GetTop());
			m_device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(90, 90, 90), 1.0f, 0);
			m_device->SetLight(0, &m_worldLight);
			m_device->LightEnable(0, TRUE);

			if (!SUCCEEDED(m_device->BeginScene())) return;

			m_grid.Render(m_device);
			CDebug::Instance().Render(m_device);

			// Render all actors with selected fill mode.
			m_device->SetMaterial(&m_defaultMaterial);
			m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
			m_device->SetRenderState(D3DRS_FILLMODE, m_fillMode);

			for (auto actor : m_actors)
			{
				actor.second->Render(m_device, stack);
			}

			if (!selectedActorIds.empty())
			{
				// Highlight the selected actor.
				m_device->SetMaterial(&m_selectedMaterial);
				m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
				for (auto selectedActorId : selectedActorIds)
				{
					m_actors[selectedActorId]->Render(m_device, stack);
				}

				// Draw the gizmo on "top" of all objects in scene.
				m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
				m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
				m_gizmo.Render(m_device, stack, GetActiveView());
			}

			m_device->EndScene();
			m_device->Present(NULL, NULL, NULL, NULL);
		}

		lastTime = currentTime;
	}

	void CScene::CheckInput(float deltaTime)
	{
		POINT mousePoint;
		GetCursorPos(&mousePoint);
		ScreenToClient(GetWndHandle(), &mousePoint);
		CView *view = GetActiveView();

		static POINT prevMousePoint = mousePoint;
		const float smoothingModifier = 16.0f;
		const float mouseSpeedModifier = 0.55f;

		if (GetActiveWindow() != GetParent(GetWndHandle())) return;
		if (GetAsyncKeyState('1')) m_gizmo.SetModifier(Translate);
		if (GetAsyncKeyState('2')) m_gizmo.SetModifier(Rotate);
		if (GetAsyncKeyState('3')) m_gizmo.SetModifier(Scale);

		if (GetAsyncKeyState(VK_LBUTTON) && !selectedActorIds.empty())
		{
			D3DXVECTOR3 rayOrigin, rayDir;
			ScreenRaycast(mousePoint, &rayOrigin, &rayDir);
			GUID lastSelectedActorId = selectedActorIds.back();
			for (auto selectedActorId : selectedActorIds)
			{
				m_gizmo.Update(GetActiveView(), rayOrigin, rayDir, m_actors[selectedActorId].get(),
					m_actors[lastSelectedActorId].get());
			}
		}
		else if (GetAsyncKeyState(VK_RBUTTON) && m_activeViewType == ViewType::Perspective)
		{
			if (GetAsyncKeyState('W')) view->Walk(4.0f * deltaTime);
			if (GetAsyncKeyState('S')) view->Walk(-4.0f * deltaTime);
			if (GetAsyncKeyState('A')) view->Strafe(-4.0f * deltaTime);
			if (GetAsyncKeyState('D')) view->Strafe(4.0f * deltaTime);

			mouseSmoothX = CUtil::Lerp(deltaTime * smoothingModifier, mouseSmoothX, (FLOAT)(mousePoint.x - prevMousePoint.x));
			mouseSmoothY = CUtil::Lerp(deltaTime * smoothingModifier, mouseSmoothY, (FLOAT)(mousePoint.y - prevMousePoint.y));

			view->Yaw(mouseSmoothX * mouseSpeedModifier * deltaTime);
			view->Pitch(mouseSmoothY * mouseSpeedModifier * deltaTime);
		}
		else if (GetAsyncKeyState(VK_MBUTTON))
		{
			mouseSmoothX = CUtil::Lerp(deltaTime * smoothingModifier, mouseSmoothX, (FLOAT)(prevMousePoint.x - mousePoint.x));
			mouseSmoothY = CUtil::Lerp(deltaTime * smoothingModifier, mouseSmoothY, (FLOAT)(mousePoint.y - prevMousePoint.y));

			view->Strafe(mouseSmoothX * deltaTime);
			view->Fly(mouseSmoothY * deltaTime);
		}
		else
		{
			m_gizmo.Reset();

			// Reset smoothing values for new mouse view movement.
			mouseSmoothX = mouseSmoothX = 0;
		}

		// Remeber the last position so we know how much to move the view.
		prevMousePoint = mousePoint;
	}

	void CScene::OnMouseWheel(short zDelta)
	{
		GetActiveView()->Walk(zDelta * 0.005f);
		UpdateViewMatrix();
	}

	void CScene::ScreenRaycast(POINT screenPoint, D3DXVECTOR3 *origin, D3DXVECTOR3 *dir)
	{
		CView *view = GetActiveView();

		D3DVIEWPORT8 viewport;
		m_device->GetViewport(&viewport);

		D3DXMATRIX matProj;
		m_device->GetTransform(D3DTS_PROJECTION, &matProj);

		D3DXMATRIX matWorld;
		D3DXMatrixIdentity(&matWorld);

		D3DXVECTOR3 v1;
		D3DXVECTOR3 start = D3DXVECTOR3((FLOAT)screenPoint.x, (FLOAT)screenPoint.y, 0.0f);
		D3DXVec3Unproject(&v1, &start, &viewport, &matProj, &view->GetViewMatrix(), &matWorld);

		D3DXVECTOR3 v2;
		D3DXVECTOR3 end = D3DXVECTOR3((FLOAT)screenPoint.x, (FLOAT)screenPoint.y, 1.0f);
		D3DXVec3Unproject(&v2, &end, &viewport, &matProj, &view->GetViewMatrix(), &matWorld);

		*origin = v1;
		D3DXVec3Normalize(dir, &(v2 - v1));
	}

	void CScene::SetViewType(ViewType::Value type)
	{
		m_activeViewType = type;
		UpdateViewMatrix();
	}

	void CScene::SetGizmoModifier(GizmoModifierState state)
	{
		m_gizmo.SetModifier(state);
	}

	CView *CScene::GetActiveView()
	{
		return &m_views[m_activeViewType];
	}

	bool CScene::ToggleMovementSpace()
	{
		if (!selectedActorIds.empty())
		{
			return m_gizmo.ToggleSpace(m_actors[selectedActorIds.back()].get());
		}
		return false;
	}

	bool CScene::ToggleFillMode()
	{
		if (m_fillMode == D3DFILL_SOLID)
		{
			m_fillMode = D3DFILL_WIREFRAME;
			return false;
		}

		m_fillMode = D3DFILL_SOLID;
		return true;
	}

	void CScene::ReleaseResources(ModelRelease::Value type)
	{
		m_grid.Release();
		m_gizmo.Release();
		CDebug::Instance().Release();
		for (auto actor : m_actors)
		{
			if (auto model = dynamic_cast<CModel*>(actor.second.get()))
			{
				model->Release(type);
			}
			else
			{
				actor.second->Release();
			}
		}
	}

	void CScene::Delete()
	{
		for (auto selectedActorId : selectedActorIds)
		{
			if (auto model = dynamic_cast<CModel*>(m_actors[selectedActorId].get()))
			{
				model->Release(ModelRelease::AllResources);
			}
			else
			{
				m_actors[selectedActorId]->Release();
			}
			m_actors.erase(selectedActorId);
		}
		selectedActorIds.clear();
	}

	void CScene::Duplicate()
	{
		for (auto selectedActorId : selectedActorIds)
		{
			switch (m_actors[selectedActorId]->GetType())
			{
				case ActorType::Model:
				{
					auto model = make_shared<CModel>(*dynamic_cast<CModel*>(m_actors[selectedActorId].get()));
					string texturePath = model->GetResources()["textureDataPath"];
					model->LoadTexture(m_device, texturePath.c_str());
					m_actors[model->GetId()] = model;
					break;
				}
				case ActorType::Camera:
				{
					auto camera = make_shared<CCamera>(*dynamic_cast<CCamera*>(m_actors[selectedActorId].get()));
					m_actors[camera->GetId()] = camera;
					break;
				}
			}
		}
	}

	void CScene::SetScript(string script)
	{
		if (!selectedActorIds.empty())
		{
			m_actors[selectedActorIds[0]]->SetScript(script);
		}
	}

	string CScene::GetScript()
	{
		if (!selectedActorIds.empty())
		{
			return m_actors[selectedActorIds[0]]->GetScript();
		}
		return string("");
	}

	HWND CScene::GetWndHandle()
	{
		D3DDEVICE_CREATION_PARAMETERS params;
		if (m_device && SUCCEEDED(m_device->GetCreationParameters(&params)))
		{
			return params.hFocusWindow;
		}
		return NULL;
	}

	void CScene::ResetViews()
	{
		for (int i = 0; i < 4; i++)
		{
			m_views[i].Reset();
			switch (i)
			{
			case ViewType::Perspective:
				m_views[i].Fly(2);
				m_views[i].Walk(-5);
				m_views[i].SetViewType(ViewType::Perspective);
				break;
			case ViewType::Top:
				m_views[i].Fly(12);
				m_views[i].Pitch(D3DX_PI / 2);
				m_views[i].SetViewType(ViewType::Top);
				break;
			case ViewType::Left:
				m_views[i].Yaw(D3DX_PI / 2);
				m_views[i].Walk(-12);
				m_views[i].SetViewType(ViewType::Left);
				break;
			case ViewType::Front:
				m_views[i].Yaw(D3DX_PI);
				m_views[i].Walk(-12);
				m_views[i].SetViewType(ViewType::Front);
				break;
			}
		}
	}

	void CScene::SetTitle(string title)
	{
		HWND parentWnd = GetParent(GetWndHandle());
		if (parentWnd != NULL)
		{
			title.append(" - ").append(APP_NAME);
			SetWindowText(parentWnd, title.c_str());
		}
	}

	bool CScene::ToggleSnapToGrid()
	{
		return m_gizmo.ToggleSnapping();
	}

	void CScene::OnAddCamera()
	{
		char buffer[1024];
		auto newCamera = make_shared<CCamera>();
		m_actors[newCamera->GetId()] = newCamera;
		sprintf(buffer, "Camera %d", m_actors.size());
		m_actors[newCamera->GetId()]->SetName(string(buffer));
	}
}
