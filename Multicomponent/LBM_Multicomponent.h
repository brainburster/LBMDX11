#pragma once

#include "dx11_wnd.hpp"
#include <memory>
#include <vector>
#include <d3dcompiler.h>

class LBM_Multicomponent final
{
private:
	std::shared_ptr<DX11_Wnd> dx11_wnd;
	struct VsIn
	{
		XMFLOAT2 pos;
		XMFLOAT2 uv;
		static constexpr UINT num_elements = 2;
		static constexpr D3D11_INPUT_ELEMENT_DESC  input_layout[num_elements] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,           D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(pos), D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
	};
	static constexpr VsIn vertices[4] = {
		{ XMFLOAT2{-1.f, 1.f}, XMFLOAT2{0.f,0.f} },
		{ XMFLOAT2{1.f, 1.f }, XMFLOAT2{1.f,0.f} },
		{ XMFLOAT2{-1.f,-1.f}, XMFLOAT2{0.f,1.f} },
		{ XMFLOAT2{1.f, -1.f}, XMFLOAT2{1.f,1.f} },
	};
	//着色器
	ComPtr<ID3D11Buffer> vertex_buffer;
	ComPtr<ID3D11InputLayout> input_layout;
	ComPtr<ID3D11VertexShader> vs;
	ComPtr<ID3D11PixelShader> ps;
	//根据控制点生成流体与墙
	ComPtr<ID3D11ComputeShader> cs_draw;
	//初始化
	//ComPtr<ID3D11ComputeShader> cs_init;
	//lbm-shan-chen模型
	ComPtr<ID3D11ComputeShader> cs_lbm_moment_update;
	ComPtr<ID3D11ComputeShader> cs_lbm_force_calculation;
	ComPtr<ID3D11ComputeShader> cs_lbm_collision;
	ComPtr<ID3D11ComputeShader> cs_lbm_streaming;
	ComPtr<ID3D11ComputeShader> cs_lbm_visualization;
	ComPtr<ID3D11ComputeShader> cs_lbm;
	//储存2个组分的分布以及其他物理量如rho,u,psi(有效密度),F_k,
	ComPtr<ID3D11Texture2D> tex_array_f_in[2];
	ComPtr<ID3D11Texture2D> tex_array_f_out[2];
	ComPtr<ID3D11UnorderedAccessView> uav_tex_array_f_in[2];
	ComPtr<ID3D11UnorderedAccessView> uav_tex_array_f_out[2];
	//4个通道分别表示不同组分的密度，第4个通道<0表示墙
	ComPtr<ID3D11Texture2D> tex_display;
	ComPtr<ID3D11UnorderedAccessView> uav_tex_display;
	ComPtr<ID3D11ShaderResourceView> srv_tex_display;
	//
	//控制点
	//用于生成流体以及墙
	struct ControlPoint
	{
		XMFLOAT2 pos;
		XMFLOAT2 data;
	};
	std::vector<ControlPoint> control_points;
	ComPtr<ID3D11Buffer> buf_control_points;
	ComPtr<ID3D11ShaderResourceView> srv_control_points;
	ComPtr<ID3D11Buffer> cbuf_num_control_points;
	ControlPoint last_control_point;

	enum
	{
		num_f_channels = 12,
		max_num_control_points = 128,
	};
public:
	LBM_Multicomponent(decltype(dx11_wnd) wnd) : dx11_wnd{ wnd }, last_control_point{ {-1.f,-1.f},{-1.f,-1.f} }{}
	//
	void run()
	{
		init();
		while (!dx11_wnd->app_should_close())
		{
			dx11_wnd->PeekMsg();
			handleInput();
			update();
			render();
		}
	}

private:
	void init()
	{
		init_shaders();
		init_resources();
		bind_resources();
		set_input_callback();
	}

	void fence()
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

	void update_control_point_buffer()
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

	void update()
	{
		decltype(auto) device = dx11_wnd->GetDevice();
		decltype(auto) ctx = dx11_wnd->GetImCtx();
		const int width = dx11_wnd->getWidth() / 8;
		const int height = dx11_wnd->getHeight() / 8;
		const UINT ThreadGroupCountX = (width - 1) / 32 + 1;
		const UINT ThreadGroupCountY = (height - 1) / 32 + 1;
		//应用控制点
		if (control_points.size() > 0)
		{
			fence();
			update_control_point_buffer();
			control_points.clear();
			ctx->CSSetShader(cs_draw.Get(), NULL, 0);
			ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
			fence();
		}

		fence();
		ctx->CSSetShader(cs_lbm_moment_update.Get(), NULL, 0);
		ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
		fence();
		ctx->CSSetShader(cs_lbm_force_calculation.Get(), NULL, 0);
		ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
		fence();
		ctx->CSSetShader(cs_lbm_collision.Get(), NULL, 0);
		ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
		fence();
		ctx->CSSetShader(cs_lbm_streaming.Get(), NULL, 0);
		ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
		fence();
		//显示
		ctx->CSSetShader(cs_lbm_visualization.Get(), NULL, 0);
		ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
		fence();
	}

	void render()
	{
		decltype(auto) ctx = dx11_wnd->GetImCtx();
		constexpr float back_color[4] = { 0.f,0.f,0.f,1.f };
		ctx->ClearRenderTargetView(dx11_wnd->GetRTV(), back_color);
		ctx->ClearDepthStencilView(dx11_wnd->GetDsv(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		//解决同一个资源同时绑定srv和uav的冲突
		ID3D11ShaderResourceView* null_srv = nullptr;
		ID3D11UnorderedAccessView* null_uav = nullptr;
		ctx->CSSetUnorderedAccessViews(4, 1, &null_uav, 0);
		ctx->PSSetShaderResources(0, 1, srv_tex_display.GetAddressOf());
		//绘制
		ctx->Draw(4, 0);
		ctx->CSSetUnorderedAccessViews(4, 1, uav_tex_display.GetAddressOf(), 0);
		ctx->PSSetShaderResources(0, 1, &null_srv);
		dx11_wnd->GetSwapChain()->Present(0, 0);
	}
	void handleInput() {}
	void init_shaders();
	void init_resources();
	void bind_resources();
	void add_control_point(XMFLOAT2 pos, XMFLOAT2 data);
	void set_input_callback();
};
