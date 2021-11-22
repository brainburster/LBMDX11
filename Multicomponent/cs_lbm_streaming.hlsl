#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

bool is_wall(uint2 pos)
{
    if ( pos.y > (600 / 4 - 1) || pos.x > (800 / 4 - 1))
    {
        return true;
    }
    return uav_display[pos].w > 0.0f;
}

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;

    float f[3][9] =
    {
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };
    uint i = 0;
    uint j = 0;

    bool flag1 = is_wall(pos);
   
    
    [unroll]
    for (i = 1; i < 9; i++)
    {
        uint2 pos_2 = pos + c[oppo[i]];
        bool flag2 = is_wall(pos_2);
            
        [unroll]
        for (j = 0; j < 3; j++)
        {
            if (flag1)
            {
                f[j][i] = 0.0f;
            }
            else if (flag2)
            {
                f[j][i] = f_out[j][uint3(pos, oppo[i])];
            }
            else
            {
                f[j][i] = f_out[j][uint3(pos_2, i)];
            }
        }

    }
    
    [unroll]
    for (j = 0; j < 3; j++)
    {
        f[j][0] = f_out[j][uint3(pos, 0)];
        
        [unroll]
        for (i = 0; i < 9; i++)
        {
            f_in[j][uint3(pos, i)] = f[j][i];
        }
    }
}