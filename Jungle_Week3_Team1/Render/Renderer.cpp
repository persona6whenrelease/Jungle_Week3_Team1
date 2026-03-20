#include "Renderer.h"
#include <cassert>

using namespace DirectX;

// ============================================================
//  내부 변환 헬퍼
//
//  FMatrix  → XMFLOAT4X4  (HLSL Column-major 규칙 → Transpose)
//  FVector  → XMFLOAT3
//  FVector4 → XMFLOAT4
//
//  외부에서 넘어오는 FMatrix 는 Row-major (UE 관례) 로 가정.
//  HLSL cbuffer 는 Column-major 이므로 GPU 업로드 시 Transpose.
// ============================================================
DirectX::XMFLOAT4X4 D3D11Renderer::ToGPUMatrix(const FMatrix& mat)
{
    // FMatrix::M 은 [row][col] Row-major 배열이라 가정
    XMMATRIX xm(
        mat.M[0][0], mat.M[0][1], mat.M[0][2], mat.M[0][3],
        mat.M[1][0], mat.M[1][1], mat.M[1][2], mat.M[1][3],
        mat.M[2][0], mat.M[2][1], mat.M[2][2], mat.M[2][3],
        mat.M[3][0], mat.M[3][1], mat.M[3][2], mat.M[3][3]
    );
    XMFLOAT4X4 result;
    XMStoreFloat4x4(&result, XMMatrixTranspose(xm)); // HLSL Column-major
    return result;
}

DirectX::XMFLOAT3 D3D11Renderer::ToXMFLOAT3(const FVector& v)
{
    return XMFLOAT3(v.X, v.Y, v.Z);
}

DirectX::XMFLOAT4 D3D11Renderer::ToXMFLOAT4(const FVector4& v)
{
    return XMFLOAT4(v.X, v.Y, v.Z, v.W);
}

// ============================================================
//  Initialize
// ============================================================
bool D3D11Renderer::Initialize(const RendererDesc& desc)
{
    m_Width  = desc.Width;
    m_Height = desc.Height;

    if (!CreateDeviceAndSwapChain(desc))              return false;
    if (!CreateRenderTargetView())                    return false;
    if (!CreateDepthStencilBuffer(m_Width, m_Height)) return false;
    if (!CreateRasterizerStates())                    return false;
    if (!CreateDepthStencilStates())                  return false;
    if (!CreateBlendStates())                         return false;
    if (!CreateSamplerState())                        return false;
    if (!CreateConstantBuffers())                     return false;
    if (!CreateLineBatchBuffers())                    return false;
    if (!CreateBillboardBuffers())                    return false;

    SetViewport(m_Width, m_Height);

    // 기본 상태 바인딩
    m_Context->OMSetRenderTargets(1, m_RTV.GetAddressOf(), m_DSV.Get());
    m_Context->OMSetDepthStencilState(m_DSSDefault.Get(), 0);
    m_Context->OMSetBlendState(m_BSOpaque.Get(), nullptr, 0xFFFFFFFF);
    m_Context->RSSetState(m_RSSolid.Get());

    // b0 = CBPerFrame, b1 = CBPerObject : VS/PS 양쪽 고정 바인딩
    // 셰이더 교체 후에도 재바인딩 불필요
    ID3D11Buffer* cbs[] = { m_CBPerFrame.Get(), m_CBPerObject.Get() };
    m_Context->VSSetConstantBuffers(0, 2, cbs);
    m_Context->PSSetConstantBuffers(0, 2, cbs);

    // s0 슬롯에 Sampler 고정 바인딩 (Billboard PS 에서 사용)
    m_Context->PSSetSamplers(0, 1, m_SamplerLinear.GetAddressOf());

    return true;
}

// ============================================================
//  Shutdown
// ============================================================
void D3D11Renderer::Shutdown()
{
    if (m_Context)
    {
        m_Context->ClearState();
        m_Context->Flush();
    }
    m_ShaderPrograms.clear();
    m_LineCPUBuffer.clear();
    m_BillboardCPUBuffer.clear();
}

