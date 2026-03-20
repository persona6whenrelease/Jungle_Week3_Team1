#include "MeshManager.h"
#include "../../Math/Utils.h"

FMeshData FMeshManager::CubeMeshData;
FMeshData FMeshManager::PlaneMeshData;
FMeshData FMeshManager::SphereMeshData;
FMeshData FMeshManager::TranslationGizmoMeshData;
FMeshData FMeshManager::RotationGizmoMeshData;
FMeshData FMeshManager::ScaleGizmoMeshData;
FMeshData FMeshManager::AxisMeshData;
FMeshData FMeshManager::MouseOverlayMeshData;
FMeshData FMeshManager::GridMeshData;

bool FMeshManager::bIsInitialized = false;

void FMeshManager::Initialize()
{
    if (bIsInitialized) return;

    if (CubeMeshData.Vertices.empty())
    {
        CreateCube();
    }

    if (PlaneMeshData.Vertices.empty())
    {
        CreatePlane();
    }

    if (SphereMeshData.Vertices.empty())
    {
        CreateSphere();
    }

    if (TranslationGizmoMeshData.Vertices.empty())
    {
        CreateTranslationGizmo();
    }

    if (ScaleGizmoMeshData.Vertices.empty())
    {
        CreateScaleGizmo();
    }

    if (RotationGizmoMeshData.Vertices.empty())
    {
        CreateRotationGizmo();
    }

    /*if (AxisMeshData.Vertices.empty())
    {
        CreateAxis();
    }

    if (GridMeshData.Vertices.empty())
    {
        CreateGrid();
    }

    if (MouseOverlayMeshData.Vertices.empty())
    {
        CreateMouseOverlay();
    }*/

    bIsInitialized = true;
}

void FMeshManager::CreateCube()
{
    TArray<FVertex>& vertices = CubeMeshData.Vertices;
    TArray<uint32>& indices = CubeMeshData.Indices;

    vertices.clear();
    indices.clear();

#if !TEST

    FVector4 right(0.0f, 1.0f, 0.0f, 1.0f);
    FVector4 left(0.0f, 0.5f, 0.0f, 1.0f);

    FVector4 up(0.0f, 0.0f, 1.0f, 1.0f);
    FVector4 down(0.0f, 0.0f, 0.5f, 1.0f);

    FVector4 front(1.0f, 0.0f, 0.0f, 1.0f);
    FVector4 back(0.5f, 0.0f, 0.0f, 1.0f);

#else

    FVector4 right(1.0f, 1.0f, 1.0f, 1.0f);
    FVector4 left(1.0f, 1.0f, 1.0f, 1.0f);

    FVector4 up(1.0f, 1.0f, 1.0f, 1.0f);
    FVector4 down(1.0f, 1.0f, 1.0f, 1.0f);

    FVector4 front(1.0f, 1.0f, 1.0f, 1.0f);
    FVector4 back(1.0f, 1.0f, 1.0f, 1.0f);

#endif

    FVector4 color(0.5f, 0.0f, 0.0f, 1.0f);

    vertices = {

        // FRONT
        {{ 0.5f,-0.5f,-0.5f}, front},
        {{ 0.5f,-0.5f, 0.5f}, front},
        {{ 0.5f, 0.5f, 0.5f}, front},
        {{ 0.5f, 0.5f,-0.5f}, front},

        // BACK
        {{-0.5f,-0.5f,-0.5f}, back},
        {{-0.5f, 0.5f,-0.5f}, back},
        {{-0.5f, 0.5f, 0.5f}, back},
        {{-0.5f,-0.5f, 0.5f}, back},

        // LEFT
        {{-0.5f,-0.5f,-0.5f}, left},
        {{-0.5f,-0.5f, 0.5f}, left},
        {{ 0.5f,-0.5f, 0.5f}, left},
        {{ 0.5f,-0.5f,-0.5f}, left},

        // RIGHT
        {{-0.5f, 0.5f,-0.5f}, right},
        {{ 0.5f, 0.5f,-0.5f}, right},
        {{ 0.5f, 0.5f, 0.5f}, right},
        {{-0.5f, 0.5f, 0.5f}, right},

        // TOP
        {{-0.5f,-0.5f, 0.5f}, up},
        {{-0.5f, 0.5f, 0.5f}, up},
        {{ 0.5f, 0.5f, 0.5f}, up},
        {{ 0.5f,-0.5f, 0.5f}, up},

        // BOTTOM
        {{-0.5f,-0.5f,-0.5f}, down},
        {{ 0.5f,-0.5f,-0.5f}, down},
        {{ 0.5f, 0.5f,-0.5f}, down},
        {{-0.5f, 0.5f,-0.5f}, down},
    };

    /*
        NOTE : 현재 24개의 index를 사용하는 이유는 엔진들은 보통 이 방식을 사용한다고 함.
        -> 추후에 다시 확인할 것
    */
    indices = {

    0,2,1, 0,3,2,      // front
    4,6,5, 4,7,6,      // back
    8,10,9, 8,11,10,   // left
    12,14,13, 12,15,14,// right
    16,18,17, 16,19,18,// top
    20,22,21, 20,23,22 // bottom
    };
}

