#include "pch.h"


#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

struct ConstantBuffer
{
    XMMATRIX mWorldViewProj; // Combine World * View * Projection
};

// Cube vertices (position and color) from Microsoft example:contentReference[oaicite:2]{index=2}:
struct Vertex { XMFLOAT3 pos; XMFLOAT3 color; };
Vertex CubeVertices[] = {
    { XMFLOAT3(-0.5f,-0.5f,-0.5f), {0,0,0} },
    { XMFLOAT3(-0.5f,-0.5f, 0.5f), {0,0,1} },
    { XMFLOAT3(-0.5f, 0.5f,-0.5f), {0,1,0} },
    { XMFLOAT3(-0.5f, 0.5f, 0.5f), {0,1,1} },
    { XMFLOAT3(0.5f,-0.5f,-0.5f), {1,0,0} },
    { XMFLOAT3(0.5f,-0.5f, 0.5f), {1,0,1} },
    { XMFLOAT3(0.5f, 0.5f,-0.5f), {1,1,0} },
    { XMFLOAT3(0.5f, 0.5f, 0.5f), {1,1,1} },
};
unsigned short CubeIndices[] = {
    0,2,1,  1,2,3,   // -X face
    4,5,6,  5,7,6,   // +X face
    0,1,5,  0,5,4,   // -Y (bottom)
    2,6,7,  2,7,3,   // +Y (top)
    0,4,6,  0,6,2,   // -Z face
    1,3,7,  1,7,5    // +Z face
};
//// Pyramid vertices (4 base corners + apex):contentReference[oaicite:3]{index=3}:
//Vertex PyramidVertices[] = {
//    { XMFLOAT3(+1.0f,0.0f,+1.0f), {0,1,0} },
//    { XMFLOAT3(+1.0f,0.0f,-1.0f), {0,1,0} },
//    { XMFLOAT3(-1.0f,0.0f,-1.0f), {0,1,0} },
//    { XMFLOAT3(-1.0f,0.0f,+1.0f), {0,1,0} },
//    { XMFLOAT3(0.0f,1.5f, 0.0f), {0,0,1} },  // apex
//};
//unsigned short PyramidIndices[] = {
//    0,1,4,  1,2,4,  2,3,4,  3,0,4, // 4 triangular sides
//    0,3,2,  0,2,1  // base (two triangles)
//};

// Global Declarations
HWND g_hWnd = nullptr;
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11Texture2D* g_pDepthStencilBuffer = nullptr;
ID3D11InputLayout* g_pInputLayout = nullptr;
ID3D11Buffer* g_pVertexBuffer = nullptr;
ID3D11Buffer* g_pIndexBuffer = nullptr;
ID3D11Buffer* g_pConstantBuffer = nullptr;
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr;

// Forward declarations
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
void CleanupDevice();
void Render();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    if (FAILED(InitWindow(hInstance, nCmdShow)))
        return 0;

    if (FAILED(InitDevice())) {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = {};
    while (WM_QUIT != msg.message) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            Render();
        }
    }

    CleanupDevice();
    return (int)msg.wParam;
}

// Register and create window
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow) {
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L,
                      hInstance, nullptr, nullptr, nullptr, nullptr,
                      L"D3D11WindowClass", nullptr };
    RegisterClassEx(&wc);

    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hWnd = CreateWindow(L"D3D11WindowClass", L"Direct3D 11 - Triangle",
        WS_OVERLAPPEDWINDOW, 100, 100, rc.right - rc.left,
        rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);

    if (!g_hWnd)
        return E_FAIL;

    ShowWindow(g_hWnd, nCmdShow);
    return S_OK;
}