// ============================================================
//  ResizeBuffers
// ============================================================
bool D3D11Renderer::ResizeBuffers(uint32 width, uint32 height)
{
    if (width == 0 || height == 0) return false;

    m_Width  = width;
    m_Height = height;

    m_Context->OMSetRenderTargets(0, nullptr, nullptr);
    m_RTV.Reset();
    m_DSV.Reset();
    m_DepthStencilTexture.Reset();

    if (FAILED(m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0)))
        return false;

    if (!CreateRenderTargetView())                return false;
    if (!CreateDepthStencilBuffer(width, height)) return false;

    SetViewport(width, height);
    m_Context->OMSetRenderTargets(1, m_RTV.GetAddressOf(), m_DSV.Get());
    return true;
}

// ============================================================
//  BeginFrame / EndFrame
// ============================================================
void D3D11Renderer::BeginFrame(const FVector4& clearColor)
{
    const float c[4] = { clearColor.X, clearColor.Y, clearColor.Z, clearColor.W };
    m_Context->ClearRenderTargetView(m_RTV.Get(), c);
    m_Context->ClearDepthStencilView(m_DSV.Get(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_Context->OMSetRenderTargets(1, m_RTV.GetAddressOf(), m_DSV.Get());
    m_Context->RSSetViewports(1, &m_Viewport);
}

void D3D11Renderer::EndFrame()
{
    m_SwapChain->Present(1, 0); // VSync ON (0 이면 OFF)
}

// ============================================================
//  Constant Buffer 업로드
// ============================================================
void D3D11Renderer::UpdatePerFrameCB(const FMatrix& view,
                                     const FMatrix& proj,
                                     const FVector& cameraPos)
{
    // FMatrix 원본 캐시 (Billboard 에서 View Right/Up 벡터 추출에 사용)
    m_CurrentView = view;
    m_CurrentProj = proj;

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(m_Context->Map(m_CBPerFrame.Get(), 0,
                                 D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        CBPerFrame* cb  = reinterpret_cast<CBPerFrame*>(mapped.pData);
        cb->View        = ToGPUMatrix(view);
        cb->Projection  = ToGPUMatrix(proj);
        cb->CameraPos   = XMFLOAT4(cameraPos.X, cameraPos.Y, cameraPos.Z, 0.f);
        m_Context->Unmap(m_CBPerFrame.Get(), 0);
    }
}

void D3D11Renderer::UpdatePerObjectCB(const FMatrix&  world,
                                      const FVector4& highlightColor)
{
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(m_Context->Map(m_CBPerObject.Get(), 0,
                                 D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        CBPerObject* cb    = reinterpret_cast<CBPerObject*>(mapped.pData);
        cb->World          = ToGPUMatrix(world);
        cb->HighlightColor = ToXMFLOAT4(highlightColor);
        m_Context->Unmap(m_CBPerObject.Get(), 0);
    }
}

// ============================================================
//  DrawMesh
// ============================================================
void D3D11Renderer::DrawMesh(ID3D11Buffer*           vertexBuffer,
                             ID3D11Buffer*           indexBuffer,
                             uint32                  indexCount,
                             uint32                  vertexStride,
                             D3D11_PRIMITIVE_TOPOLOGY topology)
{
    UINT offset = 0;
    UINT stride = vertexStride;
    m_Context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    m_Context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_Context->IASetPrimitiveTopology(topology);
    m_Context->DrawIndexed(indexCount, 0, 0);
}

// ============================================================
//  Line Batch
// ============================================================
void D3D11Renderer::AddLine(const FVector&  start,
                            const FVector&  end,
                            const FVector4& color)
{
    if (m_LineCPUBuffer.size() + 2 > MAX_LINE_VERTICES) return;

    XMFLOAT4 c = ToXMFLOAT4(color);
    m_LineCPUBuffer.push_back({ ToXMFLOAT3(start), c });
    m_LineCPUBuffer.push_back({ ToXMFLOAT3(end),   c });
}

void D3D11Renderer::FlushLines()
{
    if (m_LineCPUBuffer.empty()) return;

    const UINT vertCount = (UINT)m_LineCPUBuffer.size();

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(m_Context->Map(m_LineVB.Get(), 0,
                                 D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, m_LineCPUBuffer.data(),
               vertCount * sizeof(FLineVertex));
        m_Context->Unmap(m_LineVB.Get(), 0);
    }
    else return;

    // 라인은 이미 World 좌표 → Identity World 행렬
    UpdatePerObjectCB(FMatrix::Identity);

    UINT stride = sizeof(FLineVertex);
    UINT offset = 0;
    m_Context->IASetVertexBuffers(0, 1, m_LineVB.GetAddressOf(), &stride, &offset);
    m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    m_Context->Draw(vertCount, 0);

    m_LineCPUBuffer.clear();
}

// ============================================================
//  Billboard Text
// ============================================================
bool D3D11Renderer::LoadFontAtlas(const FString& ddsPath)
{
    // [TODO] DDSTextureLoader (DirectXTK) 연결
    // CreateDDSTextureFromFile(m_Device.Get(),
    //     std::wstring(ddsPath.begin(), ddsPath.end()).c_str(),
    //     nullptr, m_FontAtlasSRV.GetAddressOf());
    return true; // 스텁
}

void D3D11Renderer::DrawBillboardText(const FVector&  worldPos,
                                      const FString&  text,
                                      const FVector4& color,
                                      float           charSize)
{
    float offsetX = 0.0f;
    const float halfTotal = (float)text.size() * charSize * 0.5f;

    for (char c : text)
    {
        if (m_BillboardCPUBuffer.size() + 4 > MAX_BILLBOARD_VERTS) break;

        int ascii = static_cast<unsigned char>(c);
        int col   = ascii % 16;
        int row   = ascii / 16;

        XMFLOAT2 uvMin = { col * FONT_CELL_U, row  * FONT_CELL_V };
        XMFLOAT2 uvMax = { uvMin.x + FONT_CELL_U, uvMin.y + FONT_CELL_V };

        FBillboardVertex verts[4];
        BuildBillboardQuad(worldPos, charSize, charSize,
                           offsetX - halfTotal,
                           uvMin, uvMax, color, verts);

        for (auto& v : verts)
            m_BillboardCPUBuffer.push_back(v);

        offsetX += charSize;
    }
}

void D3D11Renderer::FlushBillboardText()
{
    if (m_BillboardCPUBuffer.empty()) return;
    if (!m_FontAtlasSRV)              return;

    const UINT vertCount = (UINT)m_BillboardCPUBuffer.size();
    const UINT quadCount = vertCount / 4;

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(m_Context->Map(m_BillboardVB.Get(), 0,
                                 D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, m_BillboardCPUBuffer.data(),
               vertCount * sizeof(FBillboardVertex));
        m_Context->Unmap(m_BillboardVB.Get(), 0);
    }
    else return;

    m_Context->PSSetShaderResources(0, 1, m_FontAtlasSRV.GetAddressOf());

    // 쿼드가 이미 World 좌표 → Identity
    UpdatePerObjectCB(FMatrix::Identity);

    UINT stride = sizeof(FBillboardVertex);
    UINT offset = 0;
    m_Context->IASetVertexBuffers(0, 1, m_BillboardVB.GetAddressOf(), &stride, &offset);
    m_Context->IASetIndexBuffer(m_BillboardIB.Get(), DXGI_FORMAT_R32_UINT, 0);
    m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_Context->DrawIndexed(quadCount * 6, 0, 0);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_Context->PSSetShaderResources(0, 1, &nullSRV);

    m_BillboardCPUBuffer.clear();
}

// Billboard 헬퍼 : View 행렬에서 Right/Up 벡터 뽑아 쿼드 빌드
void D3D11Renderer::BuildBillboardQuad(const FVector&   worldPos,
                                       float            charW,
                                       float            charH,
                                       float            offsetX,
                                       XMFLOAT2         uvMin,
                                       XMFLOAT2         uvMax,
                                       const FVector4&  color,
                                       FBillboardVertex outVerts[4]) const
{
    // View 행렬 행(Row) = [Right | Up | Forward | ...]
    // FMatrix::M[row][col] 기준
    //   Right  = (M[0][0], M[1][0], M[2][0])  ← 첫 번째 열
    //   Up     = (M[0][1], M[1][1], M[2][1])  ← 두 번째 열
    XMVECTOR R = XMVectorSet(m_CurrentView.M[0][0],
                              m_CurrentView.M[1][0],
                              m_CurrentView.M[2][0], 0.f);
    XMVECTOR U = XMVectorSet(m_CurrentView.M[0][1],
                              m_CurrentView.M[1][1],
                              m_CurrentView.M[2][1], 0.f);
    XMVECTOR C = XMVectorSet(worldPos.X, worldPos.Y, worldPos.Z, 0.f);

    XMVECTOR hw     = XMVectorScale(R, charW  * 0.5f);
    XMVECTOR hh     = XMVectorScale(U, charH  * 0.5f);
    XMVECTOR ox     = XMVectorScale(R, offsetX + charW * 0.5f);
    XMVECTOR center = XMVectorAdd(C, ox);

    XMFLOAT3 bl, br, tl, tr;
    XMStoreFloat3(&bl, XMVectorSubtract(XMVectorSubtract(center, hw), hh));
    XMStoreFloat3(&br, XMVectorAdd(XMVectorSubtract(center, hh), hw));
    XMStoreFloat3(&tl, XMVectorAdd(XMVectorSubtract(center, hw), hh));
    XMStoreFloat3(&tr, XMVectorAdd(XMVectorAdd(center, hw), hh));

    XMFLOAT4 c = ToXMFLOAT4(color);
    outVerts[0] = { bl, { uvMin.x, uvMax.y }, c }; // BL
    outVerts[1] = { br, { uvMax.x, uvMax.y }, c }; // BR
    outVerts[2] = { tl, { uvMin.x, uvMin.y }, c }; // TL
    outVerts[3] = { tr, { uvMax.x, uvMin.y }, c }; // TR
}

// ============================================================
//  View Mode
// ============================================================
void D3D11Renderer::SetViewMode(EViewModeIndex mode)
{
    m_ViewMode = mode;
    if (mode == EViewModeIndex::VMI_Wireframe)
        m_Context->RSSetState(m_RSWireframe.Get());
    else
        m_Context->RSSetState(m_RSSolid.Get());
}

// ============================================================
//  Show Flags
// ============================================================
void D3D11Renderer::SetShowFlag(EEngineShowFlags flag, bool enable)
{
    uint64 bit = static_cast<uint64>(flag);
    if (enable) m_ShowFlags |=  bit;
    else        m_ShowFlags &= ~bit;
}

bool D3D11Renderer::HasShowFlag(EEngineShowFlags flag) const
{
    return (m_ShowFlags & static_cast<uint64>(flag)) != 0;
}

// ============================================================
//  Rasterizer State
// ============================================================
void D3D11Renderer::SetSolidRasterizer()     { m_Context->RSSetState(m_RSSolid.Get()); }
void D3D11Renderer::SetWireframeRasterizer() { m_Context->RSSetState(m_RSWireframe.Get()); }
void D3D11Renderer::SetNoCullRasterizer()    { m_Context->RSSetState(m_RSNoCull.Get()); }

// ============================================================
//  Depth Stencil State
// ============================================================
void D3D11Renderer::SetDSSDefault()             { m_Context->OMSetDepthStencilState(m_DSSDefault.Get(),      0); }
void D3D11Renderer::SetDSSNoWrite()             { m_Context->OMSetDepthStencilState(m_DSSNoWrite.Get(),      0); }
void D3D11Renderer::SetDSSAlways()              { m_Context->OMSetDepthStencilState(m_DSSAlways.Get(),       0); }
void D3D11Renderer::SetDSSStencilWrite(uint32 ref) { m_Context->OMSetDepthStencilState(m_DSSStencilWrite.Get(), ref); }
void D3D11Renderer::SetDSSStencilTest (uint32 ref) { m_Context->OMSetDepthStencilState(m_DSSStencilTest.Get(),  ref); }

// ============================================================
//  Blend State
// ============================================================
void D3D11Renderer::SetOpaqueBlend() { m_Context->OMSetBlendState(m_BSOpaque.Get(), nullptr, 0xFFFFFFFF); }
void D3D11Renderer::SetAlphaBlend()  { m_Context->OMSetBlendState(m_BSAlpha.Get(),  nullptr, 0xFFFFFFFF); }

// ============================================================
//  Shader
// ============================================================
bool D3D11Renderer::CompileShaderFromFile(const FString& path,
                                          const FString& entryPoint,
                                          const FString& target,
                                          ID3DBlob**     outBlob)
{
    // FString = std::string → wstring 변환
    std::wstring wPath(path.begin(), path.end());

    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompileFromFile(
        wPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint.c_str(), target.c_str(),
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0, outBlob, errorBlob.GetAddressOf());

    if (FAILED(hr))
    {
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return false;
    }
    return true;
}

bool D3D11Renderer::CompileShaderFromSource(const FString& source,
                                            const FString& entryPoint,
                                            const FString& target,
                                            ID3DBlob**     outBlob)
{
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompile(
        source.c_str(), source.size(), nullptr, nullptr, nullptr,
        entryPoint.c_str(), target.c_str(),
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0, outBlob, errorBlob.GetAddressOf());

    if (FAILED(hr))
    {
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return false;
    }
    return true;
}

bool D3D11Renderer::RegisterShaderProgram(const FString&                  name,
                                          ID3DBlob*                       vsBlob,
                                          ID3DBlob*                       psBlob,
                                          const D3D11_INPUT_ELEMENT_DESC* layoutDesc,
                                          uint32                          layoutCount)
{
    ShaderProgram prog;

    if (FAILED(m_Device->CreateVertexShader(vsBlob->GetBufferPointer(),
               vsBlob->GetBufferSize(), nullptr, prog.VS.GetAddressOf())))
        return false;

    if (FAILED(m_Device->CreatePixelShader(psBlob->GetBufferPointer(),
               psBlob->GetBufferSize(), nullptr, prog.PS.GetAddressOf())))
        return false;

    if (FAILED(m_Device->CreateInputLayout(layoutDesc, layoutCount,
               vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
               prog.InputLayout.GetAddressOf())))
        return false;

    m_ShaderPrograms[name] = std::move(prog);
    return true;
}

bool D3D11Renderer::BindShaderProgram(const FString& name)
{
    auto it = m_ShaderPrograms.find(name);
    if (it == m_ShaderPrograms.end()) return false;

    const ShaderProgram& p = it->second;
    m_Context->VSSetShader(p.VS.Get(), nullptr, 0);
    m_Context->PSSetShader(p.PS.Get(), nullptr, 0);
    m_Context->IASetInputLayout(p.InputLayout.Get());
    return true;
}

// ============================================================
//  [Private] CreateDeviceAndSwapChain
// ============================================================
bool D3D11Renderer::CreateDeviceAndSwapChain(const RendererDesc& desc)
{
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferDesc.Width                   = desc.Width;
    scd.BufferDesc.Height                  = desc.Height;
    scd.BufferDesc.RefreshRate.Numerator   = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.SampleDesc.Count                   = desc.SampleCount;
    scd.SampleDesc.Quality                 = 0;
    scd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount                        = 1;
    scd.OutputWindow                       = desc.hWnd;
    scd.Windowed                           = desc.Windowed ? TRUE : FALSE;
    scd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
    scd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL selectedLevel   = {};

    UINT createFlags = 0;
#if defined(_DEBUG)
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    return SUCCEEDED(D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createFlags, featureLevels, ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION, &scd,
        m_SwapChain.GetAddressOf(),
        m_Device.GetAddressOf(),
        &selectedLevel,
        m_Context.GetAddressOf()));
}

// ============================================================
//  [Private] CreateRenderTargetView
// ============================================================
bool D3D11Renderer::CreateRenderTargetView()
{
    ComPtr<ID3D11Texture2D> backBuffer;
    if (FAILED(m_SwapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()))))
        return false;

    return SUCCEEDED(m_Device->CreateRenderTargetView(
        backBuffer.Get(), nullptr, m_RTV.GetAddressOf()));
}

// ============================================================
//  [Private] CreateDepthStencilBuffer
// ============================================================
bool D3D11Renderer::CreateDepthStencilBuffer(uint32 width, uint32 height)
{
    D3D11_TEXTURE2D_DESC td{};
    td.Width            = width;
    td.Height           = height;
    td.MipLevels        = 1;
    td.ArraySize        = 1;
    td.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT; // Depth 24 + Stencil 8
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_DEFAULT;
    td.BindFlags        = D3D11_BIND_DEPTH_STENCIL;

    if (FAILED(m_Device->CreateTexture2D(&td, nullptr,
                                         m_DepthStencilTexture.GetAddressOf())))
        return false;

    return SUCCEEDED(m_Device->CreateDepthStencilView(
        m_DepthStencilTexture.Get(), nullptr, m_DSV.GetAddressOf()));
}

// ============================================================
//  [Private] CreateRasterizerStates
// ============================================================
bool D3D11Renderer::CreateRasterizerStates()
{
    auto Make = [&](D3D11_FILL_MODE fill, D3D11_CULL_MODE cull,
                    ID3D11RasterizerState** pp) -> bool
    {
        D3D11_RASTERIZER_DESC rd{};
        rd.FillMode              = fill;
        rd.CullMode              = cull;
        rd.FrontCounterClockwise = FALSE;
        rd.DepthClipEnable       = TRUE;
        return SUCCEEDED(m_Device->CreateRasterizerState(&rd, pp));
    };

    if (!Make(D3D11_FILL_SOLID,     D3D11_CULL_BACK, m_RSSolid.GetAddressOf()))     return false;
    if (!Make(D3D11_FILL_WIREFRAME, D3D11_CULL_NONE, m_RSWireframe.GetAddressOf())) return false;
    if (!Make(D3D11_FILL_SOLID,     D3D11_CULL_NONE, m_RSNoCull.GetAddressOf()))    return false;
    return true;
}

// ============================================================
//  [Private] CreateDepthStencilStates
// ============================================================
bool D3D11Renderer::CreateDepthStencilStates()
{
    // Default : Z-Test O / Z-Write O
    {
        D3D11_DEPTH_STENCIL_DESC d{};
        d.DepthEnable    = TRUE;
        d.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        d.DepthFunc      = D3D11_COMPARISON_LESS;
        if (FAILED(m_Device->CreateDepthStencilState(&d, m_DSSDefault.GetAddressOf())))
            return false;
    }

    // NoWrite : Z-Test O / Z-Write X (Gizmo)
    {
        D3D11_DEPTH_STENCIL_DESC d{};
        d.DepthEnable    = TRUE;
        d.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        d.DepthFunc      = D3D11_COMPARISON_LESS;
        if (FAILED(m_Device->CreateDepthStencilState(&d, m_DSSNoWrite.GetAddressOf())))
            return false;
    }

    // Always : Z-Test X (World Axis, Billboard)
    {
        D3D11_DEPTH_STENCIL_DESC d{};
        d.DepthEnable    = FALSE;
        d.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        if (FAILED(m_Device->CreateDepthStencilState(&d, m_DSSAlways.GetAddressOf())))
            return false;
    }

    // StencilWrite : Highlight 1단계 - Stencil = ref 기록
    {
        D3D11_DEPTH_STENCIL_DESC d{};
        d.DepthEnable      = TRUE;
        d.DepthWriteMask   = D3D11_DEPTH_WRITE_MASK_ALL;
        d.DepthFunc        = D3D11_COMPARISON_LESS;
        d.StencilEnable    = TRUE;
        d.StencilReadMask  = 0xFF;
        d.StencilWriteMask = 0xFF;
        d.FrontFace = d.BackFace = {
            D3D11_STENCIL_OP_KEEP,
            D3D11_STENCIL_OP_KEEP,
            D3D11_STENCIL_OP_REPLACE,   // 통과 시 ref 기록
            D3D11_COMPARISON_ALWAYS
        };
        if (FAILED(m_Device->CreateDepthStencilState(&d, m_DSSStencilWrite.GetAddressOf())))
            return false;
    }

    // StencilTest : Highlight 2단계 - Stencil != ref 인 픽셀만 외곽선
    {
        D3D11_DEPTH_STENCIL_DESC d{};
        d.DepthEnable      = FALSE;
        d.DepthWriteMask   = D3D11_DEPTH_WRITE_MASK_ZERO;
        d.StencilEnable    = TRUE;
        d.StencilReadMask  = 0xFF;
        d.StencilWriteMask = 0xFF;
        d.FrontFace = d.BackFace = {
            D3D11_STENCIL_OP_KEEP,
            D3D11_STENCIL_OP_KEEP,
            D3D11_STENCIL_OP_KEEP,
            D3D11_COMPARISON_NOT_EQUAL  // Stencil != ref 픽셀만 통과
        };
        if (FAILED(m_Device->CreateDepthStencilState(&d, m_DSSStencilTest.GetAddressOf())))
            return false;
    }

    return true;
}

// ============================================================
//  [Private] CreateBlendStates
// ============================================================
bool D3D11Renderer::CreateBlendStates()
{
    // Opaque
    {
        D3D11_BLEND_DESC bd{};
        bd.RenderTarget[0].BlendEnable           = FALSE;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(m_Device->CreateBlendState(&bd, m_BSOpaque.GetAddressOf())))
            return false;
    }

    // Alpha (Gizmo, Billboard 반투명)
    {
        D3D11_BLEND_DESC bd{};
        auto& rt             = bd.RenderTarget[0];
        rt.BlendEnable           = TRUE;
        rt.SrcBlend              = D3D11_BLEND_SRC_ALPHA;
        rt.DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
        rt.BlendOp               = D3D11_BLEND_OP_ADD;
        rt.SrcBlendAlpha         = D3D11_BLEND_ONE;
        rt.DestBlendAlpha        = D3D11_BLEND_ZERO;
        rt.BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(m_Device->CreateBlendState(&bd, m_BSAlpha.GetAddressOf())))
            return false;
    }

    return true;
}

