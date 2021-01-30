Texture2D<float4> tex0 : register(t0);
RWTexture2D<float4> tex1 : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 color = float4(0.5, 0.5, 0.5, 1);
	tex1[DTid.xy] = clamp(color - tex0[DTid.xy], float4(0, 0, 0, 0), float4(1, 1, 1, 1));
}
