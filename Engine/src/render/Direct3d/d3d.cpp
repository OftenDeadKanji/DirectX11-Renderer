#include "d3d.h"
#include "../../utils/assert.h"

namespace Engine
{
	extern "C"
	{
		_declspec(dllexport) uint32_t NvOptimusEnablement = 1;
		_declspec(dllexport) uint32_t AmdPowerXpressRequestHighPerformance = 1;
	}

	D3D* D3D::s_instance = nullptr;

	D3D* D3D::createInstance()
	{
		if (!s_instance)
		{
			s_instance = new D3D();
		}

		return s_instance;
	}

	void D3D::deleteInstance()
	{
		s_instance->deinit();
		delete s_instance;

		s_instance = nullptr;
	}

	D3D* D3D::getInstancePtr()
	{
		return s_instance;
	}

	void Engine::D3D::init()
	{
		HRESULT result;
		
		result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)m_factory.reset());
		ALWAYS_ASSERT(result >= 0 && "CreateDXGIFactory");

		result = m_factory->QueryInterface(__uuidof(IDXGIFactory5), (void**)m_factory5.reset());
		ALWAYS_ASSERT(result >= 0 && "Query IDXGIFactory5");

		{
			uint32_t index = 0;
			IDXGIAdapter1* adapter;
			while (m_factory5->EnumAdapters1(index++, &adapter) != DXGI_ERROR_NOT_FOUND)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				// LOG
			}
		}

		const D3D_FEATURE_LEVEL featureLevelRequested = D3D_FEATURE_LEVEL_11_1;
		D3D_FEATURE_LEVEL featureLevelInitialized = D3D_FEATURE_LEVEL_11_1;
		result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, &featureLevelRequested, 1, D3D11_SDK_VERSION, m_device.reset(), &featureLevelInitialized, m_devcon.reset());
		ALWAYS_ASSERT(result >= 0 && "D3D11CreateDevice");
		ALWAYS_ASSERT(featureLevelRequested == featureLevelInitialized && "D3D_FEATURE_LEVEL_11_1");

		result = m_device->QueryInterface(__uuidof(ID3D11Device5), (void**)m_device5.reset());
		ALWAYS_ASSERT(result >= 0 && "Query ID3D11Device5");

		result = m_devcon->QueryInterface(__uuidof(ID3D11DeviceContext4), (void**)m_devcon4.reset());
		ALWAYS_ASSERT(result >= 0 && "Query ID3D11DeviceContext4");

		result = m_device->QueryInterface(__uuidof(ID3D11Debug), (void**)m_devdebug.reset());
		ALWAYS_ASSERT(result >= 0 && "Query ID3D11Debug");
	}

	void Engine::D3D::deinit()
	{
		m_devcon4->ClearState();
		m_devcon4->Flush();
		m_devdebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);

		m_devdebug.release();
		m_devcon4.release();
		m_device5.release();
		m_devcon.release();
		m_device.release();
		m_factory5.release();
		m_factory.release();
	}

	ID3D11Device5* D3D::getDevice()
	{
		return m_device5.ptr();
	}

	ID3D11DeviceContext4* D3D::getDeviceContext()
	{
		return m_devcon4.ptr();
	}

	IDXGIFactory5* D3D::getFactory()
	{
		return m_factory5.ptr();
	}

}