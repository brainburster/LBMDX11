Texture2D<float4> inTex : register(t0);
RWTexture2D<float4> outTex : register(u0);
RWTexture2D<float4> f_0 : register(u1);
RWTexture2D<float4> f_1 : register(u2);

static const int2 v[4] = { {1,0},{0,1},{0,-1}, {-1,0} };

void collisionStep(uint2 index)
{
	//static const float w[4] = { 0.25,0.25,0.25,0.25 };
	static const float w = 0.25;
	static const float omega = 0.005;
	const float4 f = f_0[index];
	const float rho = f.x + f.y + f.z + f.w;
	const float p = rho / 3;
	const float2 u = 1 / rho * (v[0] * f.x + v[1] * f.y + v[2] * f.z + v[3] * f.w);
	const float usqr = 1.5 * (u.x * u.x + u.y * u.y);
	const float2 _vu = 3 * (v[0] * u + v[1] * u + v[2] * u + v[3] * u);
	const float vu = _vu.x + _vu.y;
	const float eq = rho * w * (1 + vu + 0.5 * vu * vu - usqr);

	f_1[index] = f - omega * (f - eq);
}

void streamingStep(uint2 index)
{
	float4 f = float4(0, 0, 0, 0);
	[unroll(4)]
	for (uint i = 0; i < 4; i++)
	{
		float4 neighbor = f_1[index + v[i]];
		f[i] = neighbor[i];
	}
	f_0[index] = f;
}

void bounceBackAndInflow(uint2 index)
{
	float4 f = f_0[index];
	if (inTex[index].w || index.y<=0||index.y>=600)
	{
		[unroll(4)]
		for (uint i = 0; i < 4; i++)
		{
			float4 temp = f_0[index];
			f[i] = temp[3-i];
		}
	}
	else if (index.x == 0 &&index.y>250&&index.y<350&& index.y % 10 == 1)
	{
		f[3] = 1;
	}
	else if (index.x>=799)
	{
		f = float4(0,0,0,0);

	}
		f_1[index] = f;
	}

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const int2 index = DTid.xy;
	collisionStep(index);
	GroupMemoryBarrierWithGroupSync();
	streamingStep(index);
	GroupMemoryBarrierWithGroupSync();
	bounceBackAndInflow(index);
	GroupMemoryBarrierWithGroupSync();
	float4 f = f_1[DTid.xy];
	outTex[DTid.xy] = saturate((f.r + f.g + f.r + f.a) + inTex[DTid.xy]);
}