void FMeshManager::CreateSphere(int slices, int stacks)
{
    TArray<FVertex>& vertices = SphereMeshData.Vertices;
    TArray<uint32>& indices = SphereMeshData.Indices;

    vertices.clear();
    indices.clear();

    //FVector4 color(1.0f, 1.0f, 1.0f, 1.0f);
    const float radius = 0.5f;
    //Create Vertex
    for (int i = 0; i <= stacks; ++i) {
        float pi = 3.141592f * (float)i / stacks;
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * 3.141592f * (float)j / slices;

            float x = radius * sin(pi) * cos(theta);
            float y = radius * sin(pi) * sin(theta);
            float z = radius * cos(pi);

#if TEST

            FVector4 color(
                x * 0.5f + 0.5f,
                y * 0.5f + 0.5f,
                z * 0.5f + 0.5f,
                1.0f
            );

#else

            FVector4 color(1.0f, 1.0f, 1.0f, 1.0f);

#endif

            vertices.push_back({ {x, y, z}, color });
        }
    }

    //Create Index
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            uint32 first = i * (slices + 1) + j;
            uint32 second = first + slices + 1;

            indices.push_back(first);
            indices.push_back(first + 1);
            indices.push_back(second);

            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second + 1);
        }
    }
}


void FMeshManager::CreatePlane()
{
	TArray<FVertex>& vertices = PlaneMeshData.Vertices;
	TArray<uint32>& indices = PlaneMeshData.Indices;

	vertices.clear();
	indices.clear();

	FVector4 color(1.0f, 1.0f, 1.0f, 1.0f);

	// Front face (Z = 0.01f)
	vertices = {
		{ {-0.5f, -0.5f, 0.01f}, color }, // 0
		{ {-0.5f,  0.5f, 0.01f}, color }, // 1
		{ { 0.5f,  0.5f, 0.01f}, color }, // 2
		{ { 0.5f, -0.5f, 0.01f}, color }, // 3

		// Back face (Z = -0.01f) - reversed winding for opposite normal
		{ {-0.5f, -0.5f, -0.01f}, color }, // 4
		{ { 0.5f,  0.5f, -0.01f}, color }, // 5
		{ {-0.5f,  0.5f, -0.01f}, color }, // 6
		{ { 0.5f, -0.5f, -0.01f}, color }  // 7
	};

	// Front face triangles
	indices = {
		0, 2, 1,  // Front tri 1
		0, 3, 2,  // Front tri 2

		// Back face triangles (reversed winding for correct normal)
		4, 6, 5,  // Back tri 1
		4, 5, 7   // Back tri 2
	};
}


