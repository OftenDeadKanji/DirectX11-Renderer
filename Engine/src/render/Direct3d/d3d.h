#pragma once
#include "../../dependencies/Windows/win_def.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <d3d11_4.h>
#include <d3dcompiler.h>
#include "../../dependencies/Windows/win_undef.h"

#include <memory>
#include "DxRes.h"
#include "../../utils/nonCopyable.h"

namespace Engine
{
	class D3D
		: public NonCopyable
	{
	public:
		static D3D* createInstance();
		static void deleteInstance();
		static D3D* getInstancePtr();

		void init();
		void deinit();

		ID3D11Device5* getDevice();
		ID3D11DeviceContext4* getDeviceContext();
		IDXGIFactory5* getFactory();

	private:
		D3D() = default;

		static D3D* s_instance;

		DxResPtr<IDXGIFactory> m_factory;
		DxResPtr<IDXGIFactory5> m_factory5;
		DxResPtr<ID3D11Device> m_device;
		DxResPtr<ID3D11Device5> m_device5;
		DxResPtr<ID3D11DeviceContext> m_devcon;
		DxResPtr<ID3D11DeviceContext4> m_devcon4;
		DxResPtr<ID3D11Debug> m_devdebug;
	};
}