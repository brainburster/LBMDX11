#include "cs_header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    float rho0 = f_in[0][uint3(pos, 9)];
    uav_display[pos] = float4(rho0, 0, 0, uav_display[pos].w);
}
