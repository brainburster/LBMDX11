#include "logger.hpp"
#include "Blizzard.h"
#include <d3dcompiler.h>

#define HR_ASSERT(hr,info) 	do{ if (((HRESULT)(hr)) < 0)\
	{\
		Logger::error((char*)error->GetBufferPointer());\
		assert(0);\
	}}while (0);

void Blizzard::run()
{
	init();
	while (!dx11_wnd->app_should_close())
	{
		dx11_wnd->PeekMsg();
		update();
		render();
	}
}

void Blizzard::init()
{
	initShaders();
	initResources();
	bindResources();
	setInputCallback();
	auto ctx = dx11_wnd->GetImCtx();
	//...
	fence();
	const UINT ThreadGroupCountX = (dx11_wnd->getWidth() - 1) / 32 + 1;
	const UINT ThreadGroupCountY = (dx11_wnd->getWidth() - 1) / 32 + 1;
	ctx->CSSetShader(cs_init.Get(), NULL, 0);
	ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
	fence();
}

void Blizzard::fence()
{
	decltype(auto) device = dx11_wnd->GetDevice();
	decltype(auto) ctx = dx11_wnd->GetImCtx();
	ComPtr<ID3D11Query> event_query;
	D3D11_QUERY_DESC queryDesc{};
	queryDesc.Query = D3D11_QUERY_EVENT;
	queryDesc.MiscFlags = 0;
	device->CreateQuery(&queryDesc, event_query.GetAddressOf());
	ctx->End(event_query.Get());
	while (ctx->GetData(event_query.Get(), NULL, 0, 0) == S_FALSE) {}
}

void Blizzard::update()
{
	auto device = dx11_wnd->GetDevice();
	auto ctx = dx11_wnd->GetImCtx();
	const int width = dx11_wnd->getWidth();
	const int height = dx11_wnd->getHeight();
	const UINT ThreadGroupCountX = (width - 1) / 32 + 1;
	const UINT ThreadGroupCountY = (height - 1) / 32 + 1;
	//更新
	//应用控制点
	if (control_points.size() > 0)
	{
		fence();
		updateControlPointBuffer();
		control_points.clear();
		ctx->CSSetShader(cs_draw.Get(), NULL, 0);
		ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
	}
	fence();
	ctx->CSSetShader(cs_lbm1.Get(), NULL, 0);
	ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
	fence();
	ctx->CSSetShader(cs_lbm2.Get(), NULL, 0);
	ctx->Dispatch(2, 1, 1);
	fence();
	ctx->CSSetShader(cs_lbm3.Get(), NULL, 0);
	ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
	fence();
	updateRandSeed();
	ctx->CSSetShader(cs_snow1.Get(), NULL, 0);
	ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
	fence();
	ctx->CSSetShader(cs_snow2.Get(), NULL, 0);
	ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
	fence();
	ctx->CSSetShader(cs_snow3.Get(), NULL, 0);
	ctx->Dispatch((width / 2 - 1) / 32 + 1, (height / 2 - 1) / 32 + 1, 1);
	fence();
	ctx->CSSetShader(cs_snow4.Get(), NULL, 0);
	ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
	fence();
	ctx->CSSetShader(cs_visualization.Get(), NULL, 0);
	ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
	fence();
}

