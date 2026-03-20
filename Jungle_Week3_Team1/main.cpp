#include <windows.h>

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#include <d3d11.h>
#include <d3dcompiler.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "Render.h"




extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    WCHAR WindowClass[] = L"JungleWindowClass";
    WCHAR Title[] = L"Galagon";
    WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

    RegisterClassW(&wndclass);

    HWND hWnd = CreateWindowExW(
        0,
        WindowClass,
        Title,
        WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 1024,
        nullptr, nullptr, hInstance, nullptr);

    URenderer	renderer;
    renderer.Create(hWnd);
    renderer.CreateShader();
    renderer.CreateConstantBuffer();

    // Imgui 생성
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

	// Sphere기반 버텍스 버퍼 생성
    UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);
    ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));

	// 각종 세팅.
    bool bIsExit = false;

	// 경계선
    const float leftBorder = -1.0f;
    const float rightBorder = 1.0f;
    const float topBorder = 1.0f;
    const float bottomBorder = -1.0f;
    const float sphereRadius = 1.0f;

	// 프레임
    const int targetFPS = 30;
    const double targetFrameTime = 1000.0 / targetFPS;

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER startTime, endTime;
    float elapsedTime = 0.0;

	// UBall를 관리할 PrimitiveList 선언.
	UPrimitive** PrimitiveList = nullptr;
	int Capacity = 8;

	PrimitiveList = new UPrimitive * [Capacity];
	for (int i = 0; i < Capacity; i++) PrimitiveList[i] = nullptr;

	PrimitiveList[0] = new UBall();


	int numberOfBalls = 1;

	float gravityStrength = 0.005f;
	bool bGravity = true;

    // Main Loop (Quit Message가 들어오기 전까지 아래 Loop를 무한히 실행하게 됨)
	while (bIsExit == false)
	{
		QueryPerformanceCounter(&startTime);
		MSG msg;

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				bIsExit = true;
				break;
			}
		}

		// 이동 계산
		for (int i = 0; i < UBall::TotalNumBalls; i++)
		{
			UPrimitive* CurrentBall = PrimitiveList[i];

			if (bGravity)
			{
				CurrentBall->MoveGravity(gravityStrength);
			}

			CurrentBall->Move();
			CurrentBall->BorderToScreen(sphereRadius, leftBorder, rightBorder, topBorder, bottomBorder);
		}

		// 충돌 계산
		for (int i = 0; i < UBall::TotalNumBalls; i++)
		{
			for (int j = i + 1; j < UBall::TotalNumBalls; j++) 
			{
				UPrimitive* BallA = PrimitiveList[i];
				UPrimitive* BallB = PrimitiveList[j];

				BallA->CollisionCalculation(BallB);
			}
		}

		renderer.Prepare();
		renderer.PrepareShader();

		// Constant Buffer 업데이트 및 다중 렌더링.
		for (int i = 0; i < UBall::TotalNumBalls; i++)
		{
			UBall* CurrentBall = (UBall*)PrimitiveList[i];

			if (CurrentBall)
			{
				renderer.UpdateConstant(CurrentBall->Location, CurrentBall->Radius);
				renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);
			}
		}

		// ImGui준비
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle World!");
		ImGui::Checkbox("Gravity", &bGravity);

		// 생성 및 소멸
		if (ImGui::InputInt("Number of Balls", &numberOfBalls))
		{
			if (UBall::TotalNumBalls > numberOfBalls)
			{
				// 랜덤 소멸 - 소멸된 자리에 마지막 인덱스에 있던 객체가 들어감.
				int diff = UBall::TotalNumBalls - numberOfBalls;
				for (int i = 0; i < diff; i++)
				{
					if (UBall::TotalNumBalls <= 1)
					{
						numberOfBalls = 1;
						break;
					}
					int removeIdx = rand() % UBall::TotalNumBalls;
					int lastIdx = UBall::TotalNumBalls - 1;

					UPrimitive* targetToDelete = PrimitiveList[removeIdx];

					if (removeIdx != lastIdx)
					{
						PrimitiveList[removeIdx] = PrimitiveList[lastIdx];
					}

					PrimitiveList[lastIdx] = nullptr;
					delete targetToDelete;
				}
			}
			else
			{
				// 동적 생성한 PrimitiveList 꽉찰 경우 배열 2배 후 복사 이동
				if (numberOfBalls >= Capacity)
				{
					int NewCapacity = Capacity * 2;
					while (numberOfBalls >= NewCapacity) NewCapacity *= 2;

					UPrimitive** NewList = new UPrimitive * [NewCapacity];
					for (int i = 0; i < NewCapacity; i++) NewList[i] = nullptr;

					if (PrimitiveList != nullptr)
					{
						for (int i = 0; i < Capacity; i++)
						{
							NewList[i] = PrimitiveList[i];
						}
						delete[] PrimitiveList;
					}

					PrimitiveList = NewList;
					Capacity = NewCapacity;
				}

				// 생성
				for (int currentIdx = UBall::TotalNumBalls; currentIdx < numberOfBalls; currentIdx++)
				{
					PrimitiveList[currentIdx] = new UBall();
				}
			}
		}
		ImGui::End();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// 렌더러 스왑
		renderer.SwapBuffer();

		// 프레임 관리
		do
		{
			Sleep(0);

			QueryPerformanceCounter(&endTime);

			elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;
		} while (elapsedTime < targetFrameTime);
	}

	// 소멸
	for (int i = 0; i < Capacity; i++)
	{
		if (PrimitiveList[i] != nullptr)
		{
			delete PrimitiveList[i];
			PrimitiveList[i] = nullptr;
		}
	}
	delete[] PrimitiveList;
	PrimitiveList = nullptr;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    renderer.ReleaseVertexBuffer(vertexBufferSphere);

    renderer.ReleaseConstantBuffer();
    renderer.ReleaseShader();
    renderer.Release();

    return 0;
}