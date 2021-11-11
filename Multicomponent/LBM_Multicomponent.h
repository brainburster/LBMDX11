#pragma once

#include "dx11_wnd.hpp"
#include <memory>
#include <vector>

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
	//储存2个组分的分布以及其他物理量如rho,u,psi(有效密度),F_k,
	ComPtr<ID3D11Texture2D> tex_array_f_in[2];
	ComPtr<ID3D11Texture2D> tex_array_f_out[2];
	ComPtr<ID3D11UnorderedAccessView> uav_tex_array_f_in[2];
	ComPtr<ID3D11UnorderedAccessView> uav_tex_array_f_out[2];
	//4个通道分别表示不同组分的密度，第4个通道<0表示墙
	ComPtr<ID3D11Texture2D> tex_display;
	ComPtr<ID3D11UnorderedAccessView> uav_tex_display;
	ComPtr<ID3D11ShaderResourceView> srv_tex_display;

	//控制点 用于生成流体以及墙
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
	void run();
private:
	void init();
	void fence();
	void update_control_point_buffer();
	void update();
	void render();
	void handleInput() {}
	void init_shaders();
	void init_resources();
	void bind_resources();
	void add_control_point(XMFLOAT2 pos, XMFLOAT2 data);
	void set_input_callback();
};
