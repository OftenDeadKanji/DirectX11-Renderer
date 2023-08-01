#include "shader.h"
#include "../lightSystem/lightSystem.h"
#include "../../engine/renderer.h"
#include "../fogRenderer/fogRenderer.h"

namespace Engine
{
	void Shader::init(const std::wstring& vertexShaderFilePath, const std::wstring& pixelShaderFilePath, std::vector<D3D11_INPUT_ELEMENT_DESC> desc)
	{
		UINT shaderCompilationFlags = 0;
#ifndef _NDEBUG
		shaderCompilationFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		
		DxResPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob, errorBlob;
		HRESULT result;

		std::string maxPointLights = std::to_string(MAX_POINT_LIGHTS);
		std::string shaderCalcInViewSpace = std::to_string(SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE);
		std::string maxGPUParticles = std::to_string(MAX_GPU_PARTICLES);
		std::string maxFogInstances = std::to_string(MAX_VOLUMETRIC_FOG_INSTANCES);

		D3D_SHADER_MACRO globalMacros[] = { 
			"MAX_POINT_LIGHTS", maxPointLights.c_str(),
			"SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE", shaderCalcInViewSpace.c_str(),
			"MAX_GPU_PARTICLES", maxGPUParticles.c_str(),
			"MAX_VOLUMETRIC_FOG_INSTANCES", maxFogInstances.c_str(),
			NULL, NULL
		};
		
		result = D3DCompileFromFile(vertexShaderFilePath.c_str(), globalMacros, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", shaderCompilationFlags, 0, vertexShaderBlob.reset(), errorBlob.reset());
		assert(result >= 0);

		result = D3D::getInstancePtr()->getDevice()->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), NULL, m_vertexShader.reset());
		assert(result >= 0);

