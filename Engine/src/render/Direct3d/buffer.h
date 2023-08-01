#pragma once
#include "d3d.h"
#include "../../utils/assert.h"

namespace Engine
{
	template<typename T>
	class Buffer
	{
	public:
		Buffer() = default;
		bool isEmpty() const
		{
			return buffer == nullptr;
		}

		void createVertexBuffer(int verticesCount, T* data, ID3D11Device5* device)
		{
			DEV_ASSERT(data);
			capacity = verticesCount * sizeof(T);

			D3D11_BUFFER_DESC vertexBufferDesc = {};
			vertexBufferDesc.ByteWidth = capacity;
			vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA sr_data = {};
			sr_data.pSysMem = data;

			HRESULT result = device->CreateBuffer(&vertexBufferDesc, &sr_data, buffer.reset());
			ALWAYS_ASSERT(result >= 0);

		}

		void createInstanceBuffer(int instancesCount, T* data, ID3D11Device5* device)
		{
			if (buffer && capacity >= instancesCount * sizeof(T))
			{
				D3D::getInstancePtr()->getDeviceContext()->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &bufferSubresource);

				bufferSubresource.pData = data;

				D3D::getInstancePtr()->getDeviceContext()->Unmap(buffer, 0);

				return;
			}

			capacity = instancesCount * sizeof(T);

			D3D11_BUFFER_DESC vertexBufferDesc = {};
			vertexBufferDesc.ByteWidth = capacity;
			vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			HRESULT result;
			if (data)
			{
				D3D11_SUBRESOURCE_DATA sr_data = {};
				sr_data.pSysMem = data;
				result = device->CreateBuffer(&vertexBufferDesc, &sr_data, buffer.reset());
			}
			else
			{
				result = device->CreateBuffer(&vertexBufferDesc, nullptr, buffer.reset());
			}

			ALWAYS_ASSERT(result >= 0);

		}

		void createIndexBuffer(int indicesCount, T* data, ID3D11Device5* device)
		{
			DEV_ASSERT(data);

			capacity = indicesCount * sizeof(unsigned int);

			D3D11_BUFFER_DESC indexBufferDesc = {};
			indexBufferDesc.ByteWidth = capacity;
			indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

			D3D11_SUBRESOURCE_DATA sr_data = {};
			sr_data.pSysMem = data;

			HRESULT result = device->CreateBuffer(&indexBufferDesc, &sr_data, buffer.reset());
			ALWAYS_ASSERT(result >= 0);
		}

		void createConstantBuffer(ID3D11Device5* device)
		{
			capacity = sizeof(T);

			D3D11_BUFFER_DESC constantBufferDesc = {};
			constantBufferDesc.ByteWidth = capacity;
			constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			auto result = device->CreateBuffer(&constantBufferDesc, nullptr, buffer.reset());
			ALWAYS_ASSERT(result >= 0);
		}

