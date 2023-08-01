#pragma once
#include "../../math/mathUtils.h"
#include "../Direct3d/d3d.h"
#include "../Direct3d/buffer.h"
#include "../../resourcesManagers/modelManager.h"
#include "../shader/shader.h"
#include "../texture/texture.h"
#include "../../transformSystem/transformSystem.h"

namespace Engine
{
	class Camera;

	class DecalSystem
	{
	public:
		static DecalSystem* createInstance();
		static DecalSystem* getInstance();
		static void deleteInstance();

		void init();
		void deinit();

		void render(Camera& camera);

		void addInstance(const math::Vec3f& position, const math::Vec3f& direction, unsigned int objectID);
		void setTexture(std::shared_ptr<Texture> texture);

	private:
		void createAndBindGBufferCopy();
		void unbindGBufferCopy();

		static DecalSystem* s_instance;
		static std::shared_ptr<Model> s_unitCubeModel;

		static constexpr float DECAL_SIZE = 0.2f;

		struct DecalInstance
		{
			math::Mat4f decalToModel;
			TransformSystem::ID objectTransformID;

			math::Vec3f color;
			unsigned int objectID;
		};

		struct DecalInstanceInternal
		{
			math::Mat4f decalToWorld;
			math::Mat4f worldToDecal;
			math::Vec3f color;
			unsigned int objectID;
		};
		std::vector<DecalInstance> m_instances;
		std::shared_ptr<Texture> m_texture;

		Buffer<DecalInstanceInternal> m_instanceBuffer;

		Shader m_shader;
		DxResPtr<ID3D11Texture2D> gbufferNormalCopy, gBufferObjectIDCopy;
	};
}
