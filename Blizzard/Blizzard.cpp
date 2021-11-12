#include "logger.hpp"
#include "Blizzard.h"
#include <d3dcompiler.h>

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
}

void Blizzard::update()
{
	auto device = dx11_wnd->GetDevice();
	auto ctx = dx11_wnd->GetImCtx();
}

void Blizzard::render()
{
	auto ctx = dx11_wnd->GetImCtx();
	constexpr float back_color[4] = { 0.f,0.f,0.f,1.f };
	ctx->ClearRenderTargetView(dx11_wnd->GetRTV(), back_color);
	ctx->ClearDepthStencilView(dx11_wnd->GetDsv(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	ctx->Draw(4, 0);
	dx11_wnd->GetSwapChain()->Present(0, 0);
}

void Blizzard::initShaders()
{
	auto device = dx11_wnd->GetDevice();
#define STR(...) #__VA_ARGS__
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

		VsOut main( VsIn vs_in )
		{
			VsOut vs_out;
			vs_out.posH = float4(vs_in.pos,0.f, 1.f);
			vs_out.uv = vs_in.uv;
			return vs_out;
		}
	)";

	constexpr char ps_src[] = R"(
		struct VsOut
		{
			float4 posH : SV_POSITION;
			float2 uv : TEXCOORD;
		};

		float4 main(VsOut vs_out) : SV_TARGET
		{
			return float4(vs_out.uv,0.f,0.f);
		}
	)";

	constexpr char cs_display_src[] = R"(
		RWTexture2D<float4> tex0 : register(u2);
		RWTexture2D<float4> tex1 : register(u1);

		[numthreads(16, 16, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			//...
		}
	)";

	constexpr char cs_lbm_src[] = R"(
		RWTexture2DArray<float> f_in : register(u0);
		RWTexture2D<float4> tex0 : register(u2);

		// d2q9 velocity sets:
		//
		//    6    2     5
		//      ↖ ↑ ↗
		//    3 ←  0 → 1
		//      ↙ ↓ ↘
		//    7    4     8

		static const int2 c[9] = { { 0, 0 }, { 1, 0 }, { 0, -1 }, { -1, 0 }, { 0, 1 }, { 1, -1 }, { -1, -1 }, { -1, 1 }, { 1, 1 } };
		static const float w[9] =
		{
			4.f / 9.f,
			1.f / 9.f, 1.f / 9.f, 1.f / 9.f, 1.f / 9.f,
			1.f / 36.f, 1.f / 36.f, 1.f / 36.f, 1.f / 36.f
		};
		static const uint oppo[9] = {
			0, 3, 4, 1, 2, 7, 8, 5, 6
		};
		static const float k = 1.2f;

		bool is_wall(uint2 pos)
		{
			if (pos.x < 0 || pos.y > 599 || pos.y < 0 || pos.x > 799)
			{
				return true;
			}
			return tex0[pos].w < 0.0;
		}

		[numthreads(16, 16, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			const uint2 pos = DTid.xy;
			float f[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			uint i = 0;
			//float f1[9] = { };
			//传输/反弹
			bool flag1 = is_wall(pos);
			f[0] = f_in[0][uint3(pos, 0)];
			for (i = 1; i < 9; i++)
			{
				uint2 pos_2 = pos + c[oppo[i]];
				bool flag2 = is_wall(pos_2);
				if (flag1)
				{
					f[i] = 0.0;
				}
				else if (flag2)
				{
					f[i] = f_in[0][uint3(pos, oppo[i])];
					f[i] += f_in[0][uint3(pos_2, i)];
				}
				else
				{
					f[i] = f_in[0][uint3(pos_2, i)];
				}
			}

			//计算受力
			//...

			//预计算
			float rho = f[0] + f[1] + f[2] + f[3] + f[4] + f[5] + f[6] + f[7] + f[8];
			float2 u = { 0.f, 0.f };

			[unroll]
			for (i = 1; i < 9; i++)
			{
				u += f[0] * c[0];
			}
			u /= rho;
			u.y += 0.005;

			//碰撞
			float u_sqr = 1.5 * (u.x * u.x + u.y * u.y);
			[unroll]
			for (i = 0; i < 9; i++)
			{
				float cu = 3.0 * (c[i].x * u.x + c[i].y * u.y);
				float f_eq = rho * w[i] * (1.0 + cu + 0.5 * cu * cu - u_sqr);
				f[i] = (1.0 - k) * f[i] + k * f_eq;
			}

			[unroll]
			for (i = 0; i < 9; i++)
			{
				f_in[0][uint3(pos, i)] = f[i];
			}

			//显示
			tex0[pos] = float4(rho0, u.x, u.y, tex0[pos].w);
		}
	)";

	ComPtr<ID3DBlob> blob{};
	ComPtr<ID3DBlob> error{};
	HRESULT hr = NULL;
	hr = D3DCompile(vs_src, sizeof(vs_src), "vs", nullptr, nullptr, "main", "vs_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		Logger::error((char*)error->GetBufferPointer());
	}
	hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, vs.GetAddressOf());
	if (FAILED(hr))
	{
		Logger::error("failed to create VertexShader");
	}

	hr = device->CreateInputLayout(VsIn::input_layout, VsIn::num_elements, blob->GetBufferPointer(), blob->GetBufferSize(), input_layout.GetAddressOf());
	if (FAILED(hr))
	{
		Logger::error("failed to create InputLayout");
	}
	//创建PS shader
	hr = D3DCompile(ps_src, sizeof(ps_src), "ps", nullptr, nullptr, "main", "ps_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		Logger::error((char*)error->GetBufferPointer());
	}
	hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, ps.GetAddressOf());
	if (FAILED(hr))
	{
		Logger::error("failed to create PixelShader");
	}

	//
}

void Blizzard::initResources()
{
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
}

void Blizzard::setInputCallback()
{
}
