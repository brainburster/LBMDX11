#pragma once

#include "dx11_wnd.hpp"
#include <vector>
#include <memory>

class Blizzard final
{
public:
	Blizzard(std::shared_ptr<DX11_Wnd> wnd) : dx11_wnd{ wnd }, last_control_point{ {-1.f,-1.f},{-1.f,-1.f} }{}
	void run();
	auto getWnd() { return dx11_wnd; }
private:
	void init();
	void update();
	void render();
	void initShaders();
	void initResources();
	void bindResources();
	void setInputCallback();

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
	ComPtr<ID3D11Buffer> vertex_buffer;
	ComPtr<ID3D11InputLayout> input_layout;
	ComPtr<ID3D11VertexShader> vs;
	ComPtr<ID3D11PixelShader> ps;
	//...
	ComPtr<ID3D11Texture2D> tex_array_f_in;
	ComPtr<ID3D11Texture2D> tex_array_f_out;
	ComPtr<ID3D11UnorderedAccessView> uav_tex_array_f_in;
	ComPtr<ID3D11UnorderedAccessView> uav_tex_array_f_out;
	ComPtr<ID3D11Texture2D> tex_display;
	ComPtr<ID3D11UnorderedAccessView> uav_tex_display;
	ComPtr<ID3D11ShaderResourceView> srv_tex_display;
	//...
	ComPtr<ID3D11ComputeShader> cs_init;
	ComPtr<ID3D11ComputeShader> cs_draw;
	ComPtr<ID3D11ComputeShader> cs_lbm_moment_update;
	ComPtr<ID3D11ComputeShader> cs_lbm_force_calculation;
	ComPtr<ID3D11ComputeShader> cs_lbm_collision;
	ComPtr<ID3D11ComputeShader> cs_lbm_streaming;
	ComPtr<ID3D11ComputeShader> cs_lbm_visualization;

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
};
