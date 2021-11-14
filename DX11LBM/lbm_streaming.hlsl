Texture2D<float4> inTex : register(t0);
RWTexture2D<float4> outTex : register(u0);
RWTexture2DArray<float> f_in : register(u1);
RWTexture2DArray<float> f_out : register(u2);

static const int2 v[9] = { { 1, 1 }, { 1, 0 }, { 1, -1 }, { 0, 1 }, { 0, 0 }, { 0, -1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };

uint2 cycle(int2 pos, int2 v, int2 size)
{
    return (size + pos + v) % size;
}

void streaming(uint2 pos,inout float f[9])
{
	[unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        uint2 pos2 = cycle(pos, v[8 - i], int2(640, 320));
        if (inTex[pos2].w > 0.1f || pos2.y == 0)
        {
            f[i] = f_in[uint3(pos, 8 - i)];
        }
        else
        {
            f[i] = f_in[uint3(pos2, i)];
        }
    }
}

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const int2 pos = DTid.xy;
    float f[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    streaming(pos,f);

    if (pos.x == 639)
    {
        //f[0] = f_in[uint3(pos + int2(-1, 0), 0)];
        //f[1] = f_in[uint3(pos + int2(-1, 0), 1)];
        //f[2] = f_in[uint3(pos + int2(-1, 0), 2)];
        f[0] = 0.5 * (f_in[uint3(pos + int2(-1, 0), 0)] + f_in[uint3(pos + int2(-2, 0), 0)]);
        f[1] = 0.5 * (f_in[uint3(pos + int2(-1, 0), 1)] + f_in[uint3(pos + int2(-2, 0), 1)]);
        f[2] = 0.5 * (f_in[uint3(pos + int2(-1, 0), 2)] + f_in[uint3(pos + int2(-2, 0), 2)]);
    }
    
  	[unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        f_out[uint3(pos, i)] = f[i];
    }
}
