#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    for (uint i = 0; i < 9; i++)
    {
        f_in[2][uint3(DTid.xy, i)] = 0.01f;
        f_in[1][uint3(DTid.xy, i)] = 0.0002f;
        f_in[0][uint3(DTid.xy, i)] = 0.0002f;
    }
}
