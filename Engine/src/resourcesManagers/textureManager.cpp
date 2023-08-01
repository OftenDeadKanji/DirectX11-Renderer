#include "textureManager.h"
#include "../dependencies/DirectXTex/DirectXTex/DDSTextureLoader11.h"
#include "../dependencies/DirectXTex/DirectXTex/DirectXTex.h"
#include "../utils/random/random.h"
#include "../utils/assert.h"
#include "../dependencies/sivPerlinNoise.h"

namespace Engine
{
    TextureManager* TextureManager::s_instance = nullptr;

    TextureManager* Engine::TextureManager::createInstance()
    {
        return s_instance = new TextureManager;
    }

    void TextureManager::deleteInstance()
    {
        s_instance->deinit();
        delete s_instance;
        s_instance = nullptr;
    }

    TextureManager* TextureManager::getInstance()
    {
        return s_instance;
    }

    void TextureManager::init()
    { 
        generatePerlinNoise3DTexture();
    }

    void TextureManager::deinit()
    {
        deleteAllTextures();
    }

    std::shared_ptr<Texture> TextureManager::getTexture(const std::wstring& filePath)
    {
        if (auto iter = m_textures.find(filePath); iter != m_textures.end())
        {
            return iter->second;
        }

        return loadTexture(filePath);
    }

    std::shared_ptr<Texture> TextureManager::getPerlinNoise3DTexture()
    {
        return m_textures[PERLIN_NOISE_3D_TEXTURE_NAME];
    }

    void TextureManager::generatePerlinNoise3DTexture()
    {
        std::shared_ptr<Texture> texture = std::make_shared<Texture>();

        auto* random = Random::getInstance();

        const math::Vec3i size(256, 256, 256);
        std::vector<float> tex;
        tex.reserve(size.x() * size.y() * size.z());

        siv::BasicPerlinNoise<float>::seed_type seed = static_cast<unsigned int>(random->getRandomFloat(0.0f, static_cast<float>(std::numeric_limits<unsigned int>::max())));
        siv::BasicPerlinNoise<float> perlinNoise(seed);

        float minNoise = std::numeric_limits<float>::max(), maxNoise = std::numeric_limits<float>::lowest();
        for (int i = 0; i < size.x(); i++)
        {
            for (int j = 0; j < size.y(); j++)
            {
                for (int k = 0; k < size.z(); k++)
                {
                    //float noise = random->noise((float)i / size.x(), (float)j/size.y(), (float)k/size.z());
                   
                    float noise = perlinNoise.noise3D_01(5 * (float)i / size.x(), 5 * (float)j / size.y(), 5 * (float)k / size.z());

                    //minNoise = std::min(minNoise, noise);
                    //maxNoise = std::max(maxNoise, noise);

                    //uint8_t noiseInt = static_cast<float>(noise * 255);
                    tex.push_back(noise);
                }
            }
        }

        //for (auto& t : tex)
        //{
        //    t = (t - minNoise) / (maxNoise - minNoise);
        //}

        D3D11_TEXTURE3D_DESC desc{};
        desc.Width = size.x();
        desc.Height = size.y();
        desc.Depth = size.z();
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R32_FLOAT;
        //desc.Format = DXGI_FORMAT_R8_UINT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA data{};
        data.pSysMem = tex.data();
        data.SysMemPitch = size.x() * sizeof(float);
        data.SysMemSlicePitch = size.x() * size.y() * sizeof(float);

        auto result = D3D::getInstancePtr()->getDevice()->CreateTexture3D(&desc, &data, texture->m_texture3D.reset());
        ALWAYS_ASSERT(result >= 0);
        
        result = D3D::getInstancePtr()->getDevice()->CreateShaderResourceView(texture->m_texture3D, nullptr, texture->m_shaderResourceView.reset());
        ALWAYS_ASSERT(result >= 0);

        m_textures.insert({ PERLIN_NOISE_3D_TEXTURE_NAME, texture });
    }

    std::shared_ptr<Texture> TextureManager::loadTexture(const std::wstring& filePath)
    {
        std::shared_ptr<Texture> texture = std::make_shared<Texture>();

        auto* d3d = D3D::getInstancePtr();
        auto result = DirectX::CreateDDSTextureFromFile(d3d->getDevice(), d3d->getDeviceContext(), filePath.c_str(), (ID3D11Resource**)(texture->m_texture2D.reset()), texture->m_shaderResourceView.reset());

        D3D11_TEXTURE2D_DESC desc;
        texture->m_texture2D->GetDesc(&desc);

        texture->m_size = math::Vec2i(desc.Width, desc.Height);
        texture->m_mipLevels = desc.MipLevels;

        return m_textures.insert({ filePath, texture }).first->second;
    }

    void TextureManager::deleteAllTextures()
    {
        m_textures.clear();
    }

}