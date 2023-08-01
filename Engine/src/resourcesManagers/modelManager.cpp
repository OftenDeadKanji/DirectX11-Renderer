#include "modelManager.h"
#include "assimp/postprocess.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "../utils/assert.h"
#include <algorithm>

namespace Engine
{
	ModelManager* ModelManager::s_instance = nullptr;

	ModelManager* ModelManager::createInstance()
	{
		if (!s_instance)
		{
			s_instance = new ModelManager();
		}

		return s_instance;
	}

	void ModelManager::deleteInstance()
	{
		s_instance->deinit();
		delete s_instance;

		s_instance = nullptr;
	}

	ModelManager* ModelManager::getInstancePtr()
	{
		return s_instance;
	}

	void ModelManager::deinit()
	{
		deleteAllModels();
	}

	std::shared_ptr<Model> ModelManager::getModel(const std::string& filePath)
	{
		if (auto iter = m_models.find(filePath); iter != m_models.end())
		{
			return iter->second;
		}
		
		return loadModel(filePath);
	}
	std::shared_ptr<Model> ModelManager::getUnitSphereModel()
	{
		if (auto iter = m_basicShapesModels.find(UNIT_SPHERE_MODEL_NAME); iter != m_basicShapesModels.end())
		{
			return iter->second;
		}

		return createUnitSphereModel();
	}
	

	std::shared_ptr<Model> ModelManager::loadModel(const std::string& filePath)
	{
		uint32_t flags = uint32_t(aiProcess_Triangulate | aiProcess_GenBoundingBoxes | aiProcess_ConvertToLeftHanded | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);

		Assimp::Importer importer;
		const aiScene* assimpScene = importer.ReadFile(filePath, flags);
		DEV_ASSERT(assimpScene);

		int numMeshes = assimpScene->mNumMeshes;
		std::shared_ptr<Model> model(new Model());
		
		model->name = filePath;
		std::replace(model->name.begin(), model->name.end(), '\\', '/');

		model->boundingBox.reset();
		model->m_meshes.resize(numMeshes);

		static_assert(sizeof(math::Vec3f) == sizeof(aiVector3D));

		for (int i = 0; i < numMeshes; i++)
		{
			auto& srcMesh = assimpScene->mMeshes[i];
			auto& dstMesh = model->m_meshes[i];

			dstMesh.name = srcMesh->mName.C_Str();
			dstMesh.boundingBox.min = reinterpret_cast<math::Vec3f&>(srcMesh->mAABB.mMin);
			dstMesh.boundingBox.max = reinterpret_cast<math::Vec3f&>(srcMesh->mAABB.mMax);

			model->boundingBox.expand(dstMesh.boundingBox);

			dstMesh.vertices.resize(srcMesh->mNumVertices);
			dstMesh.triangles.resize(srcMesh->mNumFaces);

			for (int v = 0; v < srcMesh->mNumVertices; v++)
			{
				math::Vertex& vertex = dstMesh.vertices[v];

				vertex.position = reinterpret_cast<math::Vec3f&>(srcMesh->mVertices[v]);
				vertex.color = math::Vec4f(1.0f, 0.0f, 1.0f, 1.0f);
				vertex.textureCoordinates = reinterpret_cast<math::Vec2f&>(srcMesh->mTextureCoords[0][v]);
				vertex.normal = reinterpret_cast<math::Vec3f&>(srcMesh->mNormals[v]);
				vertex.tangent = reinterpret_cast<math::Vec3f&>(srcMesh->mTangents[v]);
				vertex.bitangent = reinterpret_cast<math::Vec3f&>(srcMesh->mBitangents[v]) * -1.0f;
			}
			math::Vertex* array = dstMesh.vertices.data();

			for (int f = 0; f < srcMesh->mNumFaces; f++)
			{
				const auto& face = srcMesh->mFaces[f];
				DEV_ASSERT(face.mNumIndices == 3);

				dstMesh.triangles[f].verticesArray = array;
				for (int index = 0; index < face.mNumIndices; index++)
				{
					dstMesh.triangles[f].vertexIndices[index] = face.mIndices[index];
				}

				dstMesh.triangles[f].computeNormalVector();
			}

			auto* assimpMaterial = assimpScene->mMaterials[srcMesh->mMaterialIndex];
			aiString diffuse, metalness, roughness;
			assimpMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &diffuse);
			assimpMaterial->GetTexture(aiTextureType_METALNESS, 0, &metalness);
			assimpMaterial->GetTexture(aiTextureType_SHININESS, 0, &roughness);


			dstMesh.initializeOctree();
		}

		std::function<void(aiNode*)> loadInstances;
		loadInstances = [&loadInstances, &model](aiNode* node)
		{
			const math::Mat4f nodeToParent = reinterpret_cast<const math::Mat4f&>(node->mTransformation.Transpose());
			const math::Mat4f parentToNode = nodeToParent.inverse();

			for (int i = 0; i < node->mNumMeshes; i++)
			{
				int meshIndex = node->mMeshes[i];
				model->m_meshes[meshIndex].instances.push_back(nodeToParent);
				model->m_meshes[meshIndex].instancesInv.push_back(parentToNode);
			}

			for (int i = 0; i < node->mNumChildren; i++)
			{
				loadInstances(node->mChildren[i]);
			}
		};

		loadInstances(assimpScene->mRootNode);
		model->createVertexBuffer();

