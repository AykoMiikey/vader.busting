#define NOMINMAX
#include <Windows.h>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include "InputSys.hpp"
#include "../source.hpp"

#include "../SDK/CVariables.hpp"

class Win32InputSys : public InputSys {
public:
	Win32InputSys( );
	virtual ~Win32InputSys( );

	virtual bool Initialize( IDirect3DDevice9* pDevice );
	virtual void Destroy( );

	virtual void* GetMainWindow( ) const { return ( void* )m_hTargetWindow; }

	virtual KeyState GetKeyState( int vk, bool always_detect_press = false);
	virtual bool IsKeyDown( int vk, bool always_detect_press = false);
	virtual bool IsInBox( Vector2D box_pos, Vector2D box_size );
	virtual bool WasKeyPressed( int vk, bool always_detect_press = false );

	virtual void RegisterHotkey( int vk, std::function< void( void ) > f );
	virtual void RemoveHotkey( int vk );

	virtual Vector2D GetMousePosition( ) {
		return m_MousePos;
	}

	virtual float GetScrollMouse( ) {
		return m_ScrollMouse;
	}

	virtual void SetScrollMouse( float scroll ) {
		m_ScrollMouse = scroll;
	}

private:
	static LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

	bool ProcessMessage( UINT uMsg, WPARAM wParam, LPARAM lParam );
	bool ProcessMouseMessage( UINT uMsg, WPARAM wParam, LPARAM lParam );
	bool ProcessKeybdMessage( UINT uMsg, WPARAM wParam, LPARAM lParam );

	float  m_ScrollMouse = 0.0f;

	HWND m_hTargetWindow;
	LONG_PTR m_ulOldWndProc;
	KeyState m_iKeyMap[ 256 ];

	Vector2D m_MousePos;

	std::function< void( void ) > m_Hotkeys[ 256 ];
};

Encrypted_t<InputSys> InputSys::Get( ) {
	static Win32InputSys instance;
	return &instance;
}

Win32InputSys::Win32InputSys( ) :
	m_hTargetWindow( nullptr ), m_ulOldWndProc( 0 ) {
}

Win32InputSys::~Win32InputSys( ) {
}

bool Win32InputSys::Initialize( IDirect3DDevice9* pDevice ) {
	D3DDEVICE_CREATION_PARAMETERS params;

	if( FAILED( pDevice->GetCreationParameters( &params ) ) ) {
		Win32::Error( XorStr( "GetCreationParameters failed" ) );
		return false;
	}

	m_hTargetWindow = params.hFocusWindow;
	m_ulOldWndProc = SetWindowLongPtr( m_hTargetWindow, GWLP_WNDPROC, ( LONG_PTR )WndProc );

	if( !m_ulOldWndProc ) {
		Win32::Error( XorStr( "SetWindowLongPtr failed" ) );
		return false;
	}

	return true;
}

void Win32InputSys::Destroy( ) {
	if( m_ulOldWndProc )
		SetWindowLongPtr( m_hTargetWindow, GWLP_WNDPROC, m_ulOldWndProc );
	m_ulOldWndProc = 0;
}

#include "../ShittierMenu/MenuNew.h"
#include "../ShittierMenu/IMGAY/impl/imgui_impl_dx9.h"
#include "../ShittierMenu/IMGAY/impl/imgui_impl_win32.h"
extern LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT __stdcall Win32InputSys::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
	auto win32input = static_cast<Win32InputSys*>(Get().Xor());

	win32input->ProcessMessage(msg, wParam, lParam);

	if (msg == WM_MOUSEMOVE) {
		win32input->m_MousePos.x = (signed short)(lParam);
		win32input->m_MousePos.y = (signed short)(lParam >> 16);
	}
	else if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) {
		win32input->m_ScrollMouse += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
	}

	if (Menu::initialized && Menu::opened && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

	if (Menu::initialized && Menu::opened) {
		if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL)
			return true;
	}

	return CallWindowProc((WNDPROC)win32input->m_ulOldWndProc, hWnd, msg, wParam, lParam);
}

