#pragma once
struct FVertexSimple
{
	float x, y, z;
	float r, g, b, a;
};

struct FVector
{
	float x, y, z;
	FVector(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
};

#include "Sphere.h"

class URenderer
{
public:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;
	ID3D11RasterizerState* RasterizerState = nullptr;
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	ID3D11VertexShader* SimpleVertexShader = nullptr;
	ID3D11PixelShader* SimplePixelShader = nullptr;
	ID3D11InputLayout* SimpleInputLayout = nullptr;
	unsigned int Stride;

	D3D11_VIEWPORT ViewportInfo;

	struct FConstants
	{
		FVector Offset;
		float Radius;
	};
	ID3D11Buffer* ConstantBuffer = nullptr;


public:
	void Create(HWND hWindow)
	{
		CreateDeviceAndSwapChain(hWindow);
		CreateFrameBuffer();
		CreateRasterizerState();
	}

	void CreateDeviceAndSwapChain(HWND hWindow)
	{
		D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

		DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
		swapchaindesc.BufferDesc.Width = 0;
		swapchaindesc.BufferDesc.Height = 0;
		swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapchaindesc.SampleDesc.Count = 1;	// 계단 현상을 위한 샘플링 비활성화
		swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// 렌더링 타겟 결과물
		swapchaindesc.BufferCount = 2;		// 더블 버퍼링
		swapchaindesc.OutputWindow = hWindow;
		swapchaindesc.Windowed = TRUE;	// 창모드
		swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// 주소 변경, 썼던거 버림.

		D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
			featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
			&swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

		SwapChain->GetDesc(&swapchaindesc);

		ViewportInfo = { 0, 0, float(swapchaindesc.BufferDesc.Width), float(swapchaindesc.BufferDesc.Height), 0, 1 };
	}

	void ReleaseDeviceAndSwapChain()
	{
		if (DeviceContext)
		{
			DeviceContext->Flush();
		}

		if (SwapChain)
		{
			SwapChain->Release();
			SwapChain = nullptr;
		}

		if (Device)
		{
			Device->Release();
			Device = nullptr;
		}

		if (DeviceContext)
		{
			DeviceContext->Release();
			DeviceContext = nullptr;
		}
	}

	void CreateFrameBuffer()
	{
		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

		CD3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
		framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
	}

	void ReleaseFrameBuffer()
	{
		if (FrameBuffer)
		{
			FrameBuffer->Release();
			FrameBuffer = nullptr;
		}

		if (FrameBufferRTV)
		{
			FrameBufferRTV->Release();
			FrameBufferRTV = nullptr;
		}
	}

	void CreateRasterizerState()
	{
		D3D11_RASTERIZER_DESC rasterizerdesc = {};
		rasterizerdesc.FillMode = D3D11_FILL_SOLID;
		rasterizerdesc.CullMode = D3D11_CULL_BACK;

		Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
	}

	void ReleaseRasterizerState()
	{
		if (RasterizerState)
		{
			RasterizerState->Release();
			RasterizerState = nullptr;
		}
	}

	void CreateConstantBuffer()
	{
		D3D11_BUFFER_DESC constantbufferdesc = {};
		constantbufferdesc.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0;
		constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
		constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
	}

	void ReleaseConstantBuffer()
	{
		if (ConstantBuffer)
		{
			ConstantBuffer->Release();
			ConstantBuffer = nullptr;
		}
	}

	void UpdateConstant(FVector Offset, float Radius)
	{
		if (ConstantBuffer)
		{
			D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

			DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
			FConstants* constants = (FConstants*)constantbufferMSR.pData;
			{
				constants->Offset = Offset;
				constants->Radius = Radius;
			}
			DeviceContext->Unmap(ConstantBuffer, 0);
		}
	}


	void CreateShader()
	{
		ID3DBlob* vertexshaderCSO;
		ID3DBlob* pixelshaderCSO;
		D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexshaderCSO, nullptr);

		Device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);

