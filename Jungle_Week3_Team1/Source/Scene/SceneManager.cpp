#include "SceneManager.h"

void SceneManager::SaveSceneFIle(const FString& InFileName, Scene* InAddress)
{
	auto IT = SceneMap.find(InFileName);


	if (IT != SceneMap.end())
	{
		// existing scene
		Scene* OldScene = IT->second;
		OldScene->OverWrite(InAddress);

		SaveSceneInfoToFile(InFileName, OldScene);
		return;
	}

	// save filename of inaddress scene
	SceneMap.insert({ InFileName , InAddress });
	SaveSceneInfoToFile(InFileName, InAddress);

}

Scene* SceneManager::LoadSceneFile(const FString& InFileName)
{

	auto IT = SceneMap.find(InFileName);


	// nothing
	if (IT == SceneMap.end())
	{

		return nullptr;

	}

	return IT->second;
	
	
}

void SceneManager::SaveSceneInfoToFile(const FString& InFileName, Scene* InAddress)
{
	Json SaveFile = {};
	SaveFile["Version"] = 1;
	SaveFile["Primitives"] = {};

	const TMap<uint32, AActor*>& AActorMap = InAddress->GetAActorMap();
	int32 Index = 0;
	for (const auto& IT : AActorMap)
	{
		Json Primitive = {};
		Primitive["Location"] = { IT.second->getLocation() };
		Primitive["Rotation"] = { IT.second->getRotation() }; // 오타 수정
		Primitive["Scale"] = { IT.second->getScale() };
		Primitive["Type"] = { IT.second->GetName() };

		SaveFile["Primitives"][std::to_string(Index++)] = Primitive; // 수정
	}

	std::ofstream File(InFileName);
	File << SaveFile.dump(4);
}
