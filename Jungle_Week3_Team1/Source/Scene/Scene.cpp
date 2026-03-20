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

	// delete all actor

	for (auto& IT : AActorMap)
	{
		// Todo Check
		delete IT.second;
	}

	AActorMap = InAddress->GetAActorMap();

	delete InAddress;
}
