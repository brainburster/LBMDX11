Texture2D<float4> inTex : register(t0);
RWTexture2D<float4> outTex : register(u0);
RWTexture2DArray<float> f_in : register(u1);
RWTexture2DArray<float> f_out : register(u2);




[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const int2 index = DTid.xy;
    float rho[5] = { 0, 0, 0, 0, 0 };
    [unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        rho[0] += f_in[uint3(index, i)];
        rho[1] += f_in[uint3(index + int2(-1,0), i)];
        rho[2] += f_in[uint3(index + int2(1, 0), i)];
        rho[3] += f_in[uint3(index + int2(0, -1), i)];
        rho[4] += f_in[uint3(index + int2(0, 1), i)];
    }
    
    float d = 0;
    [unroll(5)]
    for (uint j = 1; j < 5;j++)
    {
        d += pow(rho[j] - rho[0], 2);
    }

    float w = 1-step(1e-4f,d)*0.2f;
    outTex[index] = lerp(saturate(float4(1.f, w, w, 1.f) * pow(rho[0], 16) * 0.0001f), float4(0.15f, 0.15f, 0.15f, 1.f),
    saturate((inTex[index]).r * 2));
}