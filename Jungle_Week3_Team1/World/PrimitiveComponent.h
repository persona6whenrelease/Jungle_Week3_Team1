#pragma once
#pragma once

// #include "Object/ObjectFactory.h"
#include "SceneComponent.h"
// #include "Render/Scene/RenderBus.h"
// #include "Render/Common/RenderTypes.h"
#include "../Types/RayTypes.h"

struct FMeshData;


class UPrimitiveComponent : public USceneComponent
{
private:
	FVector WorldAABBMinLocation;
	FVector WorldAABBMaxLocation;

protected:
	const FMeshData* MeshData = nullptr;
	FVector LocalExtents = { 0.5f, 0.5f, 0.5f };
	bool bIsVisible = true;

public:
	inline const FMeshData* GetMeshData() const { return MeshData; };

	inline void SetVisibility(bool bVisible) { bIsVisible = bVisible; }

	//Collision
	// void UpdateWorldAABB();
	// bool CheckAABB(const FRay& Ray);
	// bool Raycast(const FRay& Ray, FHitResult& OutHitResult);
	// bool IntersectTriangle(const FVector& RayOrigin, const FVector& RayDir, const FVector& V0, const FVector& V1, const FVector& V2, float& OutT);
	virtual bool RaycastMesh(const FRay& Ray, FHitResult& OutHitResult);
	inline bool IsVisible() const { return bIsVisible; }

	void UpdateWorldMatrix() override;

	/*virtual bool GetRenderCommand(const FMatrix& viewMatrix, const FMatrix& projMatrix, FRenderCommand& OutCommand) {
		OutCommand.Type = ERenderCommandType::Primitive;
		OutCommand.TransformConstants.Model = GetWorldMatrix();
		OutCommand.TransformConstants.View = viewMatrix;
		OutCommand.TransformConstants.Projection = projMatrix;

		return true;
	}*/

	//	각 Primitive Component는 자신이 어떤 Primitive Type인지 Renderer에게 알려줄 수 있어야 합니다. (Dynamic Binding)
	// virtual EPrimitiveType GetPrimitiveType() const = 0;
};

