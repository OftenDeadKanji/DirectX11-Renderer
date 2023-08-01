#include "texture.h"

namespace Engine
{
	void Texture::bindSRVForVS(int slot)
	{
		auto* SRV = m_shaderResourceView.ptr();
		D3D::getInstancePtr()->getDeviceContext()->VSSetShaderResources(slot, 1, &SRV);
	}
	void Texture::bindSRVForGS(int slot)
	{
		auto* SRV = m_shaderResourceView.ptr();
		D3D::getInstancePtr()->getDeviceContext()->GSSetShaderResources(slot, 1, &SRV);
	}
	void Texture::bindSRVForPS(int slot)
	{
		auto* SRV = m_shaderResourceView.ptr();
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(slot, 1, &SRV);
	}
	math::Vec2i Texture::getSize() const
	{
		return m_size;
	}
	unsigned int Texture::getMipLevels() const
	{
		return m_mipLevels;
	}
}
