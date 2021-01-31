Texture2D<float4> inTex : register(t0);
RWTexture2D<float4> outTex : register(u0);
RWTexture2D<float4> f_in : register(u1);
RWTexture2D<float4> f_out : register(u2);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 color = float4(0.5, 0.5, 0.5, 1);
	outTex[DTid.xy] = clamp(color - inTex[DTid.xy], float4(0, 0, 0, 0), float4(1, 1, 1, 1));
}
