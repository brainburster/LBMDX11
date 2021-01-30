Texture2D<float4> tex0 : register(t0);
RWTexture2D<float4> tex1 : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 color = float4(0.5, 1, 0.1, 0.6);
	tex1[DTid.xy] = color;
}
