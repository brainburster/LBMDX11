#include "cs_header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const float2 index = DTid.xy;
    for (int i = 0; i < num_control_point; i++)
    {
        ControlPoint cp = control_points[i];
        if (distance(index, cp.pos) < cp.dis)
        {
            quantities[index] = float4(quantities[index].xyz, cp.data);
            //if (cp.data<0.0)
            //{
            //    //quantities[index].w = cp.data;
            //}
        }
    }
}