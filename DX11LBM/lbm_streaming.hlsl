#include "header.hlsli"

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{   
    const int2 pos = DTid.xy;

	[unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        uint2 pos2 = (int2(800, 600) + pos + c[8 - i]) % int2(800, 600);
        if (inTex[pos2].w > 0.0f || pos2.y == 0u)
        {
            f_out[uint3(pos, i)] = f_in[uint3(pos, 8 - i)];
        }
        else
        {
            f_out[uint3(pos, i)] = f_in[uint3(pos2, i)];
        }
    }
}
