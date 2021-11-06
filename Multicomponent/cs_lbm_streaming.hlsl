#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

bool is_wall(uint2 pos)
{
    if (pos.x < 0 || pos.y > (600 / 4 - 1) || pos.y < 0 || pos.x > (800 / 4 - 1))
    {
        return true;
    }
    return uav_display[pos].w < 0.0;
}

[numthreads(16, 16, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    
    float f0[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint i = 0;
    bool flag1 = is_wall(pos);
    f0[0] = f_out[0][uint3(pos, 0)];
    for (i = 1; i < 9; i++)
    {
        uint2 pos_2 = pos + c[oppo[i]];
        bool flag2 = is_wall(pos_2);
        if (flag1)
        {
            f0[i] = 0.0;
        }
        else if (flag2)
        {
            //半时间步长反弹
            f0[i] = f_out[0][uint3(pos, oppo[i])];
        }
        else
        {
            f0[i] = f_out[0][uint3(pos_2, i)];
        }
    }
    
    [unroll]
    for (i = 0; i < 9; i++)
    {
        f_in[0][uint3(pos,i)] = f0[i];
    }
}