bool Win32InputSys::ProcessMessage( UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	switch( uMsg ) {
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_LBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
	case WM_XBUTTONUP:
		return ProcessMouseMessage( uMsg, wParam, lParam );
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		return ProcessKeybdMessage( uMsg, wParam, lParam );
	default:
		return false;
	}
}

bool Win32InputSys::ProcessMouseMessage( UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	auto key = VK_LBUTTON;
	auto state = KeyState::None;

	switch( uMsg ) {
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		state = uMsg == WM_MBUTTONUP ? KeyState::Up : KeyState::Down;
		key = VK_MBUTTON;
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		state = uMsg == WM_RBUTTONUP ? KeyState::Up : KeyState::Down;
		key = VK_RBUTTON;
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		state = uMsg == WM_LBUTTONUP ? KeyState::Up : KeyState::Down;
		key = VK_LBUTTON;
		break;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		state = uMsg == WM_XBUTTONUP ? KeyState::Up : KeyState::Down;
		key = ( HIWORD( wParam ) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2 );
		break;
	default:
		return false;
	}

	if( state == KeyState::Up && m_iKeyMap[ key ] == KeyState::Down ) {
		m_iKeyMap[ key ] = KeyState::Pressed;
	}
	else
		m_iKeyMap[ key ] = state;
	return true;
}

bool Win32InputSys::ProcessKeybdMessage( UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	auto key = wParam;
	auto state = KeyState::None;

	switch( uMsg ) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		state = KeyState::Down;
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		state = KeyState::Up;
		break;
	default:
		return false;
	}

	if( state == KeyState::Up && m_iKeyMap[ int( key ) ] == KeyState::Down ) {
		m_iKeyMap[ int( key ) ] = KeyState::Pressed;

		auto& hotkey_callback = m_Hotkeys[ key ];

		if( hotkey_callback )
			hotkey_callback( );

	}
	else {
		m_iKeyMap[ int( key ) ] = state;
	}

	return true;
}

KeyState Win32InputSys::GetKeyState( int vk, bool always_detect_press) {
	if (!always_detect_press && (Interfaces::m_pClient->IsChatRaised() || Interfaces::m_pEngine->Con_IsVisible())) return KeyState::None;
	return m_iKeyMap[ vk ];
}

bool Win32InputSys::IsKeyDown( int vk, bool always_detect_press) {
	if (!always_detect_press && (Interfaces::m_pClient->IsChatRaised() || Interfaces::m_pEngine->Con_IsVisible())) return false;
	if( vk <= 0 || vk > 255 ) return false;

	return m_iKeyMap[ vk ] == KeyState::Down;
}

bool Win32InputSys::IsInBox( Vector2D box_pos, Vector2D box_size ) {
	return (
		m_MousePos.x > box_pos.x &&
		m_MousePos.y > box_pos.y &&
		m_MousePos.x < box_pos.x + box_size.x &&
		m_MousePos.y < box_pos.y + box_size.y
		);
}

bool Win32InputSys::WasKeyPressed( int vk, bool always_detect_press) {
	if (!always_detect_press && (Interfaces::m_pClient->IsChatRaised() || Interfaces::m_pEngine->Con_IsVisible())) return false;

	if( vk <= 0 || vk > 255 )
		return false;

	if( m_iKeyMap[ vk ] == KeyState::Pressed ) {
		m_iKeyMap[ vk ] = KeyState::Up;
		return true;
	}

	return false;
}

void Win32InputSys::RegisterHotkey( int vk, std::function< void( void ) > f ) {
	m_Hotkeys[ vk ] = f;
}

void Win32InputSys::RemoveHotkey( int vk ) {
	m_Hotkeys[ vk ] = nullptr;
}