Texture2D<float4> inTex : register(t0);
RWTexture2D<float4> outTex : register(u0);
RWTexture2D<float4> f_in : register(u1);
RWTexture2D<float4> f_out : register(u2);

void collisionStep(uint2 index)
{
}

void streamingStep(uint2 index)
{
}

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	collisionStep(DTid.xy);
	GroupMemoryBarrierWithGroupSync();
	streamingStep(DTid.xy);
	GroupMemoryBarrierWithGroupSync();
	outTex[DTid.xy] = clamp(f_in[DTid.xy] + inTex[DTid.xy], float4(0, 0, 0, 0), float4(1, 1, 1, 1));
}
