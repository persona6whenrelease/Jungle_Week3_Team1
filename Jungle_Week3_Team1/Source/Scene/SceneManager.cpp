#include "SceneManager.h"

void SceneManager::SaveSceneFIle(const FString& InFileName, Scene* InAddress)
{
	auto IT = SceneMap.find(InFileName);


	if (IT != SceneMap.end())
	{
		// 이미 존재하는 scene
		Scene* OldScene = IT->second;
		InAddress->OverWrite(OldScene);

	}

	// 이 filename에 InAddress의 정보를 저장해야함
	SceneMap.insert({ InFileName , InAddress });

}

Scene* SceneManager::LoadSceneFile(const FString& InFileName)
{

	auto IT = SceneMap.find(InFileName);


	// 못 찾는 경우
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

	// 추후 수정
	SaveFile["NextUUID"] = ;
	SaveFile["Primitive"] = {};
	const TMap<uint32, AActor*>& AActorMap = InAddress->GetAActorMap();
	uint32 temp_uuid = 1;
	for (const auto& IT : AActorMap)
	{
		Json Transform = {};
		Transform["Location"] = { IT.second.getLocation() };
		Transform["Rotateion"] = { IT.second.getRoation() };
		Transform["Scale"] = { IT.second.getScale() };
		Transform["Type"] = { IT.second.GetName() };
		
		SaveFile["Primitive"].insert(Transform);

	}
	
}
