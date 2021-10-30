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
	ComPtr<ID3D11ComputeShader> cs_init;
	//更新物理量
	ComPtr<ID3D11ComputeShader> cs_update_quantities;
	//lbm-shan-chen模型
	//collision
	ComPtr<ID3D11ComputeShader> cs_lbm_collision;
	//streaming
	ComPtr<ID3D11ComputeShader> cs_lbm_streaming;
	//储存3个组分的分布以及其他物理量如rho,u,psi(有效密度),F_k,
	ComPtr<ID3D11Texture2D> tex_array_f_in[3];
	ComPtr<ID3D11Texture2D> tex_array_f_out[3];
	ComPtr<ID3D11UnorderedAccessView> uav_tex_array_f_in[3];
	ComPtr<ID3D11UnorderedAccessView> uav_tex_array_f_out[3];
	//混合流体的物理量,4个通道，rho,u,omega, 第四个通道<0表示墙，墙与普通格点之间的边将被视为反弹边界，这个边界将会被动态计算出来
	ComPtr<ID3D11Texture2D> tex_quantities;
	ComPtr<ID3D11UnorderedAccessView> uav_tex_quantites;
	ComPtr<ID3D11ShaderResourceView> srv_tex_quantites;
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

	enum
	{
		num_f_channels = 12,
		max_num_control_points = 128,
	};
public:
	LBM_Multicomponent(decltype(dx11_wnd) wnd) : dx11_wnd{ wnd } {}
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
		const int width = dx11_wnd->getWidth();
		const int height = dx11_wnd->getHeight();

		//应用控制点
		if (control_points.size() > 0)
		{
			ID3D11ShaderResourceView* null_srv = nullptr;
			ID3D11UnorderedAccessView* null_uav = nullptr;
			ctx->CSSetUnorderedAccessViews(6, 1, uav_tex_quantites.GetAddressOf(), 0);
			ctx->PSSetShaderResources(0, 1, &null_srv);
			update_control_point_buffer();
			ctx->CSSetShader(cs_draw.Get(), NULL, 0);
			control_points.clear();
			ctx->Dispatch(width / 32 + 1, height / 32 + 1, 1);
			ctx->CSSetUnorderedAccessViews(6, 1, &null_uav, 0);
			ctx->PSSetShaderResources(0, 1, srv_tex_quantites.GetAddressOf());
		}
		//
	}

	void render()
	{
		decltype(auto) ctx = dx11_wnd->GetImCtx();
		constexpr float back_color[4] = { 0.f,0.f,0.f,1.f };
		ctx->ClearRenderTargetView(dx11_wnd->GetRTV(), back_color);
		ctx->ClearDepthStencilView(dx11_wnd->GetDsv(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
		ctx->Draw(6, 0);
		dx11_wnd->GetSwapChain()->Present(0, 0);
	}
	void handleInput() {}
	void init_shaders()
	{
		ComPtr<ID3DBlob> blob;
		decltype(auto) device = dx11_wnd->GetDevice();
		HRESULT hr = NULL;
		//创建VS shader
		hr = D3DReadFileToBlob(L"shaders/vs.cso", blob.ReleaseAndGetAddressOf());
		assert(SUCCEEDED(hr));
		hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, vs.GetAddressOf());
		assert(SUCCEEDED(hr));
		//创建 input_layout
		hr = device->CreateInputLayout(VsIn::input_layout, VsIn::num_elements, blob->GetBufferPointer(), blob->GetBufferSize(), input_layout.GetAddressOf());
		assert(SUCCEEDED(hr));
		//创建PS shader
		hr = D3DReadFileToBlob(L"shaders/ps.cso", blob.ReleaseAndGetAddressOf());
		assert(SUCCEEDED(hr));
		hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, ps.GetAddressOf());
		assert(SUCCEEDED(hr));

		hr = D3DReadFileToBlob(L"shaders/draw.cso", blob.ReleaseAndGetAddressOf());
		assert(SUCCEEDED(hr));
		hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_draw.GetAddressOf());
		assert(SUCCEEDED(hr));
	}
	void init_resources()
	{
		decltype(auto) device = dx11_wnd->GetDevice();
		HRESULT hr = NULL;
		D3D11_TEXTURE2D_DESC tex_desc = {};
		D3D11_BUFFER_DESC buf_desc = {};
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};

		//物理量的 tex uav srv
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

		hr = device->CreateTexture2D(&tex_desc, 0, tex_quantities.GetAddressOf());
		assert(SUCCEEDED(hr));
		hr = device->CreateUnorderedAccessView(tex_quantities.Get(), &uav_desc, uav_tex_quantites.GetAddressOf());
		assert(SUCCEEDED(hr));
		hr = device->CreateShaderResourceView(tex_quantities.Get(), &srv_desc, srv_tex_quantites.GetAddressOf());
		assert(SUCCEEDED(hr));

		//分别创建3个组分个粒子分布以及宏观量的存储
		tex_desc = {};
		tex_desc.Width = dx11_wnd->getWidth();
		tex_desc.Height = dx11_wnd->getHeight();
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
		hr = device->CreateTexture2D(&tex_desc, 0, tex_array_f_in[0].GetAddressOf());
		hr = device->CreateTexture2D(&tex_desc, 0, tex_array_f_in[1].GetAddressOf());
		hr = device->CreateTexture2D(&tex_desc, 0, tex_array_f_in[2].GetAddressOf());
		hr = device->CreateTexture2D(&tex_desc, 0, tex_array_f_out[0].GetAddressOf());
		hr = device->CreateTexture2D(&tex_desc, 0, tex_array_f_out[1].GetAddressOf());
		hr = device->CreateTexture2D(&tex_desc, 0, tex_array_f_out[2].GetAddressOf());
		//...

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
	}

	void bind_resources()
	{
		decltype(auto) device = dx11_wnd->GetDevice();
		decltype(auto) ctx = dx11_wnd->GetImCtx();

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
		ctx->IASetInputLayout(input_layout.Get());
		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		ctx->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);
		ctx->VSSetShader(vs.Get(), 0, 0);
		ctx->PSSetShader(ps.Get(), 0, 0);

		ctx->CSSetUnorderedAccessViews(0, 1, uav_tex_array_f_in[0].GetAddressOf(), 0);
		ctx->CSSetUnorderedAccessViews(1, 1, uav_tex_array_f_in[1].GetAddressOf(), 0);
		ctx->CSSetUnorderedAccessViews(2, 1, uav_tex_array_f_in[2].GetAddressOf(), 0);
		ctx->CSSetUnorderedAccessViews(3, 1, uav_tex_array_f_out[0].GetAddressOf(), 0);
		ctx->CSSetUnorderedAccessViews(4, 1, uav_tex_array_f_out[1].GetAddressOf(), 0);
		ctx->CSSetUnorderedAccessViews(5, 1, uav_tex_array_f_out[2].GetAddressOf(), 0);
		ctx->CSSetShaderResources(0, 1, srv_control_points.GetAddressOf());
		ctx->CSSetConstantBuffers(0, 1, cbuf_num_control_points.GetAddressOf());
		ctx->PSSetShaderResources(0, 1, srv_tex_quantites.GetAddressOf());
	}

	void add_control_point(XMFLOAT2 pos, XMFLOAT2 data);
	void set_input_callback();
};
