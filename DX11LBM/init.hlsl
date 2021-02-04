RWTexture2DArray<float> f : register(u0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 index = DTid.xy;
    [unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        f[uint3(index, i)] = 0.00009f;
    }
}