		return m_models.insert({ filePath, model }).first->second;
	}
	
	std::shared_ptr<Model> ModelManager::createUnitSphereModel()
	{
		const uint32_t SIDES = 6;
		const uint32_t GRID_SIZE = 12;
		const uint32_t TRIS_PER_SIDE = GRID_SIZE * GRID_SIZE * 2;
		const uint32_t VERT_PER_SIZE = 3 * TRIS_PER_SIDE;

		std::shared_ptr<Model> modelPtr(new Model());
		Model& model = *modelPtr;

		model.name = "UNIT_SPHERE_FLAT";
		model.boundingBox = math::Box::empty();
		
		Mesh& mesh = model.m_meshes.emplace_back();
		mesh.name = "UNIT_SPHERE_FLAT";
		mesh.boundingBox = model.boundingBox;
		mesh.instances = { math::Mat4f::Identity() };
		mesh.instancesInv = { math::Mat4f::Identity() };

		mesh.vertices.resize(VERT_PER_SIZE * SIDES);
		math::Vertex* vertex = mesh.vertices.data();

		mesh.triangles.resize(VERT_PER_SIZE * SIDES / 3.0f);
		math::Triangle* triangles = mesh.triangles.data();
		int index = 0;

		int sideMasks[6][3] =
		{
			{ 2, 1, 0 },
			{ 0, 1, 2 },
			{ 2, 1, 0 },
			{ 0, 1, 2 },
			{ 0, 2, 1 },
			{ 0, 2, 1 }
		};

		float sideSigns[6][3] =
		{
			{ +1, +1, +1 },
			{ -1, +1, +1 },
			{ -1, +1, -1 },
			{ +1, +1, -1 },
			{ +1, -1, -1 },
			{ +1, +1, +1 }
		};

		for (int side = 0; side < SIDES; ++side)
		{
			for (int row = 0; row < GRID_SIZE; ++row)
			{
				for (int col = 0; col < GRID_SIZE; ++col)
				{
					float left = (col + 0) / float(GRID_SIZE) * 2.f - 1.f;
					float right = (col + 1) / float(GRID_SIZE) * 2.f - 1.f;
					float bottom = (row + 0) / float(GRID_SIZE) * 2.f - 1.f;
					float top = (row + 1) / float(GRID_SIZE) * 2.f - 1.f;

					math::Vec3f quad[4] =
					{
						{ left, bottom, 1.f },
						{ right, bottom, 1.f },
						{ left, top, 1.f },
						{ right, top, 1.f }
					};

					math::Vertex zero{};
					vertex[0] = vertex[1] = vertex[2]  = zero;

					auto setPos = [sideMasks, sideSigns](int side, math::Vertex& dst, const math::Vec3f& pos)
					{
						dst.position[sideMasks[side][0]] = pos.x() * sideSigns[side][0];
						dst.position[sideMasks[side][1]] = pos.y() * sideSigns[side][1];
						dst.position[sideMasks[side][2]] = pos.z() * sideSigns[side][2];
						dst.position = dst.position.normalized();
					};

					setPos(side, vertex[0], quad[0]);
					setPos(side, vertex[1], quad[1]);
					setPos(side, vertex[2], quad[2]);

					{
						math::Vec3f AB = vertex[1].position - vertex[0].position;
						math::Vec3f AC = vertex[2].position - vertex[0].position;
						vertex[0].normal = vertex[1].normal = vertex[2].normal = AC.cross(AB).normalized();
					}
					triangles[0].vertexIndices[0] = index;
					triangles[0].vertexIndices[1] = index + 2;
					triangles[0].vertexIndices[2] = index + 1;
					triangles->verticesArray = mesh.vertices.data();
					triangles->computeNormalVector();
					
					vertex += 3;
					triangles += 1;
					index += 3;

					setPos(side, vertex[0], quad[1]);
					setPos(side, vertex[1], quad[3]);
					setPos(side, vertex[2], quad[2]);

					{
						math::Vec3f AB = vertex[1].position - vertex[0].position;
						math::Vec3f AC = vertex[2].position - vertex[0].position;
						vertex[0].normal = vertex[1].normal = vertex[2].normal = AC.cross(AB).normalized();
					}
					triangles[0].vertexIndices[0] = index;
					triangles[0].vertexIndices[1] = index + 2;
					triangles[0].vertexIndices[2] = index + 1;
					triangles->verticesArray = mesh.vertices.data();
					triangles->computeNormalVector();
					
					vertex += 3;
					triangles += 1;
					index += 3;
				}
			}
		}

		// list of (vertices group and avg normal)
		std::list<std::pair<std::list<math::Vertex*>, math::Vec3f>> vertices;

		for (auto& vert : mesh.vertices)
		{
			bool added = false;
			for (auto& vertGroup : vertices)
			{
				if ((vert.position - vertGroup.first.front()->position).norm() < 0.0001f)
				{
					vertGroup.first.push_back(&vert);
					vertGroup.second += vert.normal;

					added = true;
					break;
				}
			}

			if (!added)
			{
				vertices.emplace_back();
				vertices.back().first.push_back(&vert);
				vertices.back().second = vert.normal;
			}
		}

		// calculating averaged normals
		for (auto& vertGroup : vertices)
		{
			vertGroup.second.normalize();
			for (auto* vert : vertGroup.first)
			{
				vert->normal = vertGroup.second;
			}
		}

		mesh.createBoundingBox();
		mesh.initializeOctree();
		
		model.createVertexBuffer();
		return m_basicShapesModels.insert({ UNIT_SPHERE_MODEL_NAME, modelPtr }).first->second;
	}

	void ModelManager::deleteAllModels()
	{
		m_models.clear();
	}
}