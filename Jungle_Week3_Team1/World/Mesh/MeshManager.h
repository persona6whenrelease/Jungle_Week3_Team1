#pragma once

#include "../Types/VertexTypes.h"

class FMeshManager
{
private:
	FMeshManager() = default;

	static FMeshData CubeMeshData;
	static FMeshData PlaneMeshData;
	static FMeshData SphereMeshData;
	static FMeshData TranslationGizmoMeshData;
	static FMeshData RotationGizmoMeshData;
	static FMeshData ScaleGizmoMeshData;
	static FMeshData AxisMeshData;
	static FMeshData GridMeshData;

	static FMeshData MouseOverlayMeshData;

	static void CreateCube();
	static void CreatePlane();
	static void CreateSphere(int slices = 20, int stacks = 20);
	static void CreateTranslationGizmo();
	static void CreateRotationGizmo();
	static void CreateScaleGizmo();
	// static void CreateAxis();
	// static void CreateGrid();

	// static void CreateMouseOverlay();


	static bool bIsInitialized;

public:
	static FMeshManager& Get()
	{
		static FMeshManager instance;
		return instance;
	}

	FMeshManager(const FMeshManager&) = delete;
	FMeshManager& operator=(const FMeshManager&) = delete;

	static void Initialize();
	static const FMeshData& GetCube() { return Get().CubeMeshData; }
	static const FMeshData& GetPlane() { return Get().PlaneMeshData; }
	static const FMeshData& GetSphere() { return Get().SphereMeshData; }
	static const FMeshData& GetTranslationGizmo() { return Get().TranslationGizmoMeshData; }
	static const FMeshData& GetRotationGizmo() { return Get().RotationGizmoMeshData; }
	static const FMeshData& GetScaleGizmo() { return Get().ScaleGizmoMeshData; }
	static const FMeshData& GetAxis() { return Get().AxisMeshData; }
	static const FMeshData& GetGrid() { return Get().GridMeshData; }

	static const FMeshData& GetMouseOverlay() { return Get().MouseOverlayMeshData; }
};

