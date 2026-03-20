#pragma once

#include "../../CoreMinimal.h"

class UObject;
class AActor;

class Scene : public UObject
{
public:
	Scene();
	virtual ~Scene();

	const TMap<uint32, AActor*>& GetAActorMap();
	AActor* GetAActor(uint32 InUUID);
	void SpawnActor(const FString& InName);
	void OverWrite(Scene* InAddress);

private:
	TMap<uint32, AActor*> AActorMap;

};