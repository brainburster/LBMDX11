#include "cs_header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    float rho0 = f_in[0][uint3(pos, 9)];
    float2 F0 =
    {
        f_out[0][uint3(pos, 10)],
        f_out[0][uint3(pos, 11)]
    };
    uav_display[pos] = float4(rho0, length(F0)*0.5, 0, uav_display[pos].w);
}