void FMeshManager::CreateTranslationGizmo()
{
	/*TArray<FVertex>& vertices = TranslationGizmoMeshData.Vertices;
	TArray<uint32>& indices = TranslationGizmoMeshData.Indices;

	vertices.clear();
	indices.clear();

	const int32 segments = 8;
	const float radius = 0.05f;
	const float headRadius = 0.1f;
	const float stemLength = 0.8f;
	const float totalLength = 1.0f;

	FVector4 colors[3] = {
		FVector4(1.0f, 0.0f, 0.0f, 1.0f), // X: Red
		FVector4(0.0f, 1.0f, 0.0f, 1.0f), // Y: Green
		FVector4(0.0f, 0.0f, 1.0f, 1.0f)  // Z: Blue
	};

	int32 axisStartVertex = 0; // 개별 메쉬이므로 시작 인덱스는 항상 0

	for (int32 i = 0; i <= segments; ++i)
	{
		float Angle = (2.0f * 3.1415926535f * i) / segments;
		float c = cos(Angle);
		float s = sin(Angle);

		auto GetRotatedPos = [&](float x, float y, float z) -> FVector {
			FVector P(x, y, z);
			if (AxisIndex == 0) return FVector(P.Z, P.X, P.Y);
			if (AxisIndex == 1) return FVector(P.X, P.Z, P.Y);
			return P;
			};

		OutVertices.push_back({ GetRotatedPos(c * radius, s * radius, 0.0f), colors[AxisIndex], AxisIndex });
		OutVertices.push_back({ GetRotatedPos(c * radius, s * radius, stemLength), colors[AxisIndex], AxisIndex });
		OutVertices.push_back({ GetRotatedPos(c * headRadius, s * headRadius, stemLength), colors[AxisIndex], AxisIndex });
	}

	FVector TipPos = (AxisIndex == 0) ? FVector(totalLength, 0, 0) :
		(AxisIndex == 1) ? FVector(0, totalLength, 0) : FVector(0, 0, totalLength);

	OutVertices.push_back({ TipPos, colors[AxisIndex], AxisIndex });
	int32 tipIndex = (int32)OutVertices.size() - 1;

	for (int32 i = 0; i < segments; ++i)
	{
		int32 curr = axisStartVertex + (i * 3);
		int32 next = axisStartVertex + ((i + 1) * 3);

		// 몸통 (Cylinder)
		OutIndices.push_back(curr); OutIndices.push_back(curr + 1); OutIndices.push_back(next + 1);
		OutIndices.push_back(curr); OutIndices.push_back(next + 1); OutIndices.push_back(next);

		// 머리 밑면 (Cone Base)
		OutIndices.push_back(curr + 1); OutIndices.push_back(next + 2); OutIndices.push_back(curr + 2);
		OutIndices.push_back(curr + 1); OutIndices.push_back(next + 1); OutIndices.push_back(next + 2);

		// 머리 옆면 (Cone Tip)
		OutIndices.push_back(curr + 2); OutIndices.push_back(next + 2); OutIndices.push_back(tipIndex);
	}*/
}

void FMeshManager::CreateRotationGizmo()
{
    /*TArray<FVertex>& vertices = RotationGizmoMeshData.Vertices;
    TArray<uint32>& indices = RotationGizmoMeshData.Indices;

    vertices.clear();
    indices.clear();

    const float Radius = 1.0f;
    const float Thickness = 0.03f;
    const int Segments = 64;
    const int TubeSegments = 8;

    FVector4 Colors[3] = {
        FVector4(1.0f, 0.0f, 0.0f, 1.0f), // X-Axis (Red)
        FVector4(0.0f, 1.0f, 0.0f, 1.0f), // Y-Axis (Green)
        FVector4(0.0f, 0.0f, 1.0f, 1.0f)  // Z-Axis (Blue)
    };

    // 각 축(X, Y, Z)에 대해 고리 생성
    for (int axis = 0; axis < 3; ++axis)
    {
        uint32 StartVertexIdx = (uint32)vertices.size();

        for (int i = 0; i <= Segments; ++i)
        {
            float longitude = (float)i / Segments * 2.0f * M_PI;
            float sinLong = sin(longitude);
            float cosLong = cos(longitude);

            for (int j = 0; j < TubeSegments; ++j)
            {
                float latitude = (float)j / TubeSegments * 2.0f * M_PI;
                float sinLat = sin(latitude);
                float cosLat = cos(latitude);

                // 1. 로컬 토러스 좌표 계산 (기본 Z축 중심)
                float x = (Radius + Thickness * cosLat) * cosLong;
                float y = (Radius + Thickness * cosLat) * sinLong;
                float z = Thickness * sinLat;

                FVector pos;
                // 2. 축 방향에 따른 회전 정렬
                if (axis == 0)      pos = FVector(z, x, y); // X축 회전 (YZ 평면)
                else if (axis == 1) pos = FVector(x, z, y); // Y축 회전 (XZ 평면)
                else                pos = FVector(x, y, z); // Z축 회전 (XY 평면)

                vertices.push_back({ pos, Colors[axis], axis });
            }
        }

        // 인덱스 생성 (Side Quads)
        for (int i = 0; i < Segments; ++i)
        {
            for (int j = 0; j < TubeSegments; ++j)
            {
                uint32 nextI = i + 1;
                uint32 nextJ = (j + 1) % TubeSegments;

                uint32 i0 = StartVertexIdx + (i * TubeSegments + j);
                uint32 i1 = StartVertexIdx + (nextI * TubeSegments + j);
                uint32 i2 = StartVertexIdx + (nextI * TubeSegments + nextJ);
                uint32 i3 = StartVertexIdx + (i * TubeSegments + nextJ);

                indices.push_back(i0); indices.push_back(i1); indices.push_back(i2);
                indices.push_back(i0); indices.push_back(i2); indices.push_back(i3);
            }
        }
    }*/
}