		if (!pixelShaderFilePath.empty())
		{
			result = D3DCompileFromFile(pixelShaderFilePath.c_str(), globalMacros, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", shaderCompilationFlags, 0, pixelShaderBlob.reset(), errorBlob.reset());
			if (FAILED(result) && errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			}
			assert(result >= 0);

			result = D3D::getInstancePtr()->getDevice()->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), NULL, m_pixelShader.reset());
			assert(result >= 0);
		}

		if (!desc.empty())
		{
			result = D3D::getInstancePtr()->getDevice()->CreateInputLayout(desc.data(), desc.size(), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), m_inputLayout.reset());
			assert(result >= 0);
		}
	}

	void Shader::init(const std::wstring& vertexShaderFilePath, const std::wstring& hullShaderFilePath, const std::wstring& domainShaderFilePath, const std::wstring& geometryShaderFilePath, const std::wstring& pixelShaderFilePath, std::vector<D3D11_INPUT_ELEMENT_DESC> desc)
	{
		init(vertexShaderFilePath, pixelShaderFilePath, desc);

		UINT shaderCompilationFlags = 0;
#ifndef _NDEBUG
		shaderCompilationFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		DxResPtr<ID3DBlob> hullShaderBlob, domainShaderBlob, geometryShaderBlob, errorBlob;
		HRESULT result;

		std::string maxPointLights = std::to_string(MAX_POINT_LIGHTS);
		std::string shaderCalcInViewSpace = std::to_string(SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE);
		std::string maxGPUParticles = std::to_string(MAX_GPU_PARTICLES);
		std::string maxFogInstances = std::to_string(MAX_VOLUMETRIC_FOG_INSTANCES);

		D3D_SHADER_MACRO globalMacros[] = {
			"MAX_POINT_LIGHTS", maxPointLights.c_str(),
			"SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE", shaderCalcInViewSpace.c_str(),
			"MAX_GPU_PARTICLES", maxGPUParticles.c_str(),
			"MAX_VOLUMETRIC_FOG_INSTANCES", maxFogInstances.c_str(),
			NULL, NULL
		};

		if (!hullShaderFilePath.empty() && !domainShaderFilePath.empty())
		{
			result = D3DCompileFromFile(hullShaderFilePath.c_str(), globalMacros, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "hs_5_0", shaderCompilationFlags, 0, hullShaderBlob.reset(), errorBlob.reset());
			assert(result >= 0);

			result = D3DCompileFromFile(domainShaderFilePath.c_str(), globalMacros, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ds_5_0", shaderCompilationFlags, 0, domainShaderBlob.reset(), errorBlob.reset());
			assert(result >= 0);

			result = D3D::getInstancePtr()->getDevice()->CreateHullShader(hullShaderBlob->GetBufferPointer(), hullShaderBlob->GetBufferSize(), NULL, m_hullShader.reset());
			assert(result >= 0);

			result = D3D::getInstancePtr()->getDevice()->CreateDomainShader(domainShaderBlob->GetBufferPointer(), domainShaderBlob->GetBufferSize(), NULL, m_domainShader.reset());
			assert(result >= 0);
		}

		if (!geometryShaderFilePath.empty())
		{
			result = D3DCompileFromFile(geometryShaderFilePath.c_str(), globalMacros, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "gs_5_0", shaderCompilationFlags, 0, geometryShaderBlob.reset(), errorBlob.reset());
			if (FAILED(result) && errorBlob)
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			assert(result >= 0);

			result = D3D::getInstancePtr()->getDevice()->CreateGeometryShader(geometryShaderBlob->GetBufferPointer(), geometryShaderBlob->GetBufferSize(), NULL, m_geometryShader.reset());
			assert(result >= 0);
		}
	}

	void Shader::initComputeShader(const std::wstring& computeShaderFilePath)
	{
		UINT shaderCompilationFlags = 0;
#ifndef _NDEBUG
		shaderCompilationFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		DxResPtr<ID3DBlob> shaderBlob, errorBlob;
		HRESULT result;

		std::string maxPointLights = std::to_string(MAX_POINT_LIGHTS);
		std::string shaderCalcInViewSpace = std::to_string(SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE);
		std::string maxGPUParticles = std::to_string(MAX_GPU_PARTICLES);
		std::string maxFogInstances = std::to_string(MAX_VOLUMETRIC_FOG_INSTANCES);

		D3D_SHADER_MACRO globalMacros[] = {
			"MAX_POINT_LIGHTS", maxPointLights.c_str(),
			"SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE", shaderCalcInViewSpace.c_str(),
			"MAX_GPU_PARTICLES", maxGPUParticles.c_str(),
			"MAX_VOLUMETRIC_FOG_INSTANCES", maxFogInstances.c_str(),
			NULL, NULL
		};

		result = D3DCompileFromFile(computeShaderFilePath.c_str(), globalMacros, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "cs_5_0", shaderCompilationFlags, 0, shaderBlob.reset(), errorBlob.reset());
		assert(result >= 0);

		result = D3D::getInstancePtr()->getDevice()->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, m_computeShader.reset());
		assert(result >= 0);
	}

	void Shader::bind()
	{
		//layout
		D3D::getInstancePtr()->getDeviceContext()->IASetPrimitiveTopology(m_topology);
		D3D::getInstancePtr()->getDeviceContext()->IASetInputLayout(m_inputLayout.ptr());

		//vertex and pixel shaders
		D3D::getInstancePtr()->getDeviceContext()->VSSetShader(m_vertexShader.ptr(), NULL, 0);
		D3D::getInstancePtr()->getDeviceContext()->HSSetShader(m_hullShader.ptr(), NULL, 0);
		D3D::getInstancePtr()->getDeviceContext()->DSSetShader(m_domainShader.ptr(), NULL, 0);
		D3D::getInstancePtr()->getDeviceContext()->GSSetShader(m_geometryShader.ptr(), NULL, 0);
		D3D::getInstancePtr()->getDeviceContext()->PSSetShader(m_pixelShader.ptr(), NULL, 0);
		D3D::getInstancePtr()->getDeviceContext()->CSSetShader(m_computeShader.ptr(), NULL, 0);
	}

	void Shader::reset()
	{
		m_inputLayout.reset();
		m_vertexShader.reset();
		m_hullShader.reset();
		m_domainShader.reset();
		m_geometryShader.reset();
		m_pixelShader.reset();
	}

	void Shader::setTopology(D3D_PRIMITIVE_TOPOLOGY topology)
	{
		m_topology = topology;
	}

	void Shader::setVertexShader(const void* byteCode, size_t codeLength)
	{
		auto result = D3D::getInstancePtr()->getDevice()->CreateVertexShader(byteCode, codeLength, NULL, m_vertexShader.reset());
		assert(result >= 0);
	}
	void Shader::setPixelShader(const void* byteCode, size_t codeLength)
	{
		auto result = D3D::getInstancePtr()->getDevice()->CreatePixelShader(byteCode, codeLength, NULL, m_pixelShader.reset());
		assert(result >= 0);
	}
	void Shader::setInputLayout(const D3D11_INPUT_ELEMENT_DESC* desc, unsigned int count, const void* vertexShaderByteCode, size_t vertexShaderCodeLength)
	{
		auto result = D3D::getInstancePtr()->getDevice()->CreateInputLayout(desc, count, vertexShaderByteCode, vertexShaderCodeLength, m_inputLayout.reset());
		assert(result >= 0);
	}
}