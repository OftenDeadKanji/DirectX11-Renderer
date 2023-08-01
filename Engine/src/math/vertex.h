#pragma once
#include "mathUtils.h"

namespace Engine::math
{
	struct Vertex
	{
		Vec3f position;
		Vec4f color;
		Vec2f textureCoordinates;
		Vec3f normal;
		Vec3f tangent;
		Vec3f bitangent;
	};
}