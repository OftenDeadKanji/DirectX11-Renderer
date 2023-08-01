#pragma once
#include "../Direct3d/d3d.h"
#include "../../math/mathUtils.h"

namespace Engine
{
	class Shader
	{
	public:
		void init(const std::wstring& vertexShaderFilePath, const std::wstring& pixelShaderFilePath, std::vector<D3D11_INPUT_ELEMENT_DESC> desc);
		void init(const std::wstring& vertexShaderFilePath, const std::wstring& hullShaderFilePath, const std::wstring& domainShaderFilePath, const std::wstring& geometryShaderFilePath, const std::wstring& pixelShaderFilePath, std::vector<D3D11_INPUT_ELEMENT_DESC> desc);

		void initComputeShader(const std::wstring& computeShaderFilePath);

		void bind();

		void reset();

		void setTopology(D3D_PRIMITIVE_TOPOLOGY topology);

		void setVertexShader(const void* byteCode, size_t codeLength);
		void setPixelShader(const void* byteCode, size_t codeLength);
		void setInputLayout(const D3D11_INPUT_ELEMENT_DESC* desc, unsigned int count, const void* vertexShaderByteCode, size_t vertexShaderCodeLength);
		
	private:
		D3D_PRIMITIVE_TOPOLOGY m_topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		DxResPtr<ID3D11VertexShader> m_vertexShader;
		DxResPtr<ID3D11HullShader> m_hullShader;
		DxResPtr<ID3D11DomainShader> m_domainShader;
		DxResPtr<ID3D11GeometryShader> m_geometryShader;
		DxResPtr<ID3D11PixelShader> m_pixelShader;
		DxResPtr<ID3D11InputLayout> m_inputLayout;

		DxResPtr<ID3D11ComputeShader> m_computeShader;
	};
}