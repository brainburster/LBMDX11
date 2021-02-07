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
    float w = 1;
    for (uint j = 1; j < 5;j++)
    {
        d += pow(rho[j] - rho[0], 2);
    }
    if (d>0.0001)
    {
        w = 0.8;
    }
    outTex[index] = saturate(float4(1, w, w, 1) * pow(rho[0], 5) * 0.1 + (inTex[index]) * 2);
}