//drawsvgC.exe
//This is a sample program that draws an SVG file in a window using C++Direct2D.
//When you drop an SVG file onto the window, it will be rendered using Direct2D. SwapChain is used.
//Direct2D may not render correctly due to unsupported SVG elements and attributes.
//

//svgファイルをC++Direct2DでWindowに描画するサンプルプログラムです。
//Window上にSVGファイルをドロップするとDirect2Dによって描画します。SwapChainを使用しています。
//Direct2Dは、未サポートのSVG要素と属性があるので正確に描画しない場合があります。

//(c)2025 teamwind japan n.hayashi

#include <windows.h>
#include <d2d1_3.h>
#include <d2d1svg.h>
#include <wincodec.h>
#include <iostream>
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "windowscodecs.lib")

using namespace Microsoft::WRL;

// globals
HWND g_hwnd = nullptr;
UINT g_width = 600;
UINT g_height = 600;

ComPtr<ID2D1Factory1> d2dFactory;
ComPtr<IWICImagingFactory> wicFactory;
ComPtr<ID2D1DeviceContext5> d2dContext;
ComPtr<ID2D1Device> d2dDevice;
ComPtr<ID2D1SvgDocument> svgDocument;
ComPtr<IDXGISwapChain1> swapChain;
ComPtr<ID2D1Bitmap1> d2dTarget;


//init d2d and swap chain
bool InitD2D(HWND hwnd) {  

    // D2D Factory 
    D2D1_FACTORY_OPTIONS options = {};  
    HRESULT hr = D2D1CreateFactory(  
        D2D1_FACTORY_TYPE_SINGLE_THREADED,  
        __uuidof(ID2D1Factory1),  
        &options,  
        reinterpret_cast<void**>(d2dFactory.GetAddressOf())  
    );  

    // WICImagingFactory  
    if(FAILED(CoInitialize(nullptr)))
        return false;
    if(FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory))))
		return false;  

    // D3D11 Device  
    ComPtr<ID3D11Device> d3dDevice;  
    ComPtr<ID3D11DeviceContext> d3dContext;  
    D3D_FEATURE_LEVEL featureLevel;  
    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,  
                      nullptr, 0, D3D11_SDK_VERSION, &d3dDevice, &featureLevel, &d3dContext);  

    // get DXGI Device  
    ComPtr<IDXGIDevice> dxgiDevice;  
    d3dDevice.As(&dxgiDevice);  

    // D2D CreateDevice 
    d2dFactory->CreateDevice(dxgiDevice.Get(), &d2dDevice);  
    ComPtr<ID2D1DeviceContext> tempContext;  
    d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &tempContext);  
    tempContext.As(&d2dContext);  

	// DXGI swap chain  
    ComPtr<IDXGIAdapter> adapter;  
    dxgiDevice->GetAdapter(&adapter);  
    ComPtr<IDXGIFactory2> dxgiFactory;  
    adapter->GetParent(IID_PPV_ARGS(&dxgiFactory));  
    DXGI_SWAP_CHAIN_DESC1 scDesc = {};  
    scDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;  
    scDesc.Width = g_width;  
    scDesc.Height = g_height;  
    scDesc.SampleDesc.Count = 1;  
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  
    scDesc.BufferCount = 2;  
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;  
    dxgiFactory->CreateSwapChainForHwnd(d3dDevice.Get(), hwnd, &scDesc, nullptr, nullptr, &swapChain);  

    // make d2dTarget  
    ComPtr<IDXGISurface> surface;  
    swapChain->GetBuffer(0, IID_PPV_ARGS(&surface));  
    D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(  
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,  
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));  
    d2dContext->CreateBitmapFromDxgiSurface(surface.Get(), &bp, &d2dTarget);  
    d2dContext->SetTarget(d2dTarget.Get());  

    return true;  
}

// Load an SVG file into the D2D context
bool LoadSvgFile(const wchar_t* filename) {

    if (svgDocument) {
		svgDocument.Reset();
    }

    ComPtr<IWICStream> stream;
    if (FAILED(wicFactory->CreateStream(&stream))) return false;
    if (FAILED(stream->InitializeFromFilename(filename, GENERIC_READ))) return false;

    D2D1_SIZE_F size = D2D1::SizeF((FLOAT)g_width, (FLOAT)g_height);
    return SUCCEEDED(d2dContext->CreateSvgDocument(stream.Get(), size, &svgDocument));
}

// Render the SVG document
void Render() {
    d2dContext->BeginDraw();
    d2dContext->Clear(D2D1::ColorF(D2D1::ColorF::White));
    if (svgDocument) {
        d2dContext->DrawSvgDocument(svgDocument.Get());
    }
    HRESULT hr = d2dContext->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
		// Recreate the target if necessary
    }
    swapChain->Present(1, 0);
}

//drop file listener 
bool checkExtention(const std::wstring& filename, const std::wstring& extension) {
    if (filename.size() >= extension.size() &&
        filename.compare(filename.size() - extension.size(), extension.size(), extension) == 0) {
        return true;
    }
    return false;
}
void listenDropFile(HWND hWnd, WPARAM wParam)
{
	WCHAR	szpath[MAX_PATH+1];

	HDROP hdrop = (HDROP)wParam;

	//drop files
	int num = DragQueryFile(hdrop, -1, NULL, 0);
	for (int i = 0; i < num; i++) {

		//get the file name
		DragQueryFile(hdrop, i, szpath, sizeof(szpath) / 2);

		//check if the file is an SVG file
		if(checkExtention(szpath, L"svg")) {

			//load the SVG file
			if (!LoadSvgFile(szpath)) {
			} else {
				//render the SVG file
				InvalidateRect(hWnd, NULL, TRUE);
			}
		}
		//only one file
		break;
    }
    DragFinish(hdrop);
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
		case WM_PAINT:
			Render();
			ValidateRect(hWnd, nullptr);
			return 0;
		case WM_DROPFILES:
			listenDropFile(hWnd, wParam);
			break;
		case WM_CREATE:
			if (!InitD2D(hWnd)) {
				DestroyWindow(hWnd);
			}
			DragAcceptFiles(hWnd, TRUE);
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DrawSvgD2DWindowClass";
    RegisterClass(&wc);

    g_hwnd = CreateWindowEx(0, wc.lpszClassName, L"Direct2D SVG Draw Sample",
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                            g_width, g_height, nullptr, nullptr, hInstance, nullptr);
    if (!g_hwnd) return -1;

    ShowWindow(g_hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

