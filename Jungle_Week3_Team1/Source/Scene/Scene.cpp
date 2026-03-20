#include "Scene.h"

class AActor;



const TMap<uint32, AActor*>& Scene::GetAActorArray()
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