// ============================================================
//  [Private] CreateSamplerState
// ============================================================
bool D3D11Renderer::CreateSamplerState()
{
    D3D11_SAMPLER_DESC sd{};
    sd.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD         = 0;
    sd.MaxLOD         = D3D11_FLOAT32_MAX;
    return SUCCEEDED(m_Device->CreateSamplerState(&sd, m_SamplerLinear.GetAddressOf()));
}

// ============================================================
//  [Private] CreateConstantBuffers
// ============================================================
bool D3D11Renderer::CreateConstantBuffers()
{
    auto MakeCB = [&](UINT size, ID3D11Buffer** pp) -> bool
    {
        assert(size % 16 == 0);
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth      = size;
        bd.Usage          = D3D11_USAGE_DYNAMIC;
        bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        return SUCCEEDED(m_Device->CreateBuffer(&bd, nullptr, pp));
    };

    if (!MakeCB(sizeof(CBPerFrame),  m_CBPerFrame.GetAddressOf()))  return false;
    if (!MakeCB(sizeof(CBPerObject), m_CBPerObject.GetAddressOf())) return false;
    return true;
}

// ============================================================
//  [Private] CreateLineBatchBuffers
// ============================================================
bool D3D11Renderer::CreateLineBatchBuffers()
{
    m_LineCPUBuffer.reserve(MAX_LINE_VERTICES);

    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth      = sizeof(FLineVertex) * MAX_LINE_VERTICES;
    bd.Usage          = D3D11_USAGE_DYNAMIC;
    bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    return SUCCEEDED(m_Device->CreateBuffer(&bd, nullptr, m_LineVB.GetAddressOf()));
}

