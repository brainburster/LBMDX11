#include "header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    
    f_in[uint3(pos, 0)] = 0.208f;
    f_in[uint3(pos, 1)] = 0.208f;
    f_in[uint3(pos, 2)] = 0.208f;
    f_in[uint3(pos, 3)] = 0.2f;
    f_in[uint3(pos, 4)] = 0.2f;
    f_in[uint3(pos, 5)] = 0.2f;
    f_in[uint3(pos, 6)] = 0.2f;
    f_in[uint3(pos, 7)] = 0.2f;
    f_in[uint3(pos, 8)] = 0.2f;
}