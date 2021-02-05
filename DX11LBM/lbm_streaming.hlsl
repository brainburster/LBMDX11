Texture2D<float4> inTex : register(t0);
RWTexture2D<float4> outTex : register(u0);
RWTexture2DArray<float> f_in : register(u1);
RWTexture2DArray<float> f_out : register(u2);

static const int2 v[9] = { { 1, 1 }, { 1, 0 }, { 1, -1 }, { 0, 1 }, { 0, 0 }, { 0, -1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };

void bounceBack(uint2 index)
{
    if (inTex[index].w /*|| index.y == 0 || index.y == 599*/)
    {
		[unroll(9)]
        for (uint i = 0; i < 9; i++)
        {
            f_out[uint3(index, i)] = f_in[uint3(index, 8 - i)];
        }
    }
}


uint2 cycle(int2 index, int2 v, int2 size)
{
    //return index + v;
    return (size + index + v) % size;
    //return uint2(index.x + v.x, (size.y + index.y + v.y) % size.y);
}

void streaming(uint2 index)
{
	[unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        uint2 id = cycle(index, v[8-i], int2(800, 600));
        f_in[uint3(id, i)] = f_out[uint3(index, i)];
    }
}

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const int2 index = DTid.xy;
    bounceBack(index);
    streaming(index);
    //DeviceMemoryBarrierWithGroupSync();
    
    float p = 0;
    [unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        p += f_in[uint3(index, i)];
    }
    p /= 3;
    float display = saturate(p*p*p*p*20);
    outTex[index] = saturate(float4(display, display, display, 1) - inTex[index]);
}
