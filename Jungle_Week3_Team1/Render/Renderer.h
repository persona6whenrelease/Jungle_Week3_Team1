#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <unordered_map>

// 프로젝트 공용 타입
#include "CoreMinimal.h"   // FMatrix, FVector, FVector4, FQuat ...

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

// ============================================================
//  View Mode
// ============================================================
enum class EViewModeIndex : uint32_t
{
    VMI_Lit,
    VMI_Unlit,
    VMI_Wireframe,
};

// ============================================================
//  Show Flags  (비트 플래그)
// ============================================================
enum class EEngineShowFlags : uint64_t
{
    SF_Primitives    = 1ULL << 0,
    SF_BillboardText = 1ULL << 1,
    SF_Grid          = 1ULL << 2,
    SF_BoundingBox   = 1ULL << 3,
    SF_Axis          = 1ULL << 4,
};

// ============================================================
//  GPU Constant Buffer 구조체
//  - HLSL cbuffer 규칙 : 반드시 16 바이트 배수
//  - GPU 에 직접 업로드되므로 XMFLOAT 계열 사용
//  - 외부에서는 FMatrix / FVector / FVector4 로 전달하고
//    내부에서 변환한다
// ============================================================
struct CBPerFrame
{
    DirectX::XMFLOAT4X4 View;
    DirectX::XMFLOAT4X4 Projection;
    DirectX::XMFLOAT4   CameraPos;  // w = padding
};

struct CBPerObject
{
    DirectX::XMFLOAT4X4 World;
    DirectX::XMFLOAT4   HighlightColor; // w=0 이면 하이라이트 없음
};

// ============================================================
//  Vertex 포맷  (GPU 업로드용이므로 XMFLOAT 계열 사용)
// ============================================================
struct FVertex          // 일반 메시
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 UV;
    DirectX::XMFLOAT4 Color;
};

struct FLineVertex      // Line Batch
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT4 Color;
};

struct FBillboardVertex // Billboard Text
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT2 UV;
    DirectX::XMFLOAT4 Color;
};

// ============================================================
//  Shader 프로그램
// ============================================================
struct ShaderProgram
{
    ComPtr<ID3D11VertexShader> VS;
    ComPtr<ID3D11PixelShader>  PS;
    ComPtr<ID3D11InputLayout>  InputLayout;
};

// ============================================================
//  초기화 설정
// ============================================================
struct RendererDesc
{
    HWND hWnd        = nullptr;
    uint32 Width     = 1280;
    uint32 Height    = 720;
    bool   Windowed  = true;
    uint32 SampleCount = 1;
};

// ============================================================
//  D3D11Renderer
//
//  [담당]
//    Device / Context / SwapChain 소유
//    RenderState / DepthStencilState / BlendState 관리
//    Constant Buffer 업로드 (b0=PerFrame, b1=PerObject)
//    Shader 컴파일 · 등록 · 바인딩
//    DrawMesh        : 일반 메시 드로우
//    Line Batch      : AddLine → FlushLines
//    Billboard Text  : DrawBillboardText → FlushBillboardText
//    ViewMode / ShowFlags 상태 관리
//    ResizeBuffers
//
//  [담당 안 함]
//    카메라 행렬 계산, Gizmo 교차, 입력 처리, ini 저장/로드
//
//  [타입 규칙]
//    공개 API  → FMatrix / FVector / FVector4  (프로젝트 공용 타입)
//    GPU 내부  → XMFLOAT / XMVECTOR           (변환 후 업로드)
// ============================================================
class D3D11Renderer
{
public:
    D3D11Renderer()  = default;
    ~D3D11Renderer() = default;

    D3D11Renderer(const D3D11Renderer&)            = delete;
    D3D11Renderer& operator=(const D3D11Renderer&) = delete;

    // ----------------------------------------------------------
    //  Life-cycle
    // ----------------------------------------------------------
    bool Initialize(const RendererDesc& desc);
    void Shutdown();
    bool ResizeBuffers(uint32 width, uint32 height);

    // ----------------------------------------------------------
    //  프레임 제어
    // ----------------------------------------------------------
    void BeginFrame(const FVector4& clearColor = FVector4(0.1f, 0.1f, 0.1f, 1.0f));
    void EndFrame();

    // ----------------------------------------------------------
    //  Constant Buffer 업로드
    //  PerFrame  → 카메라가 매 프레임 시작 시 1회 호출
    //  PerObject → DrawMesh 직전 오브젝트마다 호출
    // ----------------------------------------------------------
    void UpdatePerFrameCB(const FMatrix&  view,
                          const FMatrix&  proj,
                          const FVector&  cameraPos);