// ============================================================
//  [Private] CreateBillboardBuffers
// ============================================================
bool D3D11Renderer::CreateBillboardBuffers()
{
    m_BillboardCPUBuffer.reserve(MAX_BILLBOARD_VERTS);

    // Dynamic VB
    {
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth      = sizeof(FBillboardVertex) * MAX_BILLBOARD_VERTS;
        bd.Usage          = D3D11_USAGE_DYNAMIC;
        bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(m_Device->CreateBuffer(&bd, nullptr, m_BillboardVB.GetAddressOf())))
            return false;
    }

    // Static IB : quad 당 0,1,2, 1,3,2 패턴
    {
        const uint32 maxQuads = MAX_BILLBOARD_VERTS / 4;
        TArray<uint32> indices;
        indices.reserve(maxQuads * 6);
        for (uint32 i = 0; i < maxQuads; ++i)
        {
            uint32 base = i * 4;
            indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
            indices.push_back(base + 1); indices.push_back(base + 3); indices.push_back(base + 2);
        }

        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = sizeof(uint32) * (uint32)indices.size();
        bd.Usage     = D3D11_USAGE_IMMUTABLE;
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA init{ indices.data(), 0, 0 };
        if (FAILED(m_Device->CreateBuffer(&bd, &init, m_BillboardIB.GetAddressOf())))
            return false;
    }

    return true;
}

// ============================================================
//  [Private] SetViewport
// ============================================================
void D3D11Renderer::SetViewport(uint32 width, uint32 height)
{
    m_Viewport = { 0.f, 0.f, (float)width, (float)height, 0.f, 1.f };
    m_Context->RSSetViewports(1, &m_Viewport);
}
