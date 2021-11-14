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
		Texture2D<float4> srv_tex0 : register(t0);
		Texture2D<float4> srv_tex1 : register(t1);

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
			float4 data0 = srv_tex0.Sample(sampler0, vs_out.uv);
			float4 data1 = srv_tex1.Sample(sampler0, vs_out.uv);

			float4 wall_color = float4(0.25f, 0.25f, 0.25f, 1.0f)* abs(data.w);
			float4 color = float4(data1,data1,data1, 1.0f);

			if (data.w < 0.0f)
			{
				return wall_color * abs(data.w);
			}
			return float4(vs_out.uv,0.f,0.f);
		}
	)";

	constexpr char cs_init_src[] = R"(
		RWTexture2DArray<float> f_in : register(u0);

		[numthreads(16, 16, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			[unroll]
			for (uint i = 0; i < 9; i++)
			{
				f_in[uint3(DTid.xy, i)] = 0.1f;
			}
		}
	)";

	constexpr char cs_draw_src[] = R"(
		RWTexture2D<float4> tex0 : register(u1);
		RWTexture2D<float4> tex1 : register(u2);
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

		[numthreads(16, 16, 1)]
		void main(uint3 DTid : SV_DispatchThreadID)
		{
			const uint2 index = DTid.xy;
			[loop]
			for (int i = 0; i < num_control_point; i++)
			{
				ControlPoint cp = control_points[i];
				[branch]
				if (distance(index, cp.pos) < cp.dis)
				{
					[branch]
					if (cp.data == 2.0)
					{
						tex0[index] = float4(0,0,0,1);
					}
					else if (cp.data == 1.0)
					{
						tex1[index] = float4(1,0,0,0);
					}
					else if (cp.data == 0.0)
					{
						tex0[index] = float4(0,0,0,0);
						tex1[index] = float4(0,0,0,0);
					}
				}
			}
		}
	)";

	constexpr char cs_lbm_src[] = R"(
		RWTexture2DArray<float> f_in : register(u0);
		RWTexture2D<float4> tex0 : register(u1);

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
	HR_ASSERT(hr, (char*)error->GetBufferPointer());
	hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, vs.GetAddressOf());
	HR_ASSERT(hr, "failed to create VertexShader");

	hr = device->CreateInputLayout(VsIn::input_layout, VsIn::num_elements, blob->GetBufferPointer(), blob->GetBufferSize(), input_layout.GetAddressOf());
	HR_ASSERT(hr, "failed to create InputLayout");

	//创建PS shader
	hr = D3DCompile(ps_src, sizeof(ps_src), "ps", nullptr, nullptr, "main", "ps_5_0", 0, 0, blob.ReleaseAndGetAddressOf(), error.ReleaseAndGetAddressOf());
	HR_ASSERT(hr, (char*)error->GetBufferPointer());

	hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, ps.GetAddressOf());
	HR_ASSERT(hr, "failed to create PixelShader");

	//
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

	//tex0,tex1
	hr = device->CreateTexture2D(&tex_desc, 0, tex0.GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateUnorderedAccessView(tex0.Get(), &uav_desc, uav_tex0.GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateShaderResourceView(tex0.Get(), &srv_desc, srv_tex0.GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateTexture2D(&tex_desc, 0, tex1.GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateUnorderedAccessView(tex0.Get(), &uav_desc, uav_tex1.GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateShaderResourceView(tex0.Get(), &srv_desc, srv_tex1.GetAddressOf());
	assert(SUCCEEDED(hr));

	//f_in
	tex_desc = {};
	tex_desc.Width = dx11_wnd->getWidth() / 8;
	tex_desc.Height = dx11_wnd->getHeight() / 8;
	tex_desc.ArraySize = num_f_channels;
	tex_desc.Format = DXGI_FORMAT_R32_FLOAT;
	tex_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MipLevels = 1;
	tex_desc.SampleDesc.Count = 1;
	uav_desc = {};
	uav_desc.Format = tex_desc.Format;
	uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	uav_desc.Texture2DArray.ArraySize = num_f_channels;
	hr = device->CreateTexture2D(&tex_desc, 0, tex_array_f_in.GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateUnorderedAccessView(tex_array_f_in.Get(), &uav_desc, uav_tex_array_f_in.GetAddressOf());
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
}

void Blizzard::updateControlPointBuffer()
{
	decltype(auto) ctx = dx11_wnd->GetImCtx();
	HRESULT hr = NULL;
	D3D11_MAPPED_SUBRESOURCE mapped_subresource = {};
	hr = ctx->Map(buf_control_points.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
	assert(SUCCEEDED(hr));
	ControlPoint* p_data = (ControlPoint*)mapped_subresource.pData;
	const size_t n_data = control_points.size();
	memcpy_s(p_data, max_num_control_points * sizeof ControlPoint,
		&control_points[0], n_data * sizeof ControlPoint);
	ctx->Unmap(buf_control_points.Get(), 0);
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
	static unsigned int seed = 0;
	srand(seed++);
	D3D11_MAPPED_SUBRESOURCE mapped_subresource = {};
	hr = ctx->Map(cbuf_num_control_points.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
	assert(SUCCEEDED(hr));
	float* p_num_control_points = (float*)mapped_subresource.pData;
	*p_num_control_points = (float)rand() / RAND_MAX;
	ctx->Unmap(cbuf_num_control_points.Get(), 0);
}

void Blizzard::setInputCallback()
{
	const auto onmousemove = [&](WPARAM wparam, LPARAM lparam) {
		const POINTS p = MAKEPOINTS(lparam);
		const XMFLOAT2 pos = { (float)p.x / 8,(float)p.y / 8 };
		if (wparam & MK_SHIFT) {
			addControlPoint(pos, { 2.f, 0.f });
		}
		else if (wparam & MK_LBUTTON) {
			addControlPoint(pos, { 5.f,1.f });
		}
		else if (wparam & MK_RBUTTON) {
			addControlPoint(pos, { 5.f,2.f });
		}
		else if (wparam & MK_MBUTTON) {
			addControlPoint(pos, { 1.2f,3.f });
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
