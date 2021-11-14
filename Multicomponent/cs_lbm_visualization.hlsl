#include "cs_header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    float rho0 = f_in[0][uint3(pos, 9)];
    float rho1 = f_in[1][uint3(pos, 9)];
    float rho2 = f_in[2][uint3(pos, 9)];
    
    
    float2 F =
    {
        f_out[0][uint3(pos, 10)] + f_out[1][uint3(pos, 10)] * 5.f,
        f_out[0][uint3(pos, 11)] + f_out[1][uint3(pos, 11)] * 5.f
    };
    
    uav_display[pos] = float4(rho0, length(F) * 0.5f, rho1*5, uav_display[pos].w);
    //uav_display[pos] = float4(rho0, rho2, rho1, uav_display[pos].w);
    
}
