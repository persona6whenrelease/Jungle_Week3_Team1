#pragma once
#include "../../CoreMinimal.h"

#include <fstream>
#include "json.hpp"
#include "Scene.h"

using Json = nlohmann::json;


class SceneManager
{

public:


	// Scene이 없는 경우는 없음. 다만 renderer 부분에서 만들어지는 문제가 생길 수 있ㄷ음. SceneMap이관리할 수없기 때문에 이점에 대한 문제를 해결해야함.
	void SaveSceneFIle(const FString& InFileName, Scene* InAddress);

	// load
	Scene* LoadSceneFile(const FString& InFileName);

	 

private:
	TMap<FString, Scene*> SceneMap;
	void SaveSceneInfoToFile(const FString& InFileName, Scene* InAddress);

};