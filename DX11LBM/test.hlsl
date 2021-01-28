RWTexture2D<float4> tex : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 color = float4(0.5, 1, 0.1, 0.6);
	tex[DTid.xy] = color;
}