    void UpdatePerObjectCB(const FMatrix&  world,
                           const FVector4& highlightColor = FVector4(0,0,0,0));

    // ----------------------------------------------------------
    //  DrawMesh  :  UpdatePerObjectCB 호출 후 사용
    // ----------------------------------------------------------
    void DrawMesh(ID3D11Buffer*            vertexBuffer,
                  ID3D11Buffer*            indexBuffer,
                  uint32                   indexCount,
                  uint32                   vertexStride,
                  D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ----------------------------------------------------------
    //  Line Batch  (Grid / BoundingBox / Axis 모두 이 API 사용)
    //
    //  순서:
    //    1) AddLine() 을 원하는 만큼 호출
    //    2) FlushLines() 로 한 번에 GPU 업로드 + 드로우
    // ----------------------------------------------------------
    void AddLine(const FVector&  start,
                 const FVector&  end,
                 const FVector4& color);

    void FlushLines();

    // ----------------------------------------------------------
    //  Billboard Text
    //
    //  사전 준비:
    //    LoadFontAtlas() 로 DDS 텍스처 로드 (16x16 ASCII 그리드)
    //
    //  매 프레임 순서:
    //    1) UpdatePerFrameCB() 로 View 행렬 갱신
    //    2) DrawBillboardText() 를 오브젝트 수만큼 호출
    //    3) FlushBillboardText() 로 한 번에 드로우
    // ----------------------------------------------------------
    bool LoadFontAtlas(const FString& ddsPath);

    void DrawBillboardText(const FVector&  worldPos,
                           const FString&  text,
                           const FVector4& color    = FVector4(1,1,1,1),
                           float           charSize = 0.15f);

    void FlushBillboardText();

    // ----------------------------------------------------------
    //  View Mode  (VMI_Wireframe 전환 시 RS 상태 자동 변경)
    // ----------------------------------------------------------
    void           SetViewMode(EViewModeIndex mode);
    EViewModeIndex GetViewMode() const { return m_ViewMode; }

    // ----------------------------------------------------------
    //  Show Flags
    //  ex) renderer->SetShowFlag(EEngineShowFlags::SF_Grid, true);
    //      if (renderer->HasShowFlag(EEngineShowFlags::SF_Grid)) { ... }
    // ----------------------------------------------------------
    void SetShowFlag(EEngineShowFlags flag, bool enable);
    bool HasShowFlag(EEngineShowFlags flag) const;

    // ----------------------------------------------------------
    //  Rasterizer State
    // ----------------------------------------------------------
    void SetSolidRasterizer();
    void SetWireframeRasterizer();
    void SetNoCullRasterizer();

    // ----------------------------------------------------------
    //  Depth Stencil State
    // ----------------------------------------------------------
    void SetDSSDefault();                    // 일반 오브젝트 (Z-Test O / Z-Write O)
    void SetDSSNoWrite();                    // Gizmo        (Z-Test O / Z-Write X)
    void SetDSSAlways();                     // World Axis, Billboard (Z 무시)
    void SetDSSStencilWrite(uint32 ref = 1); // Highlight 1단계 (Stencil 기록)
    void SetDSSStencilTest (uint32 ref = 1); // Highlight 2단계 (외곽선 출력)

    // ----------------------------------------------------------
    //  Blend State
    // ----------------------------------------------------------
    void SetOpaqueBlend();
    void SetAlphaBlend();

    // ----------------------------------------------------------
    //  Shader
    // ----------------------------------------------------------
    bool CompileShaderFromFile(const FString& path,
                               const FString& entryPoint,
                               const FString& target,
                               ID3DBlob**     outBlob);

    bool CompileShaderFromSource(const FString& source,
                                 const FString& entryPoint,
                                 const FString& target,
                                 ID3DBlob**     outBlob);

    bool RegisterShaderProgram(const FString&                  name,
                               ID3DBlob*                       vsBlob,
                               ID3DBlob*                       psBlob,
                               const D3D11_INPUT_ELEMENT_DESC* layoutDesc,
                               uint32                          layoutCount);

    bool BindShaderProgram(const FString& name);

    // ----------------------------------------------------------
    //  접근자
    // ----------------------------------------------------------
    ID3D11Device*        GetDevice()      const { return m_Device.Get(); }
    ID3D11DeviceContext* GetContext()     const { return m_Context.Get(); }
    uint32               GetWidth()       const { return m_Width; }
    uint32               GetHeight()      const { return m_Height; }
    float                GetAspectRatio() const { return (float)m_Width / (float)m_Height; }

    // Billboard 에서 View 행렬을 참조하기 위해 캐싱된 값 제공
    const FMatrix& GetCurrentView() const { return m_CurrentView; }
    const FMatrix& GetCurrentProj() const { return m_CurrentProj; }

private:
    // ----------------------------------------------------------
    //  내부 초기화
    // ----------------------------------------------------------
    bool CreateDeviceAndSwapChain(const RendererDesc& desc);
    bool CreateRenderTargetView();
    bool CreateDepthStencilBuffer(uint32 width, uint32 height);
    bool CreateRasterizerStates();
    bool CreateDepthStencilStates();
    bool CreateBlendStates();
    bool CreateSamplerState();
    bool CreateConstantBuffers();
    bool CreateLineBatchBuffers();
    bool CreateBillboardBuffers();
    void SetViewport(uint32 width, uint32 height);

    // Billboard 쿼드 1개 빌드 (View 행렬 Right/Up 벡터 기반)
    void BuildBillboardQuad(const FVector&  worldPos,
                            float           charW,
                            float           charH,
                            float           offsetX,
                            DirectX::XMFLOAT2 uvMin,
                            DirectX::XMFLOAT2 uvMax,
                            const FVector4& color,
                            FBillboardVertex outVerts[4]) const;

    // FMatrix → XMFLOAT4X4  (GPU 업로드용, Transpose 포함)
    static DirectX::XMFLOAT4X4 ToGPUMatrix(const FMatrix& mat);
    // FVector → XMFLOAT3
    static DirectX::XMFLOAT3   ToXMFLOAT3(const FVector& v);
    // FVector4 → XMFLOAT4
    static DirectX::XMFLOAT4   ToXMFLOAT4(const FVector4& v);

    // ---- 코어 ----
    ComPtr<ID3D11Device>           m_Device;
    ComPtr<ID3D11DeviceContext>    m_Context;
    ComPtr<IDXGISwapChain>         m_SwapChain;

    ComPtr<ID3D11RenderTargetView> m_RTV;
    ComPtr<ID3D11Texture2D>        m_DepthStencilTexture;
    ComPtr<ID3D11DepthStencilView> m_DSV;

    // ---- Render States ----
    ComPtr<ID3D11RasterizerState>   m_RSSolid;
    ComPtr<ID3D11RasterizerState>   m_RSWireframe;
    ComPtr<ID3D11RasterizerState>   m_RSNoCull;

    ComPtr<ID3D11DepthStencilState> m_DSSDefault;
    ComPtr<ID3D11DepthStencilState> m_DSSNoWrite;
    ComPtr<ID3D11DepthStencilState> m_DSSAlways;
    ComPtr<ID3D11DepthStencilState> m_DSSStencilWrite;
    ComPtr<ID3D11DepthStencilState> m_DSSStencilTest;

    ComPtr<ID3D11BlendState>        m_BSOpaque;
    ComPtr<ID3D11BlendState>        m_BSAlpha;

    ComPtr<ID3D11SamplerState>      m_SamplerLinear;

    // ---- Constant Buffer ----
    ComPtr<ID3D11Buffer> m_CBPerFrame;
    ComPtr<ID3D11Buffer> m_CBPerObject;

    // Billboard 에서 View 행렬 참조용 캐시 (FMatrix 원본)
    FMatrix m_CurrentView;
    FMatrix m_CurrentProj;

    // ---- Line Batch ----
    static constexpr uint32 MAX_LINE_VERTICES = 131072; // 최대 65536 라인
    ComPtr<ID3D11Buffer>     m_LineVB;
    TArray<FLineVertex>      m_LineCPUBuffer;

    // ---- Billboard ----
    static constexpr uint32 MAX_BILLBOARD_VERTS = 16384; // 최대 4096 문자
    ComPtr<ID3D11Buffer>                  m_BillboardVB;
    ComPtr<ID3D11Buffer>                  m_BillboardIB;
    ComPtr<ID3D11ShaderResourceView>      m_FontAtlasSRV;
    TArray<FBillboardVertex>              m_BillboardCPUBuffer;

    static constexpr float FONT_CELL_U = 1.0f / 16.0f;
    static constexpr float FONT_CELL_V = 1.0f / 16.0f;

    // ---- Shader Map ----
    TMap<FString, ShaderProgram> m_ShaderPrograms;

    // ---- View Mode / Show Flags ----
    EViewModeIndex m_ViewMode  = EViewModeIndex::VMI_Lit;
    uint64         m_ShowFlags = ~0ULL; // 기본: 전부 ON

    // ---- Viewport ----
    D3D11_VIEWPORT m_Viewport = {};
    uint32         m_Width    = 0;
    uint32         m_Height   = 0;
};
