Texture2D<float4> inTex : register(t0);
RWTexture2D<float4> outTex : register(u0);
RWTexture2DArray<float> f_in : register(u1);
RWTexture2DArray<float> f_out : register(u2);

static const int2 v[9] = { { 1, 1 }, { 1, 0 }, { 1, -1 }, { 0, 1 }, { 0, 0 }, { 0, -1 }, { -1, 1 }, { -1, 0 }, { -1, -1 } };
static const float w[9] =
{
    1.0f / 36, 1.0f / 9, 1.0f / 36,
    1.0f / 9,  4.0f / 9, 1.0f / 9,
    1.0f / 36, 1.0f / 9, 1.0f / 36
};

void collisionStep(uint2 index)
{
    //static const float omega = 1.8645;
    static const float omega =0.9;
    
    float f[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	[unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        f[i] = f_out[uint3(index, i)];
    }
    
    float rho = 0;
    [unroll(9)]
    for (uint j = 0; j < 9; j++)
    {
        rho += f[j];
    }
    
    float2 u = { 0, 0 };
    [unroll(9)]
    for (uint k = 0; k < 9; k++)
    {
        u += v[k] * f[k];
    }
    u /= rho;
    
    float vu[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    [unroll(9)]
    for (uint l = 0; l < 9; l++)
    {
        vu[l] = 3 * (v[l].x * u.x + v[l].y * u.y);
    }
    
    //const float p = rho / 3;
    const float usqr = 1.5 * (u.x * u.x + u.y * u.y);
    
    float eq[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    [unroll(9)]
    for (uint m = 0; m < 9; m++)
    {
        eq[m] = rho * w[m] * (1 + vu[m] + 0.5 * vu[m] * vu[m] - usqr);
    }
    
    [unroll(9)]
    for (uint n = 0; n < 9; n++)
    {
        f_out[uint3(index, n)] = f[n] - omega * (f[n] - eq[n]);
    }
}

void streamingStep(uint2 index)
{
	[unroll(9)]
    for (uint i = 0; i < 9; i++)
    {
        f_in[uint3(index + v[i], i)] = f_out[uint3(index, i)];
    }
}

void bounceBackAndInflow(uint2 index)
{
    if (inTex[index].w || index.y <= 0 || index.y >= 599)
    {
		[unroll(9)]
        for (uint i = 0; i < 9; i++)
        {
            f_out[uint3(index, i)] = f_in[uint3(index, 8 - i)];
        }
    }
    else
    {
        [unroll(9)]
        for (uint i = 0; i < 9; i++)
        {
            f_out[uint3(index, i)] = f_in[uint3(index, i)];
        }
    }
    
    if (index.x == 0 && index.y%60<10)
    {
        f_out[uint3(index, 0)] = f_out[uint3(index, 8)] + 0.05f;
        f_out[uint3(index, 1)] = f_out[uint3(index, 7)] + 0.05f;
        f_out[uint3(index, 2)] = f_out[uint3(index, 6)] + 0.05f;
    }
    if (index.x >= 799)
    {
        f_out[uint3(index, 6)] = f_out[uint3(index + uint2(-1, 0), 6)];
        f_out[uint3(index, 7)] = f_out[uint3(index + uint2(-1, 0), 7)];
        f_out[uint3(index, 8)] = f_out[uint3(index + uint2(-1, 0), 8)];
    }
}

float4 get_pixel_color(uint2 index)
{
    float4 color = float4(0, 0, 0, 1);
    color.z = f_out[uint3(index, 2)] + f_out[uint3(index, 5)] + f_out[uint3(index, 8)];
    color.x = f_out[uint3(index, 1)] + f_out[uint3(index, 4)] + f_out[uint3(index, 7)];
    color.y = f_out[uint3(index, 0)] + f_out[uint3(index, 3)] + f_out[uint3(index, 6)];
    color.x = color.x + color.y + color.z;
    color.y = color.x;
    color.z = color.x;
    return color/3;
}

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const int2 index = DTid.xy;
    collisionStep(index);
    GroupMemoryBarrierWithGroupSync();
    streamingStep(index);
    //GroupMemoryBarrierWithGroupSync();
    bounceBackAndInflow(index);
    //GroupMemoryBarrierWithGroupSync();
    outTex[DTid.xy] = saturate(get_pixel_color(index) + inTex[index]);
}