void FMeshManager::CreateScaleGizmo()
{
    /*TArray<FVertex>& vertices = ScaleGizmoMeshData.Vertices;
    TArray<uint32>& indices = ScaleGizmoMeshData.Indices;

    vertices.clear();
    indices.clear();

    const float LineLength = 1.0f;
    const float BoxSize = 0.05f;
    const float StemThickness = 0.03f;

    FVector4 colors[3] = {
        FVector4(1.0f, 0.0f, 0.0f, 1.0f), // X
        FVector4(0.0f, 1.0f, 0.0f, 1.0f), // Y
        FVector4(0.0f, 0.0f, 1.0f, 1.0f)  // Z
    };

    FVector dirs[3] = { FVector(1,0,0), FVector(0,1,0), FVector(0,0,1) };

    auto AddBox = [&](const FVector& Center, const FVector& Extent, const FVector4& Color, int SubID) {
        uint32 StartIdx = (uint32)vertices.size();
        FVector p[8] = {
            Center + FVector(-Extent.X, -Extent.Y, -Extent.Z), Center + FVector(Extent.X, -Extent.Y, -Extent.Z),
            Center + FVector(Extent.X, Extent.Y, -Extent.Z),   Center + FVector(-Extent.X, Extent.Y, -Extent.Z),
            Center + FVector(-Extent.X, -Extent.Y, Extent.Z),  Center + FVector(Extent.X, -Extent.Y, Extent.Z),
            Center + FVector(Extent.X, Extent.Y, Extent.Z),    Center + FVector(-Extent.X, Extent.Y, Extent.Z)
        };

        for (int j = 0; j < 8; ++j)
        {
            vertices.push_back({ p[j], Color, SubID });
        }

        uint32 BoxIndices[] = {
            0,2,1, 0,3,2, 4,5,6, 4,6,7,
            0,1,5, 0,5,4, 2,3,7, 2,7,6,
            0,4,7, 0,7,3, 1,2,6, 1,6,5
        };
        for (uint32 Idx : BoxIndices) indices.push_back(StartIdx + Idx);
        };

    for (int i = 0; i < 3; ++i) {
        FVector StemExtent = (i == 0) ? FVector(LineLength * 0.5f, StemThickness, StemThickness) :
            (i == 1) ? FVector(StemThickness, LineLength * 0.5f, StemThickness) :
            FVector(StemThickness, StemThickness, LineLength * 0.5f);

        AddBox(dirs[i] * (LineLength * 0.5f), StemExtent, colors[i], i);

        float boxSizeHalf = BoxSize;
        AddBox(dirs[i] * LineLength, FVector(boxSizeHalf, boxSizeHalf, boxSizeHalf), colors[i], i);
    }*/
}