		D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelshaderCSO, nullptr);

		Device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);

		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), &SimpleInputLayout);

		Stride = sizeof(FVertexSimple);

		vertexshaderCSO->Release();
		pixelshaderCSO->Release();
	}

	void ReleaseShader()
	{
		if (SimpleInputLayout)
		{
			SimpleInputLayout->Release();
			SimpleInputLayout = nullptr;
		}

		if (SimplePixelShader)
		{
			SimplePixelShader->Release();
			SimplePixelShader = nullptr;
		}

		if (SimpleVertexShader)
		{
			SimpleVertexShader->Release();
			SimpleVertexShader = nullptr;
		}
	}

	ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT ByteWidth)
	{
		D3D11_BUFFER_DESC vertexbufferdesc = {};
		vertexbufferdesc.ByteWidth = ByteWidth;
		vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

		ID3D11Buffer* vertexBuffer = nullptr;

		Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);

		return vertexBuffer;
	}

	void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
	{
		vertexBuffer->Release();
	}

	void Release()
	{
		RasterizerState->Release();

		DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

		ReleaseFrameBuffer();
		ReleaseDeviceAndSwapChain();
	}

	void Prepare()
	{
		DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		DeviceContext->RSSetViewports(1, &ViewportInfo);
		DeviceContext->RSSetState(RasterizerState);

		DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
		DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	}

	void PrepareShader()
	{
		DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
		DeviceContext->IASetInputLayout(SimpleInputLayout);

		if (ConstantBuffer)
		{
			DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
		}
	}

	void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)
	{
		UINT offset = 0;
		DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);

		DeviceContext->Draw(numVertices, 0);
	}

	void SwapBuffer()
	{
		SwapChain->Present(1, 0);
	}
};
class UPrimitive
{
public:
	virtual void Move() {}
	virtual void MoveGravity(float gravityStrength) {}
	virtual void BorderToScreen(float sphereRadius, float leftBorder, float rightBorder, float topBorder, float bottomBorder) {}
	virtual void CollisionCalculation(UPrimitive* other) {}
	virtual ~UPrimitive() {}
};

class UBall : public UPrimitive
{
public:
	FVector Location;
	FVector Velocity;
	float Radius;
	float Mass;
	static int TotalNumBalls;

	UBall()
	{
		Location.x = ((float)(rand() % 100 - 50)) * 0.01f;
		Location.y = ((float)(rand() % 100 - 50)) * 0.01f;
		Velocity.x = ((float)(rand() % 100 - 50)) * 0.001f;
		Velocity.y = ((float)(rand() % 100 - 50)) * 0.001f;
		Radius = ((float)(rand() % 50)) * 0.003f + 0.03f;
		Mass = Radius * 1000.0f;

		TotalNumBalls++;
	}

	void CollisionCalculation(UPrimitive* other) override
	{
		UBall* otherBall = (UBall*)other;
		if (!otherBall || otherBall == this) return;

		float dx = otherBall->Location.x - Location.x;
		float dy = otherBall->Location.y - Location.y;
		float distanceSq = dx * dx + dy * dy;
		float minDistance = Radius + otherBall->Radius;

		if (distanceSq < minDistance * minDistance)
		{
			float distance = sqrt(distanceSq);
			float nx = dx / distance;
			float ny = dy / distance;

			// 겹침 처리
			float overlap = minDistance - distance;
			const float overlapRatio = 0.5f;

			Location.x -= nx * overlap * overlapRatio;
			Location.y -= ny * overlap * overlapRatio;
			otherBall->Location.x += nx * overlap * overlapRatio;
			otherBall->Location.y += ny * overlap * overlapRatio;


			float v12x = Velocity.x - otherBall->Velocity.x;
			float v12y = Velocity.y - otherBall->Velocity.y;

			float x12x = -dx;
			float x12y = -dy;

			float dot = (v12x * x12x) + (v12y * x12y);

			// 선처리 예외
			if (dot > 0) return;

			float m1 = Mass;
			float m2 = otherBall->Mass;

			float commonFactor = dot / distanceSq;

			// this 속도 계산
			float v1Impulse = (2 * m2) / (m1 + m2) * commonFactor;
			Velocity.x -= v1Impulse * x12x;
			Velocity.y -= v1Impulse * x12y;

			// other 속도 계산
			float v2Impulse = (2 * m1) / (m1 + m2) * commonFactor;
			otherBall->Velocity.x -= v2Impulse * (-x12x);
			otherBall->Velocity.y -= v2Impulse * (-x12y);
		}
	}

	// 속력 계산
	void Move() override
	{
		Location.x += Velocity.x;
		Location.y += Velocity.y;
		Location.z += Velocity.z;
	}

	// 중력 계산
	void MoveGravity(float gravityStrength) override
	{
		Velocity.y -= gravityStrength;
	}

	// 경계창에 의한 반사
	void BorderToScreen(float sphereRadius, float leftBorder, float rightBorder, float topBorder, float bottomBorder)
	{
		float renderRadius = sphereRadius * Radius;
		if (Location.x < leftBorder + renderRadius)
		{
			Velocity.x *= -1.0f;
			Location.x = leftBorder + renderRadius;
		}
		if (Location.x > rightBorder - renderRadius)
		{
			Velocity.x *= -1.0f;
			Location.x = rightBorder - renderRadius;
		}
		if (Location.y < bottomBorder + renderRadius)
		{
			Velocity.y *= -1.0f;
			Location.y = bottomBorder + renderRadius;
		}
		if (Location.y > topBorder - renderRadius)
		{
			Velocity.y *= -1.0f;
			Location.y = topBorder - renderRadius;
		}
	}

	~UBall()
	{
		TotalNumBalls--;
	}
};

int UBall::TotalNumBalls = 0;
