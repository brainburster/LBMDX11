Texture2D<float4> inTex : register(t0);
RWTexture2D<float4> outTex : register(u0);
RWTexture2DArray<float> f_in : register(u1);
RWTexture2DArray<float> f_out : register(u2);

static const int2 v[9] = { { 1, 1 }, { 1, 0 }, { 1, -1 }, { 0, 1 }, { 0, 0 }, { 0, -1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };


void bounceBack(uint2 index)
{
    if (inTex[index].w > 0.1f)
    {
		[unroll(9)]
        for (uint i = 0; i < 9; i++)
        {
            f_in[uint3(index + v[i], 8 - i)] = f_in[uint3(index, i)];
        }
    }
}


uint2 cycle(int2 index, int2 v, int2 size)
{
    return (size + index + v) % size;
}

void streaming(uint2 index)
{
	[unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        uint2 id = cycle(index, v[8-i], int2(640, 320));
        f_in[uint3(id, i)] = f_out[uint3(index, i)];
    }
}

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const int2 index = DTid.xy;
    streaming(index);
    AllMemoryBarrier();
    bounceBack(index);
    //AllMemoryBarrier();
    if (index.x == 639)
    {
        f_in[uint3(index, 0)] = f_in[uint3(index + int2(-1, 0), 0)];
        f_in[uint3(index, 1)] = f_in[uint3(index + int2(-1, 0), 1)];
        f_in[uint3(index, 2)] = f_in[uint3(index + int2(-1, 0), 2)];
        if (index.y == 0)
        {
            f_in[uint3(index, 0)] = f_in[uint3(index + int2(-1, -1), 0)];
        }
        if (index.y == 319)
        {
            f_in[uint3(index, 2)] = f_in[uint3(index + int2(-1, 1), 2)];
        }
    }
}
