#include "decalSystem.h"
#include "../camera/camera.h"
#include "../../engine/renderer.h"
#include "../../utils/random/random.h"
#include <numbers>
#include "../meshSystem/meshSystem.h"

namespace Engine
{
	DecalSystem* DecalSystem::s_instance = nullptr;
	std::shared_ptr<Model> DecalSystem::s_unitCubeModel = nullptr;

	DecalSystem* DecalSystem::createInstance()
	{
		return s_instance = new DecalSystem();
	}
	DecalSystem* DecalSystem::getInstance()
	{
		return s_instance;
	}
	void DecalSystem::deleteInstance()
	{
		s_instance->deinit();

		delete s_instance;
		s_instance = nullptr;
	}
	void DecalSystem::init()
	{
		s_unitCubeModel = ModelManager::getInstancePtr()->getModel("Assets/Models/Cube/cube.fbx");

		std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc =
		{
			{"POS",			0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0,								D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COL",			0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 16,								D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEX",			0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORM",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TANG",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"BTANG",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0},

			{"INS",			0, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, 0,								D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INS",			1, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INS",			2, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INS",			3, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INSINV",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INSINV",		1, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INSINV",		2, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INSINV",		3, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"CLR",			0, DXGI_FORMAT_R32G32B32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"OID",			0, DXGI_FORMAT_R32_UINT,			1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1}
		};

		m_shader.init(L"Shaders/decals/decalVS.hlsl", L"Shaders/decals/decalPS.hlsl", inputElementDesc);
	}
	void DecalSystem::deinit()
	{
		s_unitCubeModel = nullptr;
	}
	void DecalSystem::render(Camera& camera)
	{
		if (!m_instances.empty())
		{
			Renderer::getInstancePtr()->setFrontFaceCulling();

			createAndBindGBufferCopy();

			m_instanceBuffer.createInstanceBuffer(m_instances.size(), nullptr, D3D::getInstancePtr()->getDevice());
			auto& mappedResource = m_instanceBuffer.map(D3D::getInstancePtr()->getDeviceContext());

			auto* trSystem = TransformSystem::getInstance();
			DecalInstanceInternal* instances = static_cast<DecalInstanceInternal*>(mappedResource.pData);
			for (int i = 0; i < m_instances.size(); i++)
			{
				instances[i].decalToWorld = m_instances[i].decalToModel * trSystem->getMatrix(m_instances[i].objectTransformID);
				instances[i].color = m_instances[i].color;
				instances[i].objectID = m_instances[i].objectID;

#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
				math::setTranslation(instances[i].decalToWorld, math::getTranslation(instances[i].decalToWorld) - camera.position());
#endif
				instances[i].worldToDecal = instances[i].decalToWorld.inverse();
			}

			m_instanceBuffer.unmap(D3D::getInstancePtr()->getDeviceContext());

			m_shader.bind();

			Renderer::getInstancePtr()->setPerFrameGlobalSamplersForPS();
			m_instanceBuffer.setInstanceBufferForInputAssembler(D3D::getInstancePtr()->getDeviceContext());

			s_unitCubeModel->setVertexBufferForIA();
			s_unitCubeModel->setIndexBufferForIA();

			m_texture->bindSRVForPS(20);

			D3D::getInstancePtr()->getDeviceContext()->DrawIndexedInstanced(s_unitCubeModel->getMeshRange(0).indexNum, m_instances.size(), 0, 0, 0);

			unbindGBufferCopy();

			Renderer::getInstancePtr()->setBackFaceCulling();
		}
	}
	void DecalSystem::addInstance(const math::Vec3f& position, const math::Vec3f& direction, unsigned int objectID)
	{
		auto* trSystem = TransformSystem::getInstance();
		
		DecalInstance instance;
		instance.objectTransformID = MeshSystem::getInstancePtr()->getObjectTransformID(objectID);
		auto& objectTransform = trSystem->getMatrix(instance.objectTransformID);
		math::Mat4f objectTransformInv = objectTransform.inverse();

		float rotationAngle = Random::getInstance()->getRandomFloat(0.0f, 2.0f * std::numbers::pi_v<float>);

		math::Mat4f scale;
		scale <<
			DECAL_SIZE, 0.0, 0.0, 0.0,
			0.0, DECAL_SIZE, 0.0, 0.0,
			0.0, 0.0, DECAL_SIZE, 0.0,
			0.0, 0.0, 0.0, 1.0;

		math::Mat4f rotationZ;
		rotationZ << 
			cosf(rotationAngle), -sinf(rotationAngle), 0.0f, 0.0f,
			sinf(rotationAngle),  cosf(rotationAngle), 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		;

		math::Mat4f lookAt = math::lookAt(position, position + direction);

		math::Mat4f lookAtInv = lookAt.inverse();
		math::Mat4f decalToWorld = (scale * rotationZ) * lookAtInv;
		
		instance.decalToModel = decalToWorld * objectTransformInv;
		instance.objectID = objectID;
		instance.color = math::Vec3f(Random::getInstance()->getRandomFloat(), Random::getInstance()->getRandomFloat(), Random::getInstance()->getRandomFloat());

		m_instances.push_back(instance);
	}
	void DecalSystem::setTexture(std::shared_ptr<Texture> texture)
	{
		m_texture = texture;
	}
	void DecalSystem::createAndBindGBufferCopy()
	{
		Renderer::GBuffer& gbuffer = Renderer::getInstancePtr()->getGBuffer();

		D3D11_TEXTURE2D_DESC texDesc{};

		gbuffer.normal->GetDesc(&texDesc);
		D3D::getInstancePtr()->getDevice()->CreateTexture2D(&texDesc, nullptr, gbufferNormalCopy.reset());
		D3D::getInstancePtr()->getDeviceContext()->CopyResource(gbufferNormalCopy, gbuffer.normal);

		gbuffer.objectID->GetDesc(&texDesc);
		D3D::getInstancePtr()->getDevice()->CreateTexture2D(&texDesc, nullptr, gBufferObjectIDCopy.reset());
		D3D::getInstancePtr()->getDeviceContext()->CopyResource(gBufferObjectIDCopy, gbuffer.objectID);

		DxResPtr<ID3D11ShaderResourceView> srvNormal;
		D3D::getInstancePtr()->getDevice()->CreateShaderResourceView(gbufferNormalCopy, nullptr, srvNormal.reset());

		DxResPtr<ID3D11ShaderResourceView> srvID;
		D3D::getInstancePtr()->getDevice()->CreateShaderResourceView(gBufferObjectIDCopy, nullptr, srvID.reset());

		ID3D11ShaderResourceView* srvPtr[] = { 
			gbuffer.depthSRV.ptr(),
			srvNormal.ptr(), 
			srvID.ptr()
		};

		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(21, 3, srvPtr);
	}
	void DecalSystem::unbindGBufferCopy()
	{
		ID3D11ShaderResourceView* nullsrv[] = { NULL, NULL, NULL };
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(21, 3, nullsrv);
	}
}