// Initialize Direct3D device and pipeline
HRESULT InitDevice() {
    // Define swap chain
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 800;
    sd.BufferDesc.Height = 600;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    // Create device and swap chain
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice,
        nullptr, &g_pImmediateContext);
    if (FAILED(hr))
        return hr;

    // Create render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
        (void**)&pBackBuffer);
    if (FAILED(hr))
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr,
        &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return hr;

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

    // create depth stencil buffer
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = sd.BufferDesc.Width;
    depthDesc.Height = sd.BufferDesc.Height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24-bit depth + 8-bit stencil
    depthDesc.SampleDesc.Count = 1; // No MSAA for now
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;

    hr = g_pd3dDevice->CreateTexture2D(&depthDesc, nullptr, &g_pDepthStencilBuffer);
    if (FAILED(hr)) {
        return hr;
    }

    hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencilBuffer, nullptr, &g_pDepthStencilView);
    if (FAILED(hr)) {
        return hr;
    }
    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);


    // Setup viewport
    D3D11_VIEWPORT vp = {};
    vp.Width = 800.0f;
    vp.Height = 600.0f;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    g_pImmediateContext->RSSetViewports(1, &vp);

    // Compile and create shaders
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    const char* vsCode =
        "cbuffer ConstantBuffer : register(b0) { matrix mWorldViewProj; };"
        "struct VS_INPUT { float3 pos : POSITION; float4 col : COLOR; };"
        "struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };"
        "PS_INPUT VSMain(VS_INPUT input) {"
        "    PS_INPUT output;"
        "    output.pos = mul(float4(input.pos, 1.0f), mWorldViewProj);"
        "    output.col = input.col;"
        "    return output;"
        "}";

    hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr,
        "VSMain", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) errorBlob->Release();
        return hr;
    }

    hr = g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr, &g_pVertexShader);
    if (FAILED(hr)) {
        vsBlob->Release();
        return hr;
    }

    const char* psCode =
        "struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };"
        "float4 PSMain(PS_INPUT input) : SV_TARGET {"
        "    return input.col;"
        "}";

    hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr,
        "PSMain", "ps_4_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) errorBlob->Release();
        vsBlob->Release();
        return hr;
    }

    hr = g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr, &g_pPixelShader);
    if (FAILED(hr)) {
        psBlob->Release();
        vsBlob->Release();
        return hr;
    }

    // Define input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
          0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
          12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = g_pd3dDevice->CreateInputLayout(layout, 2,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &g_pInputLayout);
    vsBlob->Release();
    psBlob->Release();
    if (FAILED(hr))
        return hr;

    g_pImmediateContext->IASetInputLayout(g_pInputLayout);

    // Create vertex buffer
    // Example (cube) – similar for pyramid:
    D3D11_BUFFER_DESC vDesc = { sizeof(CubeVertices), D3D11_USAGE_DEFAULT,
        D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA vData = { CubeVertices, 0, 0 };
    hr = g_pd3dDevice->CreateBuffer(&vDesc, &vData, &g_pVertexBuffer);
    if (FAILED(hr))
        return hr;

    D3D11_BUFFER_DESC iDesc = { sizeof(CubeIndices), D3D11_USAGE_DEFAULT,
        D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA iData = { CubeIndices, 0, 0 };
    hr = g_pd3dDevice->CreateBuffer(&iDesc, &iData, &g_pIndexBuffer);
    if (FAILED(hr)) {
        MessageBoxA(0, "Failed to create index buffer", "Error", MB_ICONERROR);
    }

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pConstantBuffer);
    if (FAILED(hr)) {
        MessageBoxA(0, "Failed to create constant buffer", "Error", MB_ICONERROR);
    }

    return S_OK;
}

// Render frame
void Render() {
    // Clear the back buffer
    float clearColor[4] = { 0.1f, 0.2f, 0.3f, 1.0f };
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
    g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);


    // Set vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    // Set index buffer
    g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    // Set primitive topology
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set shaders
    g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

    // Set constant buffer (for MVP matrix, etc.)
    g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);

    //Initialize and set the world-view-projection matrix
    XMMATRIX world = XMMatrixRotationY(0.0f); // static cube, or animate over time
    XMMATRIX view = XMMatrixLookAtLH(
        XMVectorSet(0.0f, 1.5f, -3.0f, 0.0f),  // camera position
        XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),   // look at origin
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));  // up vector

    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 800.0f / 600.0f, 0.1f, 100.0f);

    XMMATRIX wvp = world * view * proj;

    ConstantBuffer cb;
    cb.mWorldViewProj = XMMatrixTranspose(wvp); // Transpose for HLSL

    g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb, 0, 0);

    // Draw indexed geometry
    g_pImmediateContext->DrawIndexed(_countof(CubeIndices), 0, 0);

    // Present the buffer to the screen
    g_pSwapChain->Present(1, 0);
}

// Cleanup resources
void CleanupDevice() {
    if (g_pImmediateContext) g_pImmediateContext->ClearState();
    if (g_pVertexBuffer) g_pVertexBuffer->Release();
    if (g_pInputLayout) g_pInputLayout->Release();
    if (g_pVertexShader) g_pVertexShader->Release();
    if (g_pPixelShader) g_pPixelShader->Release();
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT:
        Render();
        ValidateRect(hWnd, nullptr);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}