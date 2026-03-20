#pragma once
#include "../CoreMinimal.h"
#include "../Object/Object.h"
#include "ActorComponent.h"
#include "../Object/ObjectFactory.h"


class USceneComponent;
class UScene;

class AActor : public UObject
{
	DECLARE_OBJECT(AActor, UObject)

public:
	AActor() = default;
	~AActor() = default;

	virtual void Tick(float DeltaTime);
	virtual void Release() override;

public:
	template<typename T>
	T* AddComponent()
	{
		static_assert(std::is_base_of_v<UActorComponent, T>, "T must derive from UActorComponent");
		T* ActorComponent = NewObject<T>();
		ActorComponent->Owner = this;
		ActorComponent->InitializeComponent();

		Components.push_back(ActorComponent);
		return ActorComponent;
	}

	void AddComponent(UActorComponent* InComponent)
	{
		if (!InComponent) return;
		InComponent->Owner = this;
		Components.push_back(InComponent);
	}

	template<typename T>
	bool RemoveComponent()
	{
		static_assert(std::is_base_of_v<UActorComponent, T>, "T must derive from UActorComponent");

		uint64 currentSize = Components.size();
		for (uint64 i = 0; i < currentSize; ++i)
		{
			if (Components[i]->IsA(T::StaticClass()))
			{
				uint64 lastIndex = currentSize - 1;
				UActorComponent* Removed = Components[i];

				if (i != lastIndex)
				{
					Components[i] = Components[lastIndex];
				}

				Components.pop_back();
				
				Removed->Owner = nullptr;
				return true; 
			}
		}
		return false;
	}

	void RemoveComponent(UActorComponent* InComponent)
	{
		if (!InComponent) return;

		uint64 currentSize = Components.size();
		for (uint64 i = 0; i < currentSize; ++i)
		{
			if (Components[i] == InComponent)
			{
				uint64 lastIndex = currentSize - 1;

				if (i != lastIndex)
				{
					Components[i] = Components[lastIndex];
				}

				Components.pop_back();
				InComponent->Owner = nullptr;
				return;
			}
		}
	}

	template<typename T>
	T* GetComponentByClass()
	{
		for (uint32 i = 0; i < Components.size(); ++i)
		{
			if (Components[i]->IsA(T::StaticClass()))
			{
				return static_cast<T*>(Components[i]);
			}
		}
		return nullptr;
	}


	template<typename T>
	TArray<T*> GetComponentArrayByClass()
	{
		TArray<T*> Result;
		for (uint32 i = 0; i < Components.size(); ++i)
		{
			if (Components[i]->IsA(T::StaticClass()))
				Result.push_back(static_cast<T*>(Components[i]));
		}
		return Result;
	}



public:
	USceneComponent* RootComponent = nullptr;
	UScene* OwningScene = nullptr;

private:
	TArray<UActorComponent*> Components;
};