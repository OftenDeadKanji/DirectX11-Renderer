#ifndef __IBL_GENERATION_HLSL__
#define __IBL_GENERATION_HLSL__

#ifndef PI
	#define PI 3.14159
#endif

// ---------------- SAMPLE GENERATION ----------------

// Fibonacci hemisphere point uniform distribution
float3 randomHemisphere(out float NdotV, float i, float N)
{
	const float GOLDEN_RATIO = (1.0 + sqrt(5.0)) / 2.0;
	float theta = 2.0 * PI * i / GOLDEN_RATIO;
	float phiCos = NdotV = 1.0 - (i + 0.5) / N;
	float phiSin = sqrt(1.0 - phiCos * phiCos);
	float thetaCos, thetaSin;
	sincos(theta, thetaSin, thetaCos);
	return float3(thetaCos * phiSin, thetaSin * phiSin, phiCos);
}

// Frisvad with z == -1 problem avoidance
void basisFromDir(out float3 right, out float3 top, in float3 dir)
{
	float k = 1.0 / max(1.0 + dir.z, 0.00001);
	float a = dir.y * k;
	float b = dir.y * a;
	float c = -dir.x * a;
	right = float3(dir.z + b, c, -dir.x);
	top = float3(c, 1.0 - b, -dir.y);
}

// Frisvad with z == -1 problem avoidance
float3x3 basisFromDir(float3 dir)
{
	float3x3 rotation;
	rotation[2] = dir;
	basisFromDir(rotation[0], rotation[1], dir);
	return rotation;
}

float randomVanDeCorput(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// random2D from random1D
float2 randomHammersley(float i, float N)
{
	return float2(i / N, randomVanDeCorput(i));
}

// GGX importance sampling, returns microsurface normal (half-vector)
float3 randomGGX(float2 random, float rough2)
{
	float phi = 2.0 * PI * random.x;
	float cosTheta = sqrt((1.0 - random.y) / (1.0 + (rough2 - 1.0) * random.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	float3 dir;
	dir.x = cos(phi) * sinTheta;
	dir.y = sin(phi) * sinTheta;
	dir.z = cosTheta;
	return dir;
}

// GGX importance sampling, returns microsurface normal (half-vector)
float3 randomGGX(out float NdotH, uint index, uint N, float roughness, float3x3 rotation)
{
	float3 H = randomGGX(randomHammersley(index, N), roughness);
	NdotH = H.z;
	return mul(H, rotation);
}

// Determing which mip level to read in cubemap sampling with uniform/importance sampling
float hemisphereMip(float sampleProbability, float cubemapSize)
{
	float hemisphereTexels = cubemapSize * cubemapSize * 3;
	float log4 = 0.5 * log2(sampleProbability * hemisphereTexels);
	return log4;
}


#endif