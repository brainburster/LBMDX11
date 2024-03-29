#include "cs_header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 index = DTid.xy;
    [loop] 
    for (int i = 0; i < num_control_point; i++)
    {
        ControlPoint cp = control_points[i];
        [branch]
        if (distance(index, cp.pos) < cp.dis)
        {
            
            [branch]
            if (cp.data == 3.0 && uav_display[index].w >= 0.f)
            {
                uav_display[index] = float4(uav_display[index].xyz, 1.0f);
            }
            else if (cp.data == 2.0)
            {
                f_in[0][uint3(index.xy, 0)] = 0.f;
                f_in[1][uint3(index.xy, 0)] = rho0[1] * 3.f;
            }
            else if (cp.data == 1.0)
            {
                f_in[0][uint3(index.xy, 0)] = rho0[0] * 3.f;
                f_in[1][uint3(index.xy, 0)] = 0.f;
            }
            else if (cp.data == 0.0)
            {
                uav_display[index] = float4(uav_display[index].xyz, 0.0f);
                f_in[0][uint3(index.xy, 0)] = 0.0f;
                f_in[1][uint3(index.xy, 0)] = 0.0f;
            }
        }
    }
}