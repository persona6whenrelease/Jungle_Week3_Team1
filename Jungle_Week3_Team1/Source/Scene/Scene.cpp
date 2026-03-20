#include "Scene.h"

class AActor;



const TMap<uint32, AActor*>& Scene::GetAActorMap()
{
	return AActorMap;
}

AActor* Scene::GetAActor(uint32 InUUID)
{
		
	auto IT = AActorMap.find(InUUID);

	if (IT == AActorMap.end())
	{
		return nullptr;
	}
	
	return IT->second;


}

void Scene::SpawnActor(const FString& InName)
{

	//AActor* NewActor = ObjectFactory::ConstructObject<AActor*>();
	//AActorMap.insert({ InName, NewActor });
	//
}

void Scene::OverWrite(Scene* InAddress)
{

	// 기존의 actor에 있는거 다 지우기

	for (auto& IT : AActorMap)
	{
		// 전에 있던 AActor 근데 남은 component들은 어떻게함?
		delete IT.second;
	}

	AActorMap = InAddress->GetAActorMap();

	delete InAddress;
}