void Blizzard::render()
{
	auto ctx = dx11_wnd->GetImCtx();
	constexpr float back_color[4] = { 0.f,0.f,0.f,1.f };
	ctx->ClearRenderTargetView(dx11_wnd->GetRTV(), back_color);
	ctx->ClearDepthStencilView(dx11_wnd->GetDsv(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
	ID3D11ShaderResourceView* null_srv = nullptr;
	ID3D11UnorderedAccessView* null_uav = nullptr;
	ctx->CSSetUnorderedAccessViews(2, 1, &null_uav, 0);
	ctx->PSSetShaderResources(0, 1, srv_tex_display.GetAddressOf());
	fence();

	ctx->Draw(4, 0);

	ctx->CSSetUnorderedAccessViews(2, 1, uav_tex_display.GetAddressOf(), 0);
	ctx->PSSetShaderResources(0, 1, &null_srv);

	dx11_wnd->GetSwapChain()->Present(0, 0);
}

void Blizzard::initShaders()
{
	auto device = dx11_wnd->GetDevice();
	//#define STR(...) #__VA_ARGS__
	constexpr char vs_src[] = R"(
		struct VsIn
		{
			float2 pos : POSITION;
			float2 uv : TEXCOORD;
		};

		struct VsOut
		{
			float4 posH : SV_POSITION;
			float2 uv : TEXCOORD;
		};

		VsOut main(VsIn vs_in)
		{
			VsOut vs_out;
			vs_out.posH = float4(vs_in.pos, 0.f, 1.f);
			vs_out.uv = vs_in.uv;
			return vs_out;
		}
	)";

	constexpr char ps_src[] = R"(
		Texture2D<float4> srv_display : register(t0);

		SamplerState sampler0
		{
			Filter = MIN_MAG_MIP_LINEAR;
			AddressU = Wrap;
			AddressV = Wrap;
		};

		struct VsOut
		{
			float4 posH : SV_POSITION;
			float2 uv : TEXCOORD;
		};

		float4 main(VsOut vs_out) : SV_TARGET
		{
			float4 data = srv_display.Sample(sampler0, vs_out.uv);

			float4 wall_color = float4(0.25f, 0.25f, 0.25f, 1.0f)* abs(data.w);
			float4 snow_color = float4(0.55f,0.55f,0.55f,1.f);
			float4 snow_wall_color = float4(0.8f,0.8f,0.8f,1.f);
			float4 wind_color = float4(0.1f,data.r+0.1f,0.1f,1.f);

			return lerp(lerp(wind_color,lerp(snow_color,snow_wall_color,data.g),data.b+data.g),wall_color,data.w);
		}
	)";

	constexpr char cs_init_src[] = R"(
		RWTexture2DArray<float> f_in : register(u0);

		[numthreads(32, 32, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			f_in[uint3(DTid.xy, 1)] = .07f;
			f_in[uint3(DTid.xy, 0)] = .05f;
			[unroll]
            for (uint i = 2; i < 9; i++)
            {
				f_in[uint3(DTid.xy, i)] = .05f;
            }
		}
	)";

	constexpr char cs_visualization_src[] = R"(
		RWTexture2DArray<float> f_in : register(u0);
		RWTexture2DArray<int> snow : register(u1);
		RWTexture2D<float4> uav_display : register(u2);

		[numthreads(32, 32, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			const uint2 pos = DTid.xy;
			const float is_wall = f_in[uint3(pos,12)];
			const int snow_particle = snow[uint3(pos, 0)];
			const int snow_wall = snow[uint3(pos, 3)];
			const float air = length(float2( f_in[uint3(pos,10)], f_in[uint3(pos,11)])); //f_in[uint3(pos,9)];
			uav_display[pos] = float4(air, snow_wall, snow_particle, is_wall);
		}
	)";

	constexpr char cs_draw_src[] = R"(
		RWTexture2DArray<float> f_in : register(u0);
		RWTexture2DArray<int> snow : register(u1);

		cbuffer PerFrame : register(b0)
		{
			uint num_control_point;
		};

		struct ControlPoint
		{
			float2 pos;
			float dis;
			float data;
		};

		StructuredBuffer<ControlPoint> control_points : register(t0);

		[numthreads(32, 32, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			const uint2 pos = DTid.xy;
			[loop]
			for (uint i = 0; i < num_control_point; i++)
			{
				ControlPoint cp = control_points[i];
				[branch]
				if (distance(pos, cp.pos) < cp.dis)
				{
					[branch]
					if (cp.data == 2.0f)
					{
						f_in[uint3(pos, 12)] = 1.f;
					}
					else if (cp.data == 1.0f)
					{
						snow[uint3(pos, 0)] = 1;
					}
					else if (cp.data == 0.0f || cp.data == 3.0f)
					{
						f_in[uint3(pos, 12)] = 0.f;
						snow[uint3(pos, 0)] = 0;
						snow[uint3(pos, 3)] = 0;
					}
				}
			}
		}
	)";

	constexpr char cs_lbm1_src[] = R"(
		RWTexture2DArray<float> f_in : register(u0);
		RWTexture2DArray<int> snow : register(u1);

		// d2q9 velocity sets:
		//
		//    6    2     5
		//      ↖ ↑ ↗
		//    3 ←  0 → 1
		//      ↙ ↓ ↘
		//    7    4     8

		static const int2 c[9] = { { 0, 0 },
			 { 1, 0 }, { 0, -1 }, { -1, 0 }, { 0, 1 },
			{ 1, -1 }, { -1, -1 }, { -1, 1 }, { 1, 1 } };
		static const float w[9] =
		{
			4.f / 9.f,
			1.f / 9.f, 1.f / 9.f, 1.f / 9.f, 1.f / 9.f,
			1.f / 36.f, 1.f / 36.f, 1.f / 36.f, 1.f / 36.f
		};
		static const uint oppo[9] = {
			0, 3, 4, 1, 2, 7, 8, 5, 6
		};

		static const float tau = .8f;
		static const float k = 1.f/tau;

		[numthreads(32, 32, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			const int2 pos = DTid.xy;
			float f[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			float f_eq[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			uint i = 0;

			[unroll]
			for (i = 0; i < 9; i++)
			{
				f[i] = f_in[uint3(pos, i)];
			}

			float rho = f[0] + f[1] + f[2] + f[3] + f[4] + f[5] + f[6] + f[7] + f[8] +1e-20f;
			float2 u = { 0.f, 0.f };

			[unroll]
			for (i = 1; i < 9; i++)
			{
				u += f[i] * c[i];
			}
			u /= rho;
			u*=saturate(1.f-(snow[uint3(pos,0)]+snow[uint3(pos, 3)]*2.5f)*.03f);
			//...
			f_in[uint3(pos, 9)] = rho;
			f_in[uint3(pos, 10)] = u.x;
			f_in[uint3(pos, 11)] = u.y;

			if (length(u) > 0.57f)
			{
				u = normalize(u) * 0.57f;
			}

			float u_sqr = 1.5f * dot(u, u);

			[unroll]
			for (i = 0; i < 9; i++)
			{
				float cu = 3.f * dot(c[i], u);
				f_eq[i] = rho * w[i] * (1.f + cu + 0.5 * cu * cu - u_sqr);
			}

			float p = 0.f;
			[unroll(2)]
			for (uint a = 0; a < 2; a++)
			{
				[unroll(2)]
				for (uint b = 0; b < 2; b++)
				{
					[unroll]
					for (i = 0; i < 9; i++)
					{
						p += pow(c[i][a] * c[i][b] * (f[i] - f_eq[i]), 2);
					}
				}
			}

			float tau2 = tau + 0.5f * (-tau + sqrt(pow(tau, 2) + 1.62f * sqrt(p)));

			[unroll]
			for (i = 0; i < 9; i++)
			{
				//f[i] = (1.f - k) * f[i] + k * f_eq[i];
				f[i] = (1.f - 1.f / tau2) * f[i] + 1.f / tau2 * f_eq[i];
			}

			[unroll]
			for (i = 0; i < 9; i++)
			{
				f_in[uint3(pos, i+13)] = f[i];
			}
		}
	)";
	constexpr char cs_lbm2_src[] = R"(
		RWTexture2DArray<float> f_in : register(u0);
		RWTexture2DArray<int> snow : register(u1);
		static const int2 c[9] = { { 0, 0 },
			 { 1, 0 }, { 0, -1 }, { -1, 0 }, { 0, 1 },
			{ 1, -1 }, { -1, -1 }, { -1, 1 }, { 1, 1 } };
		static const float w[9] =
		{
			4.f / 9.f,
			1.f / 9.f, 1.f / 9.f, 1.f / 9.f, 1.f / 9.f,
			1.f / 36.f, 1.f / 36.f, 1.f / 36.f, 1.f / 36.f
		};
		static const uint oppo[9] = {
			0, 3, 4, 1, 2, 7, 8, 5, 6
		};
		[numthreads(1, 599, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			const uint2 pos ={
				DTid.x ? 799 : 0,
				DTid.y
			};
			if (pos.x)
			{
				[unroll]
				for (uint i = 0; i < 9; i++)
				{
					f_in[uint3(pos, i+13)] = f_in[uint3(pos + uint2(-1, 0), i+13)];
					//f_in[uint3(pos, i+13)] = f_in[uint3(pos + uint2(-2, 0), i+13)] * 2 - f_in[uint3(pos + uint2(-1, 0), i+13)];
					//f_in[uint3(pos, i+13)] = f_in[uint3(pos + uint2(-1, 0), i+13)] * 1.5f - f_in[uint3(pos + uint2(-2, 0), i+13)] * 0.5f;
				}
			}
			else
			{
				float f[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
				uint i = 0;
				float rho = 0;
				float2 u = { 0.0f, 0.0f };
				//
				[unroll]
				for (i = 0; i < 9; i++)
				{
					f[i] = f_in[uint3(pos, i+13)];
				}

				u = float2(0.04f, 0);
				rho = 1 / (1 - u.x) * (f[2] + f[0] + f[4] + (f[6] + f[3] + f[7]) * 2 + 1e-20f);

				const float usqr = 1.5 * dot(u, u);
				float eq[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

				[unroll]
				for (i = 0; i < 9; i++)
				{
					float cu = 3.f * dot(u, c[i]);
					eq[i] = rho * w[i] * (1.f + cu + 0.5f * cu * cu - usqr);
				}

				f[5] = eq[5] + f[oppo[5]] - eq[oppo[5]];
				f[1] = eq[1] + f[oppo[1]] - eq[oppo[1]];
				f[8] = eq[8] + f[oppo[8]] - eq[oppo[8]];

				[unroll]
				for (i = 0; i < 9; i++)
				{
					f_in[uint3(pos, i+13)] = f[i];
				}
			}
		}
	)";
	constexpr char cs_lbm3_src[] = R"(
		RWTexture2DArray<float> f_in : register(u0);
		RWTexture2DArray<int> snow : register(u1);
		static const int2 c[9] = { { 0, 0 },
			 { 1, 0 }, { 0, -1 }, { -1, 0 }, { 0, 1 },
			{ 1, -1 }, { -1, -1 }, { -1, 1 }, { 1, 1 } };
		static const uint oppo[9] = {
			0, 3, 4, 1, 2, 7, 8, 5, 6
		};

		bool is_wall(uint2 pos)
		{
			return pos.y > 599u || pos.x > 799u|| f_in[uint3(pos, 12)] > 0.f;
		}

		[numthreads(32, 32, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			const int2 pos = DTid.xy;
			bool flag1 = is_wall(pos);
			f_in[uint3(pos, 0)] = f_in[uint3(pos, 13)];
			for (uint i = 1; i < 9; i++)
			{
				uint2 pos_2 = pos + c[oppo[i]];
				bool flag2 = is_wall(pos_2);
				if (flag1)
				{
					f_in[uint3(pos, i)] = 0.0f;
				}
				else if (flag2)
				{
					f_in[uint3(pos, i)] = f_in[uint3(pos, oppo[i]+13)];
				}
				else
				{
					f_in[uint3(pos, i)] = f_in[uint3(pos_2, i+13)];
				}
			}
		}
	)";

	constexpr char cs_snow1_src[] = R"(
		RWTexture2DArray<float> f_in : register(u0);
		RWTexture2DArray<int> snow : register(u1);

		static const int2 v[5] = { { 0, 0 }, { 1, 0 }, { 0, -1 }, { -1, 0 }, { 0, 1 } };

		cbuffer PerFrame : register(b1)
		{
			float rand;
		};

		uint3 pcg3d(uint3 v)
		{
			v = v * 1664525u + 1013904223u;

			v.x += v.y * v.z;
			v.y += v.z * v.x;
			v.z += v.x * v.y;

			v ^= v >> 16u;

			v.x += v.y * v.z;
			v.y += v.z * v.x;
			v.z += v.x * v.y;

			return v;
		}

		float3 rand3(float3 seed)
		{
			return frac(float3(pcg3d(uint3((seed + float3(1.0f, 1.0f, 1.0f)) * 999999.f))) * (1.0 / float(0xffffffffu)));
		}

		bool is_wall(uint2 pos)
		{
			return pos.y > 599u || pos.x > 799u|| f_in[uint3(pos, 12)] > 0.f || snow[uint3(pos, 3)] > 0;
		}

		[numthreads(32, 32, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			const int2 pos = DTid.xy;
			const float2 u = float2(f_in[uint3(pos, 10)], f_in[uint3(pos, 11)] + .02f)*10.f;
			float2 rand2 = rand3(float3(rand, pos.x, pos.y)).xy;
			int f = snow[uint3(pos, 0)];
			if(pos.x==3u&& pos.y %4 == 1&&pos.y<500&&rand<0.05f)
			{
				f = 1;
			}
			if(pos.x<3u)
			{
				f = 0;
			}
			if(f > 0)
			{
				float px = dot(v[1], u);
				float pnx = dot(v[3], u);
				float py = dot(v[4], u);
				float pny = dot(v[2], u);
				int offx = step(rand2.x,max(px,pnx))*((rand2.x<px)?1:-1);
				int offy = step(rand2.y,max(py,pny))*((rand2.y<py)?1:-1);

				int2 off = { offx,offy };
				//int2 off = { (rand< 0.5f)?1:-1, (rand< 0.5f)?1:-1 };
				int2 pos2 = pos + off;
				if(is_wall(pos2))
				{
					return ;
				}
				InterlockedAdd(snow[uint3(pos2, 1)], 1);
				//rand2 = rand3(float3(rand2,rand)).xy;
				f--;
			}
			snow[uint3(pos, 0)]=f;
			//...
		}
	)";

	constexpr char cs_snow2_src[] = R"(
		RWTexture2DArray<float> f_in : register(u0);
		RWTexture2DArray<int> snow : register(u1);

		cbuffer PerFrame : register(b1)
		{
			float rand;
		};

		[numthreads(32, 32, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			const int2 pos = DTid.xy;
			int f = snow[uint3(pos, 0)];
			f += snow[uint3(pos, 1)];

			if(f > 5 && rand>0.5f)
			{
				snow[uint3(pos, 3)] = f;
				f = 0;
			}

			snow[uint3(pos, 0)] = f;
			snow[uint3(pos, 1)] = 0;
		}
	)";
	constexpr char cs_snow3_src[] = R"(
		RWTexture2DArray<float> f_in : register(u0);
		RWTexture2DArray<int> snow : register(u1);

		cbuffer PerFrame : register(b1)
		{
			float rand;
		};

		uint3 pcg3d(uint3 v)
		{
			v = v * 1664525u + 1013904223u;

			v.x += v.y * v.z;
			v.y += v.z * v.x;
			v.z += v.x * v.y;

			v ^= v >> 16u;

			v.x += v.y * v.z;
			v.y += v.z * v.x;
			v.z += v.x * v.y;

			return v;
		}

		float3 rand3(float x,float y,float z)
		{
			return frac(float3(pcg3d(uint3((float3(x,y,z) + float3(1.0f, 1.0f, 1.0f)) * 999999.f))) * (1.0 / float(0xffffffffu)));
		}

		static const int2 d[4] = {{0,0},{0,-1},{1,0},{1,-1}};

		bool is_wall(uint2 pos)
		{
			return pos.y > 598u || pos.x > 799u|| f_in[uint3(pos, 12)] > 0.f;
		}

		[numthreads(32, 32, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			const int2 pos = {DTid.x*2+step(rand,0.5f), DTid.y*2+step(rand,0.5f)};
			if(pos.y>599)
			{
				return;
			}
			if(is_wall(pos)||is_wall(pos+d[2])||is_wall(pos+d[1])||is_wall(pos+d[3]))
			{
				return;
			}

			int f[4] = {
				snow[uint3(pos, 3)],
				snow[uint3(pos+d[1], 3)],
				snow[uint3(pos+d[2], 3)],
				snow[uint3(pos+d[3], 3)],
			};

			float ux = f_in[uint3(pos, 10)];
			float uy = f_in[uint3(pos, 10)]+0.3f;

			float3 r = rand3(rand,pos.x,pos.y);
			if(r.x<abs(ux*20)&&(f[1]>f[3]&&ux>0)||(f[1]<f[3]&&ux<0))
			{
				int t= f[1];
				f[1] = f[3];
				f[3] = t;
			}
			if(r.y<uy*10)
			{
				if(f[0]<f[1])
				{
					int t= f[0];
					f[0] = f[1];
					f[1] = t;
				}
				if(f[2]<f[3])
				{
					int t= f[2];
					f[2] = f[3];
					f[3] = t;
				}
			}
			if(r.z<0.1f)
			{
				if(f[2]<f[1])
				{
					int t= f[2];
					f[2] = f[1];
					f[1] = t;
				}
				if(f[0]<f[3])
				{
					int t= f[0];
					f[0] = f[3];
					f[3] = t;
				}
			}
			snow[uint3(pos, 3)] = f[0];
			snow[uint3(pos+d[1], 3)]=f[1];
			snow[uint3(pos+d[2], 3)]=f[2];
			snow[uint3(pos+d[3], 3)]=f[3];
		}
	)";
	constexpr char cs_snow4_src[] = R"(
		RWTexture2DArray<float> f_in : register(u0);
		RWTexture2DArray<int> snow : register(u1);

		cbuffer PerFrame : register(b1)
		{
			float rand;
		};

		static const int2 d[4] = {{0,0},{0,-1},{1,0},{1,-1}};

		[numthreads(32, 32, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			const int2 pos = {DTid.x, DTid.y};
			const float2 u = float2(f_in[uint3(pos, 10)], f_in[uint3(pos, 11)]);
			int sw = snow[uint3(pos, 3)];
			if(pos.x<3u||pos.x>797)
			{
				snow[uint3(pos, 3)] = 0;
				snow[uint3(pos, 0)] = 0;
			}
			else if(sw>0&&rand<(pow(length(u),2)*50.f))
			{
				snow[uint3(pos, 3)] = 0;
				snow[uint3(pos, 0)] += sw;
			}
		}
	)";

	ComPtr<ID3DBlob> blob{};
	ComPtr<ID3DBlob> error{};
	HRESULT hr = NULL;
	hr = D3DCompile(vs_src, strlen(vs_src), "vs", nullptr, nullptr, "main", "vs_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	HR_ASSERT(hr, (char*)error->GetBufferPointer());
	hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, vs.GetAddressOf());
	HR_ASSERT(hr, "failed to create VertexShader");

	hr = device->CreateInputLayout(VsIn::input_layout, VsIn::num_elements, blob->GetBufferPointer(), blob->GetBufferSize(), input_layout.GetAddressOf());
	HR_ASSERT(hr, "failed to create InputLayout");

	//创建PS shader
	hr = D3DCompile(ps_src, strlen(ps_src), "ps", nullptr, nullptr, "main", "ps_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	HR_ASSERT(hr, (char*)error->GetBufferPointer());

	hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, ps.GetAddressOf());
	HR_ASSERT(hr, "failed to create PixelShader");

	//
	//创建CS draw
	hr = D3DCompile(cs_draw_src, strlen(cs_draw_src), "cs_draw", nullptr, nullptr, "main", "cs_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	HR_ASSERT(hr, (char*)error->GetBufferPointer());

	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_draw.GetAddressOf());
	HR_ASSERT(hr, "failed to create cs_draw");

	//创建CS visualization
	hr = D3DCompile(cs_visualization_src, strlen(cs_visualization_src), "cs_visualization", nullptr, nullptr, "main", "cs_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	HR_ASSERT(hr, (char*)error->GetBufferPointer());

	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_visualization.GetAddressOf());
	HR_ASSERT(hr, "failed to create cs_visualization");

	//创建CS init
	hr = D3DCompile(cs_init_src, strlen(cs_init_src), "cs_init", nullptr, nullptr, "main", "cs_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	HR_ASSERT(hr, (char*)error->GetBufferPointer());

	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_init.GetAddressOf());
	HR_ASSERT(hr, "failed to create cs_init");

	//创建CS lbm1
	hr = D3DCompile(cs_lbm1_src, strlen(cs_lbm1_src), "cs_lbm1", nullptr, nullptr, "main", "cs_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	HR_ASSERT(hr, (char*)error->GetBufferPointer());

	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_lbm1.GetAddressOf());
	HR_ASSERT(hr, "failed to create cs_lbm1");
	//创建CS lbm2
	hr = D3DCompile(cs_lbm2_src, strlen(cs_lbm2_src), "cs_lbm2", nullptr, nullptr, "main", "cs_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	HR_ASSERT(hr, (char*)error->GetBufferPointer());

	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_lbm2.GetAddressOf());
	HR_ASSERT(hr, "failed to create cs_lbm2");

	//创建CS lbm3
	hr = D3DCompile(cs_lbm3_src, strlen(cs_lbm3_src), "cs_lbm3", nullptr, nullptr, "main", "cs_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	HR_ASSERT(hr, (char*)error->GetBufferPointer());

	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_lbm3.GetAddressOf());
	HR_ASSERT(hr, "failed to create cs_lbm3");

	//创建CS snow1
	hr = D3DCompile(cs_snow1_src, strlen(cs_snow1_src), "cs_snow1", nullptr, nullptr, "main", "cs_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	HR_ASSERT(hr, (char*)error->GetBufferPointer());

	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_snow1.GetAddressOf());
	HR_ASSERT(hr, "failed to create cs_snow1");

	//创建CS snow2
	hr = D3DCompile(cs_snow2_src, strlen(cs_snow2_src), "cs_snow2", nullptr, nullptr, "main", "cs_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	HR_ASSERT(hr, (char*)error->GetBufferPointer());

	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_snow2.GetAddressOf());
	HR_ASSERT(hr, "failed to create cs_snow2");

	//创建CS snow3
	hr = D3DCompile(cs_snow3_src, strlen(cs_snow3_src), "cs_snow3", nullptr, nullptr, "main", "cs_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	HR_ASSERT(hr, (char*)error->GetBufferPointer());

	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_snow3.GetAddressOf());
	HR_ASSERT(hr, "failed to create cs_snow3");

	//创建CS snow4
	hr = D3DCompile(cs_snow4_src, strlen(cs_snow4_src), "cs_snow4", nullptr, nullptr, "main", "cs_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	HR_ASSERT(hr, (char*)error->GetBufferPointer());

	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_snow4.GetAddressOf());
	HR_ASSERT(hr, "failed to create cs_snow4");
}

void Blizzard::initResources()
{
	decltype(auto) device = dx11_wnd->GetDevice();
	HRESULT hr = NULL;
	D3D11_TEXTURE2D_DESC tex_desc = {};
	D3D11_BUFFER_DESC buf_desc = {};
	D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};

	tex_desc.Width = dx11_wnd->getWidth();
	tex_desc.Height = dx11_wnd->getHeight();
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	tex_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MipLevels = 1;
	tex_desc.SampleDesc.Count = 1;

	uav_desc.Format = tex_desc.Format;
	uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

	srv_desc.Format = tex_desc.Format;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;

	//tex_display,tex1
	hr = device->CreateTexture2D(&tex_desc, 0, tex_display.GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateUnorderedAccessView(tex_display.Get(), &uav_desc, uav_tex_display.GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateShaderResourceView(tex_display.Get(), &srv_desc, srv_tex_display.GetAddressOf());
	assert(SUCCEEDED(hr));

	//snow
	tex_desc = {};
	tex_desc.Width = dx11_wnd->getWidth();
	tex_desc.Height = dx11_wnd->getHeight();
	tex_desc.ArraySize = num_wind_channels;
	tex_desc.Format = DXGI_FORMAT_R32_FLOAT;
	tex_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MipLevels = 1;
	tex_desc.SampleDesc.Count = 1;
	uav_desc = {};
	uav_desc.Format = tex_desc.Format;
	uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	uav_desc.Texture2DArray.ArraySize = num_wind_channels;
	hr = device->CreateTexture2D(&tex_desc, 0, tex_array_wind.GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateUnorderedAccessView(tex_array_wind.Get(), &uav_desc, uav_tex_array_wind.GetAddressOf());
	assert(SUCCEEDED(hr));

	//wind
	tex_desc = {};
	tex_desc.Width = dx11_wnd->getWidth();
	tex_desc.Height = dx11_wnd->getHeight();
	tex_desc.ArraySize = num_snow_channels;
	tex_desc.Format = DXGI_FORMAT_R32_SINT;
	tex_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MipLevels = 1;
	tex_desc.SampleDesc.Count = 1;
	uav_desc = {};
	uav_desc.Format = tex_desc.Format;
	uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	uav_desc.Texture2DArray.ArraySize = num_snow_channels;
	hr = device->CreateTexture2D(&tex_desc, 0, tex_array_snow.GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateUnorderedAccessView(tex_array_snow.Get(), &uav_desc, uav_tex_array_snow.GetAddressOf());
	assert(SUCCEEDED(hr));

	//创建控制点buffur
	buf_desc = {};
	buf_desc.ByteWidth = sizeof(ControlPoint) * max_num_control_points;
	buf_desc.Usage = D3D11_USAGE_DYNAMIC;
	buf_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	buf_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buf_desc.StructureByteStride = sizeof(ControlPoint);
	buf_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	//
	srv_desc = {};
	srv_desc.Format = DXGI_FORMAT_UNKNOWN;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srv_desc.Buffer.FirstElement = 0;
	srv_desc.Buffer.NumElements = max_num_control_points;

	hr = device->CreateBuffer(&buf_desc, 0, buf_control_points.GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateShaderResourceView(buf_control_points.Get(), &srv_desc, srv_control_points.GetAddressOf());
	assert(SUCCEEDED(hr));

	//创建cbuf_num_control_points
	buf_desc = {};
	buf_desc.Usage = D3D11_USAGE_DYNAMIC;
	buf_desc.ByteWidth = 16;
	buf_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buf_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hr = device->CreateBuffer(&buf_desc, nullptr, cbuf_num_control_points.GetAddressOf());
	assert(SUCCEEDED(hr));

	//创建cbuf_rand
	buf_desc = {};
	buf_desc.Usage = D3D11_USAGE_DYNAMIC;
	buf_desc.ByteWidth = 16;
	buf_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buf_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hr = device->CreateBuffer(&buf_desc, nullptr, cbuf_rand.GetAddressOf());
	assert(SUCCEEDED(hr));
}

void Blizzard::bindResources()
{
	auto ctx = dx11_wnd->GetImCtx();
	auto device = dx11_wnd->GetDevice();

	//创建 vertices_buffer
	HRESULT hr = NULL;
	D3D11_BUFFER_DESC vbd{};
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(vertices);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = vertices;

	hr = device->CreateBuffer(&vbd, &sd, vertex_buffer.GetAddressOf());
	assert(SUCCEEDED(hr));

	//设置 input_layout
	UINT stride = sizeof(VsIn);
	UINT offset = 0;
	ctx->VSSetShader(vs.Get(), 0, 0);
	ctx->PSSetShader(ps.Get(), 0, 0);
	ctx->IASetInputLayout(input_layout.Get());
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	ctx->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);

	//
	ctx->CSSetUnorderedAccessViews(0, 1, uav_tex_array_wind.GetAddressOf(), 0);
	ctx->CSSetUnorderedAccessViews(1, 1, uav_tex_array_snow.GetAddressOf(), 0);
	ctx->CSSetUnorderedAccessViews(2, 1, uav_tex_display.GetAddressOf(), 0);
	ctx->CSSetShaderResources(0, 1, srv_control_points.GetAddressOf());
	ctx->CSSetConstantBuffers(0, 1, cbuf_num_control_points.GetAddressOf());
	ctx->CSSetConstantBuffers(1, 1, cbuf_rand.GetAddressOf());
}

void Blizzard::updateControlPointBuffer()
{
	decltype(auto) ctx = dx11_wnd->GetImCtx();
	HRESULT hr = NULL;
	D3D11_MAPPED_SUBRESOURCE mapped_subresource = {};
	if (control_points.size() > 0)
	{
		hr = ctx->Map(buf_control_points.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
		assert(SUCCEEDED(hr));
		ControlPoint* p_data = (ControlPoint*)mapped_subresource.pData;
		const size_t n_data = control_points.size();
		memcpy_s(p_data, max_num_control_points * sizeof ControlPoint,
			&control_points[0], n_data * sizeof ControlPoint);
		ctx->Unmap(buf_control_points.Get(), 0);
	}
	//
	mapped_subresource = {};
	hr = ctx->Map(cbuf_num_control_points.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
	assert(SUCCEEDED(hr));
	int* p_num_control_points = (int*)mapped_subresource.pData;
	*p_num_control_points = (int)control_points.size();
	ctx->Unmap(cbuf_num_control_points.Get(), 0);
}

void Blizzard::updateRandSeed()
{
	decltype(auto) ctx = dx11_wnd->GetImCtx();
	HRESULT hr = NULL;
	static unsigned int seed = (unsigned int)time(NULL);
	srand(seed);
	D3D11_MAPPED_SUBRESOURCE mapped_subresource = {};
	hr = ctx->Map(cbuf_rand.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
	assert(SUCCEEDED(hr));
	float* rand_seed = (float*)mapped_subresource.pData;
	seed = rand();
	*rand_seed = seed / (float)RAND_MAX;
	ctx->Unmap(cbuf_rand.Get(), 0);
}

void Blizzard::setInputCallback()
{
	const auto onmousemove = [&](WPARAM wparam, LPARAM lparam) {
		const POINTS p = MAKEPOINTS(lparam);
		const XMFLOAT2 pos = { (float)p.x ,(float)p.y };
		if (wparam & MK_SHIFT) {
			addControlPoint(pos, { 15.f, 0.f });
		}
		else if (wparam & MK_LBUTTON) {
			addControlPoint(pos, { 50.f,1.f });
		}
		else if (wparam & MK_RBUTTON) {
			addControlPoint(pos, { 10.f,2.f });
		}
		else if (wparam & MK_MBUTTON) {
			addControlPoint(pos, { 10.f, 3.f });
		}

		TRACKMOUSEEVENT track_mouse_event{};
		track_mouse_event.cbSize = sizeof(TRACKMOUSEEVENT);
		track_mouse_event.dwFlags = TME_LEAVE;
		track_mouse_event.dwHoverTime = HOVER_DEFAULT;
		track_mouse_event.hwndTrack = dx11_wnd->Hwnd();
		TrackMouseEvent(&track_mouse_event);
		return true;
	};

	const auto onmousebtnupormouseleave = [&](WPARAM wparam, LPARAM lparam) {
		last_control_point = { {-1.f,-1.f},{-1.f,-1.f} };
		return true;
	};

	dx11_wnd->AddWndProc(WM_MOUSEMOVE, onmousemove);
	dx11_wnd->AddWndProc(WM_LBUTTONDOWN, onmousemove);
	dx11_wnd->AddWndProc(WM_RBUTTONDOWN, onmousemove);
	dx11_wnd->AddWndProc(WM_MBUTTONDOWN, onmousemove);

	dx11_wnd->AddWndProc(WM_MOUSELEAVE, onmousebtnupormouseleave);
	dx11_wnd->AddWndProc(WM_LBUTTONUP, onmousebtnupormouseleave);
	dx11_wnd->AddWndProc(WM_RBUTTONUP, onmousebtnupormouseleave);
	dx11_wnd->AddWndProc(WM_MBUTTONUP, onmousebtnupormouseleave);
}

void Blizzard::addControlPoint(XMFLOAT2 pos, XMFLOAT2 data)
{
	if (last_control_point.pos.x >= 0 && data.y == last_control_point.data.y)
	{
		XMVECTOR pos0 = XMLoadFloat2(&last_control_point.pos);
		XMVECTOR pos1 = XMLoadFloat2(&pos);
		XMVECTOR direction = pos1 - pos0;
		XMVECTOR len = XMVector2Length(direction);
		direction = direction / len;
		const float r = 1;//data.x;
		const float length = XMVectorGetX(len);
		for (float d = r; d < length; d += r)
		{
			XMVECTOR pos2 = pos0 + XMVectorSet(d, d, 0, 0) * direction;
			XMFLOAT2 pos_float2 = {};
			XMStoreFloat2(&pos_float2, pos2);
			control_points.push_back({ pos_float2 , data });
		}
	}

	control_points.push_back({ pos , data });
	last_control_point = { pos , data };
}
