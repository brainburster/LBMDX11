RWTexture2DArray<float> f : register(u0);

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 index = DTid.xy;

    f[uint3(index, 0)] = 0.16f;
    f[uint3(index, 1)] = 0.17f;
    f[uint3(index, 2)] = 0.16f;
    f[uint3(index, 3)] = 0.2f;
    f[uint3(index, 4)] = 0.2f;
    f[uint3(index, 5)] = 0.2f;
    f[uint3(index, 6)] = 0.2f;
    f[uint3(index, 7)] = 0.2f;
    f[uint3(index, 8)] = 0.2f;
}