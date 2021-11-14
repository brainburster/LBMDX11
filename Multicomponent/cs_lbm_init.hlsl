#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    f_in[2][uint3(DTid.xy, 0)] = 0.1f;
}