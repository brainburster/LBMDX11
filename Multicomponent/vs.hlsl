#include "vs_ps_header.hlsli"

VsOut main( VsIn vs_in )
{
    VsOut vs_out;
    vs_out.posH = float4(vs_in.pos,0.f, 1.f);
    vs_out.uv = vs_in.uv;
    return vs_out;
}
