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
        pow(f_out[0][uint3(pos, 10)], 2) + pow(f_out[0][uint3(pos, 11)], 2),
        pow(f_out[1][uint3(pos, 10)], 2) + pow(f_out[1][uint3(pos, 11)], 2)
    };
    
    float2 u =
    {
        pow(f_in[0][uint3(pos, 10)], 2) + pow(f_in[0][uint3(pos, 11)], 2),
        pow(f_in[1][uint3(pos, 10)], 2) + pow(f_in[1][uint3(pos, 11)], 2)
    };
    
    F = sqrt(F);
    u = sqrt(u);
    
    uint2 pos_r = pos + uint2(1u,0);
    uint2 pos_l = pos - uint2(1u,0);
    uint2 pos_u = pos - uint2(1u,0);
    uint2 pos_d = pos + uint2(1u,0);
    
    float u_l_y = f_in[0][uint3(pos_l, 11)] + f_in[1][uint3(pos_l, 11)];
    float u_r_y = f_in[0][uint3(pos_r, 11)] + f_in[1][uint3(pos_r, 11)];
    float u_u_x = f_in[0][uint3(pos_u, 10)] + f_in[1][uint3(pos_u, 10)];
    float u_d_x = f_in[0][uint3(pos_d, 10)] + f_in[1][uint3(pos_d, 10)];
    
    float du_ydx = (u_r_y - u_l_y) / 2;
    float du_xdy = (u_d_x - u_u_x) / 2;
    float vorticity = du_ydx - du_xdy;
    
    uav_display[pos] = float4(vorticitymode ? vorticity * 2.f : (rho0),
    //-(rho0 + rho1) * 500000.f +
    show_air * (rho2 * 100.f) +
    velocitymode * (u.x + u.y) * 0.5f +
    forcemode * (F.x*0.5f + F.y) +
    0,
    vorticitymode ? -vorticity*2.f : (rho1 * 10.f),
    uav_display[pos].w);
}
