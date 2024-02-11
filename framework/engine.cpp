
#define NOMINMAX
#include <cassert>
#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>

#include "game.hpp"
#include "scene.hpp"


//-------------------------------------------------------
//	window related stuff
//-------------------------------------------------------

namespace
{
	HWND windowHandle = nullptr;

	constexpr int windowWidth = 1280;
	constexpr int windowHeight = 720;


	//-------------------------------------------------------
	LRESULT CALLBACK windowProcedure( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		switch ( message )
		{
			case WM_DESTROY:
				PostQuitMessage( 0 );
				break;

			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDBLCLK:
				Game::mouseButtonPressed(
					Scene::screenToWorldX( float( GET_X_LPARAM( lParam ) ) / windowWidth ),
					Scene::screenToWorldY( 1.f - float( GET_Y_LPARAM( lParam ) ) / windowHeight ) );
				break;

			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
				Game::mouseButtonReleased(
					Scene::screenToWorldX( float( GET_X_LPARAM( lParam ) ) / windowWidth ),
					Scene::screenToWorldY( 1.f - float( GET_Y_LPARAM( lParam ) ) / windowHeight ) );
				break;

			case WM_KEYDOWN:
				if ( wParam == VK_ESCAPE )
					DestroyWindow( windowHandle );
				if ( wParam == VK_SPACE )
				{
					Game::deinit();
					Game::init();
				}
				break;
		}
		return DefWindowProc( hwnd, message, wParam, lParam );
	}


	//-------------------------------------------------------
	void initWindow()
	{
		WNDCLASSEX windowClass;

		windowClass.cbSize = sizeof( windowClass );
		windowClass.hInstance = GetModuleHandle( nullptr );
		windowClass.lpszClassName = TEXT( "MiniBill_WndClass" );
		windowClass.lpfnWndProc = windowProcedure;
		windowClass.style = CS_DBLCLKS;

		windowClass.hIcon = nullptr;
		windowClass.hIconSm = nullptr;
		windowClass.hCursor = LoadCursor( nullptr, IDC_ARROW );
		windowClass.lpszMenuName = nullptr;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hbrBackground = nullptr;

		RegisterClassEx( &windowClass );

		RECT windowRect;
		windowRect.left = windowRect.top = 0;
		windowRect.bottom = windowHeight;
		windowRect.right = windowWidth;
		AdjustWindowRect( &windowRect, WS_CAPTION | WS_SYSMENU, FALSE );

		int screenWidth = GetSystemMetrics( SM_CXFULLSCREEN );
		int screenHeight = GetSystemMetrics( SM_CYFULLSCREEN );

		windowHandle = CreateWindowEx( 0, TEXT( "MiniBill_WndClass" ), TEXT( "Mini Billiard [Pre-Alpha]" ), WS_CAPTION | WS_SYSMENU,
								screenWidth / 2 - windowWidth / 2, screenHeight / 2 - windowHeight / 2, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
								HWND_DESKTOP, nullptr, GetModuleHandle( nullptr ), nullptr );

		ShowWindow( windowHandle, SW_SHOW );
	}


	//-------------------------------------------------------
	void deinitWindow()
	{
		DestroyWindow( windowHandle );
	}


	//-------------------------------------------------------
	bool processWindowMessages()
	{
		MSG msg;
		while ( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
				return false;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		return true;
	}
}


//-------------------------------------------------------
//	opengl related stuff
//-------------------------------------------------------

namespace
{
	HDC windowDC = nullptr;
	HGLRC openGLHandle = nullptr;


	//-------------------------------------------------------
	void initOGL()
	{
		windowDC = GetDC( windowHandle );

		PIXELFORMATDESCRIPTOR pfd;
		memset( &pfd, 0, sizeof( pfd ) );
		pfd.nSize = sizeof( pfd );
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int npfd = ChoosePixelFormat( windowDC, &pfd );

		memset( &pfd, 0, sizeof( pfd ) );
		pfd.nSize = sizeof( pfd );
		SetPixelFormat( windowDC, npfd, &pfd );

		openGLHandle = wglCreateContext( windowDC );
		wglMakeCurrent( windowDC, openGLHandle );

		using PFNWGLSWAPINTERVALEXTPROC = BOOL (WINAPI *)( int );
		if ( PFNWGLSWAPINTERVALEXTPROC wglSwapInterval = ( PFNWGLSWAPINTERVALEXTPROC )wglGetProcAddress( "wglSwapIntervalEXT" ) )
			wglSwapInterval( 0 );
	}


	//-------------------------------------------------------
	void deinitOGL()
	{
		wglMakeCurrent( nullptr, nullptr );
		wglDeleteContext( openGLHandle );
		ReleaseDC( windowHandle, windowDC );
		openGLHandle = nullptr;
		windowDC = nullptr;
	}


	//-------------------------------------------------------
	void draw()
	{
		Scene::draw();
		SwapBuffers( windowDC );

		assert( glGetError() == 0 );
	}
}


//-------------------------------------------------------
//	update and time related stuff
//-------------------------------------------------------

namespace
{
	constexpr int minFPS = 5;
	constexpr int maxFPS = 200;
	int targetFPS = maxFPS;

	LARGE_INTEGER clockFrequency;
	LARGE_INTEGER clockLastTick;


	//-------------------------------------------------------
	void initClock()
	{
		QueryPerformanceFrequency( &clockFrequency );
		QueryPerformanceCounter( &clockLastTick );
	}


	//-------------------------------------------------------
	void update()
	{
		float dt = 0.f;

		while ( true )
		{
			LARGE_INTEGER clockTick;
			QueryPerformanceCounter( &clockTick );
			double deltaTime = double( clockTick.QuadPart - clockLastTick.QuadPart ) / double( clockFrequency.QuadPart );
			if ( deltaTime >= 1.0 / targetFPS )
			{
				dt = float( deltaTime );
				clockLastTick = clockTick;
				break;
			}
		}

		Game::update( dt );
	}
}


//-------------------------------------------------------
//	public engine interface
//-------------------------------------------------------

namespace Engine
{
	void setTargetFPS( int fps )
	{
		targetFPS = fps > maxFPS ? maxFPS : fps < minFPS ? minFPS : fps;
	}


	void run()
	{
		initWindow();
		initOGL();
		initClock();
		Game::init();
		while ( processWindowMessages() )
		{
			update();
			draw();
		}
		Game::deinit();
		deinitOGL();
		deinitWindow();
	}
}
