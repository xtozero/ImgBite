// PaintTest.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "PaintTest.h"

#include "../ImgBite/JFIF/JFIF.h"
#include "../ImgBite/PNG/PNG.h"
#include "../ImgBite/NETPBM/NETPBM.h"

#include "Window/Window.h"

#pragma comment( lib, "ImgBite.lib" )

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

enum
{
	DRAW_PNG = 0,
	DRAW_JPG,
	DRAW_PBM,
	DRAW_PGM,
	DRAW_PPM,
};

int curDrawImage = DRAW_PNG;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

using Monochrome = std::integral_constant<int, 1>;
using Polychrome = std::integral_constant<int, 3>;

template <typename IMG>
void LoadAndRenderImage( HDC hdc, const char* filePath, Monochrome )
{
	IMG image;
	image.Load( filePath );

	unsigned int width = image.GetWidth( );
	unsigned int height = image.GetHeight( );
	unsigned char bpp = image.GetBytePerPixel( );

	const std::vector<unsigned char>& colors = image.GetByteStream( );

	for ( unsigned int i = 0; i < height; ++i )
	{
		size_t pos = ( i * width * bpp );
		for ( unsigned int j = 0; j < width; ++j )
		{
			SetPixel( hdc, j, i, RGB( colors[pos], colors[pos], colors[pos] ) );
			++pos;
		}
	}
}

template <typename IMG>
void LoadAndRenderImage( HDC hdc, const char* filePath, Polychrome )
{
	IMG image;
	image.Load( filePath );

	unsigned int width = image.GetWidth( );
	unsigned int height = image.GetHeight( );
	unsigned char bpp = image.GetBytePerPixel( );

	const std::vector<unsigned char>& colors = image.GetByteStream( );

	for ( unsigned int i = 0; i < height; ++i )
	{
		size_t pos = ( i * width * bpp );
		for ( unsigned int j = 0; j < width; ++j )
		{
			const unsigned char* color = &colors[pos];
			SetPixel( hdc, j, i, RGB( color[0], color[1], color[2] ) );
			pos += 3;
		}
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PAINTTEST, szWindowClass, MAX_LOADSTRING);
    

	CWindowSetup setup( hInstance, 512, 512 );
	Window window( szTitle );

    // Perform application initialization:
    if (!window.Run( setup, WndProc ))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PAINTTEST));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

			switch ( curDrawImage )
			{
			case DRAW_PNG:
				LoadAndRenderImage<PNG>( hdc, "../Image/lena.png", Polychrome( ) );
				break;
			case DRAW_JPG:
				LoadAndRenderImage<JFIF>( hdc, "../Image/lena.jpg", Polychrome( ) );
				break;
			case DRAW_PBM:
				LoadAndRenderImage<NETPBM>( hdc, "../Image/NETPBM/elephant.pbm", Monochrome( ) );
				break;
			case DRAW_PGM:
				LoadAndRenderImage<NETPBM>( hdc, "../Image/NETPBM/elephant.pgm", Monochrome( ) );
				break;
			case DRAW_PPM:
				LoadAndRenderImage<NETPBM>( hdc, "../Image/NETPBM/elephant.ppm", Polychrome( ) );
				break;
			}

            EndPaint(hWnd, &ps);
        }
        break;
	case WM_KEYUP:
		switch ( wParam )
		{
		case VK_F1:
			curDrawImage = DRAW_PNG;
			InvalidateRect( hWnd, nullptr, true );
			break;
		case VK_F2:
			curDrawImage = DRAW_JPG;
			InvalidateRect( hWnd, nullptr, true );
			break;
		case VK_F3:
			curDrawImage = DRAW_PBM;
			InvalidateRect( hWnd, nullptr, true );
			break;
		case VK_F4:
			curDrawImage = DRAW_PGM;
			InvalidateRect( hWnd, nullptr, true );
			break;
		case VK_F5:
			curDrawImage = DRAW_PPM;
			InvalidateRect( hWnd, nullptr, true );
			break;
		default:
			break;
		}
		break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
