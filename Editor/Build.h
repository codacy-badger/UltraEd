#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "gameobject.h"
#include "settings.h"
#include "shlwapi.h"

using namespace std;

namespace UltraEd
{
	class CBuild
	{
	public:
		static bool Start(vector<CGameObject*> gameObjects);
		static bool Run();
		static bool Load();
		static bool WriteSpecFile(vector<CGameObject*> gameObjects);
		static bool WriteSegmentsFile(vector<CGameObject*> gameObjects);
		static bool WriteModelsFile(vector<CGameObject*> gameObjects);
		static bool WriteCamerasFile(vector<CGameObject*> gameObjects);
		static bool WriteScriptsFile(vector<CGameObject*> gameObjects);
		static bool WriteMappingsFile(vector<CGameObject*> gameObjects);
		static bool WriteCollisionsFile(vector<CGameObject*> gameObjects);

	private:
		static bool Compile();
	};
}