		void createStructuredRWBuffer(int dataCount, ID3D11Device5* device)
		{
			DEV_ASSERT(dataCount > 0);
			capacity = dataCount * sizeof(T);

			{
				D3D11_BUFFER_DESC desc{};
				desc.ByteWidth = capacity;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
				desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				desc.StructureByteStride = sizeof(T);

				auto result = device->CreateBuffer(&desc, nullptr, buffer.reset());
				ALWAYS_ASSERT(result >= 0);
			}

			{
				D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
				desc.Format = DXGI_FORMAT_UNKNOWN;
				desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				desc.Buffer.FirstElement = 0;
				desc.Buffer.NumElements = dataCount;

				auto result = device->CreateShaderResourceView(buffer, &desc, bufferSRV.reset());
				ALWAYS_ASSERT(result >= 0);
			}

			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC desc{};
				desc.Format = DXGI_FORMAT_UNKNOWN;
				desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
				desc.Buffer.FirstElement = 0;
				desc.Buffer.NumElements = dataCount;
				
				auto result = device->CreateUnorderedAccessView(buffer, &desc, bufferUAV.reset());
				ALWAYS_ASSERT(result >= 0);
			}
		}

		void createRWBuffer(T* data, int dataCount, DXGI_FORMAT format, ID3D11Device* device, UINT additionalMiscFlags)
		{
			DEV_ASSERT(data != nullptr);

			{
				D3D11_BUFFER_DESC desc{};
				desc.ByteWidth = sizeof(T) * dataCount;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
				desc.MiscFlags = additionalMiscFlags;

				D3D11_SUBRESOURCE_DATA sr_data = {};
				sr_data.pSysMem = data;

				auto result = device->CreateBuffer(&desc, &sr_data, buffer.reset());
				ALWAYS_ASSERT(result >= 0);
			}
			
			{
				D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
				desc.Format = format;
				desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				desc.Buffer.FirstElement = 0;
				desc.Buffer.NumElements = dataCount;
			
				auto result = device->CreateShaderResourceView(buffer, &desc, bufferSRV.reset());
				ALWAYS_ASSERT(result >= 0);
			}

			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC desc{};
				desc.Format = format;
				desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
				desc.Buffer.FirstElement = 0;
				desc.Buffer.NumElements = dataCount;

				auto result = device->CreateUnorderedAccessView(buffer, &desc, bufferUAV.reset());
				ALWAYS_ASSERT(result >= 0);
			}
		}

		ID3D11UnorderedAccessView* getRWBufferUAV()
		{
			return bufferUAV.ptr();
		}

		void setUAVForCS(ID3D11DeviceContext4* devcon, int slot = 0)
		{
			auto* uav = bufferUAV.ptr();
			devcon->CSSetUnorderedAccessViews(slot, 1, &uav, nullptr);
		}

		void unsetUAVForCS(ID3D11DeviceContext4* devcon, int slot = 0)
		{
			ID3D11UnorderedAccessView* const uav[1] = { NULL };
			devcon->CSSetUnorderedAccessViews(slot, 1, uav, nullptr);
		}

		void setBufferForVS(ID3D11DeviceContext4* devcon, int slot = 0)
		{
			auto* srv = bufferSRV.ptr();
			devcon->VSSetShaderResources(slot, 1, &srv);
		}

		void unsetBufferForVS(ID3D11DeviceContext4* devcon, int slot = 0)
		{
			ID3D11ShaderResourceView* srv[] = { NULL };
			devcon->VSSetShaderResources(slot, 1, srv);
		}

		void setBufferForPS(ID3D11DeviceContext4* devcon, int slot = 0)
		{
			auto* srv = bufferSRV.ptr();
			devcon->PSSetShaderResources(slot, 1, &srv);
		}

		void setVertexBufferForInputAssembler(ID3D11DeviceContext4* devcon, int slot = 0) const
		{
			ID3D11Buffer* bufferPtr = buffer.ptr();
			UINT vertexStride = sizeof(T);
			UINT vertexOffset = 0;

			devcon->IASetVertexBuffers(slot, 1, &bufferPtr, &vertexStride, &vertexOffset);
		}

		void setInstanceBufferForInputAssembler(ID3D11DeviceContext4* devcon, int slot = 1) const
		{
			ID3D11Buffer* bufferPtr = buffer.ptr();
			UINT vertexStride = sizeof(T);
			UINT vertexOffset = 0;

			devcon->IASetVertexBuffers(slot, 1, &bufferPtr, &vertexStride, &vertexOffset);
		}

		void setIndexBufferForInputAssembler(ID3D11DeviceContext4* devcon) const
		{
			devcon->IASetIndexBuffer(buffer, DXGI_FORMAT_R32_UINT, 0);
		}

		D3D11_MAPPED_SUBRESOURCE& map(ID3D11DeviceContext4* devcon)
		{
			devcon->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &bufferSubresource);
			return bufferSubresource;
		}

		void unmap(ID3D11DeviceContext4* devcon)
		{
			devcon->Unmap(buffer, 0);
		}

		void setConstantBufferForVertexShader(ID3D11DeviceContext4* devcon, int slot = 0)
		{
			auto buffer = this->buffer.ptr();
			devcon->VSSetConstantBuffers(slot, 1, &buffer);
		}

		void setConstantBufferForGeometryShader(ID3D11DeviceContext4* devcon, int slot = 0)
		{
			auto buffer = this->buffer.ptr();
			devcon->GSSetConstantBuffers(slot, 1, &buffer);
		}

		void setConstantBufferForPixelShader(ID3D11DeviceContext4* devcon, int slot = 0)
		{
			auto buffer = this->buffer.ptr();
			devcon->PSSetConstantBuffers(slot, 1, &buffer);
		}

		void setConstantBufferForComputeShader(ID3D11DeviceContext4* devcon, int slot = 0)
		{
			auto buffer = this->buffer.ptr();
			devcon->CSSetConstantBuffers(slot, 1, &buffer);
		}

		ID3D11Buffer* getBufferPtr()
		{
			return buffer.ptr();
		}

		void reset()
		{
			buffer.reset();
		}
	private:
		DxResPtr<ID3D11Buffer> buffer;
		D3D11_MAPPED_SUBRESOURCE bufferSubresource;
		DxResPtr<ID3D11ShaderResourceView> bufferSRV;
		DxResPtr<ID3D11UnorderedAccessView> bufferUAV;
		int capacity;
	};
}
