#include "LBM_Multicomponent.h"
#include <d3dcompiler.h>

LBM_Multicomponent::LBM_Multicomponent(decltype(dx11_wnd) wnd) : dx11_wnd{ wnd }, last_control_point{ {-1.f,-1.f},{-1.f,-1.f}, }, display_setting{ 0,0,0,0 }, pause{ false }{}

void LBM_Multicomponent::init_shaders()
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

	hr = D3DReadFileToBlob(L"shaders/cs_draw.cso", blob.ReleaseAndGetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_draw.GetAddressOf());
	assert(SUCCEEDED(hr));

	hr = D3DReadFileToBlob(L"shaders/cs_lbm_init.cso", blob.ReleaseAndGetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_lbm_init.GetAddressOf());
	assert(SUCCEEDED(hr));

	hr = D3DReadFileToBlob(L"shaders/cs_lbm_collision.cso", blob.ReleaseAndGetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_lbm_collision.GetAddressOf());
	assert(SUCCEEDED(hr));

	hr = D3DReadFileToBlob(L"shaders/cs_lbm_streaming.cso", blob.ReleaseAndGetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_lbm_streaming.GetAddressOf());
	assert(SUCCEEDED(hr));

	hr = D3DReadFileToBlob(L"shaders/cs_lbm_visualization.cso", blob.ReleaseAndGetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_lbm_visualization.GetAddressOf());
	assert(SUCCEEDED(hr));

	hr = D3DReadFileToBlob(L"shaders/cs_lbm_force_calculation.cso", blob.ReleaseAndGetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_lbm_force_calculation.GetAddressOf());
	assert(SUCCEEDED(hr));

	hr = D3DReadFileToBlob(L"shaders/cs_lbm_moment_update.cso", blob.ReleaseAndGetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_lbm_moment_update.GetAddressOf());
	assert(SUCCEEDED(hr));
}

void LBM_Multicomponent::update_displaysetting()
{
	decltype(auto) ctx = dx11_wnd->GetImCtx();
	D3D11_MAPPED_SUBRESOURCE mapped_subresource = {};
	HRESULT hr = ctx->Map(cbuf_display_setting.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
	assert(SUCCEEDED(hr));
	DisplaySetting* p_data = (DisplaySetting*)mapped_subresource.pData;
	*p_data = display_setting;
	ctx->Unmap(cbuf_display_setting.Get(), 0);
}

void LBM_Multicomponent::update_control_point_buffer()
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

void LBM_Multicomponent::fence()
{
	decltype(auto) device = dx11_wnd->GetDevice();
	decltype(auto) ctx = dx11_wnd->GetImCtx();
	ComPtr<ID3D11Query> event_query{};
	D3D11_QUERY_DESC queryDesc{};
	queryDesc.Query = D3D11_QUERY_EVENT;
	queryDesc.MiscFlags = 0;
	device->CreateQuery(&queryDesc, event_query.GetAddressOf());
	ctx->End(event_query.Get());
	while (ctx->GetData(event_query.Get(), NULL, 0, 0) == S_FALSE) {}
}

void LBM_Multicomponent::init()
{
	init_shaders();
	init_resources();
	bind_resources();
	set_input_callback();
	decltype(auto) ctx = dx11_wnd->GetImCtx();
	fence();
	ctx->CSSetShader(cs_lbm_init.Get(), 0, 0);
	ctx->Dispatch((dx11_wnd->getWidth() - 1) / 32 + 1, (dx11_wnd->getHeight() - 1) / 32 + 1, 1);
	fence();
}

//

void LBM_Multicomponent::run()
{
	init();
	while (!dx11_wnd->app_should_close())
	{
		dx11_wnd->PeekMsg();
		handleInput();
		update();
		render();
		//Sleep(1);
	}
}

void LBM_Multicomponent::init_resources()
{
	decltype(auto) device = dx11_wnd->GetDevice();
	HRESULT hr = NULL;
	D3D11_TEXTURE2D_DESC tex_desc = {};
	D3D11_BUFFER_DESC buf_desc = {};
	D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	D3D11_SUBRESOURCE_DATA InitData = {};

	tex_desc.Width = dx11_wnd->getWidth() / grid_size;
	tex_desc.Height = dx11_wnd->getHeight() / grid_size;
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

	hr = device->CreateTexture2D(&tex_desc, 0, tex_display.GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateUnorderedAccessView(tex_display.Get(), &uav_desc, uav_tex_display.GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateShaderResourceView(tex_display.Get(), &srv_desc, srv_tex_display.GetAddressOf());
	assert(SUCCEEDED(hr));

	//分别创建3个组分个粒子分布以及宏观量的存储
	tex_desc = {};
	tex_desc.Width = dx11_wnd->getWidth() / grid_size;
	tex_desc.Height = dx11_wnd->getHeight() / grid_size;
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

	assert(SUCCEEDED(hr));
	hr = device->CreateUnorderedAccessView(tex_array_f_in[0].Get(), &uav_desc, uav_tex_array_f_in[0].GetAddressOf());
	hr = device->CreateUnorderedAccessView(tex_array_f_in[1].Get(), &uav_desc, uav_tex_array_f_in[1].GetAddressOf());
	hr = device->CreateUnorderedAccessView(tex_array_f_in[2].Get(), &uav_desc, uav_tex_array_f_in[2].GetAddressOf());
	hr = device->CreateUnorderedAccessView(tex_array_f_out[0].Get(), &uav_desc, uav_tex_array_f_out[0].GetAddressOf());
	hr = device->CreateUnorderedAccessView(tex_array_f_out[1].Get(), &uav_desc, uav_tex_array_f_out[1].GetAddressOf());
	hr = device->CreateUnorderedAccessView(tex_array_f_out[2].Get(), &uav_desc, uav_tex_array_f_out[2].GetAddressOf());
	assert(SUCCEEDED(hr));

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

	//创建cbuf_SimSetting
	buf_desc = {};
	buf_desc.Usage = D3D11_USAGE_IMMUTABLE;
	buf_desc.ByteWidth = 16;
	buf_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buf_desc.CPUAccessFlags = 0;
	SimSetting sim_setting = {
		dx11_wnd->getWidth(),
		dx11_wnd->getHeight(),
		grid_size
	};
	InitData = {};
	InitData.pSysMem = &sim_setting;
	hr = device->CreateBuffer(&buf_desc, &InitData, cbuf_sim_setting.GetAddressOf());
	assert(SUCCEEDED(hr));

	//创建cbuf_SimSetting
	buf_desc = {};
	buf_desc.Usage = D3D11_USAGE_DYNAMIC;
	buf_desc.ByteWidth = 16;
	buf_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buf_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	InitData = {};
	InitData.pSysMem = &display_setting;
	hr = device->CreateBuffer(&buf_desc, &InitData, cbuf_display_setting.GetAddressOf());
	assert(SUCCEEDED(hr));
}

void LBM_Multicomponent::bind_resources()
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
	ctx->CSSetUnorderedAccessViews(6, 1, uav_tex_display.GetAddressOf(), 0);
	ctx->CSSetShaderResources(0, 1, srv_control_points.GetAddressOf());
	ctx->CSSetConstantBuffers(0, 1, cbuf_sim_setting.GetAddressOf());
	ctx->CSSetConstantBuffers(1, 1, cbuf_num_control_points.GetAddressOf());
	ctx->CSSetConstantBuffers(2, 1, cbuf_display_setting.GetAddressOf());
	//ctx->PSSetShaderResources(0, 1, srv_tex_display.GetAddressOf());
}

void LBM_Multicomponent::add_control_point(XMFLOAT2 pos, XMFLOAT2 data)
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

void LBM_Multicomponent::set_input_callback()
{
	const auto onmousemove = [&](WPARAM wparam, LPARAM lparam) {
		const POINTS p = MAKEPOINTS(lparam);
		const XMFLOAT2 pos = { (float)p.x / grid_size,(float)p.y / grid_size };
		if (wparam & MK_SHIFT) {
			add_control_point(pos, { 30.f / grid_size, 0.f });
		}
		else if (wparam & MK_LBUTTON) {
			add_control_point(pos, { 30.f / grid_size,1.f });
		}
		else if (wparam & MK_RBUTTON) {
			add_control_point(pos, { 30.f / grid_size,2.f });
		}
		else if (wparam & MK_MBUTTON) {
			add_control_point(pos, { 20.f / grid_size,3.f });
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

	dx11_wnd->AddWndProc(WM_KEYDOWN, [&](WPARAM wparam, LPARAM lparam) {
		switch (wparam)
		{
		case 'A':
		{
			display_setting.show_air = display_setting.show_air ? 0u : 1u;
			update_displaysetting();
			break;
		}
		case 'C':
		case 'S':
		{
			display_setting.velocitymode = display_setting.velocitymode ? 0u : 1u;
			update_displaysetting();
			break;
		}
		case 'F':
		{
			display_setting.forcemode = display_setting.forcemode ? 0u : 1u;
			update_displaysetting();
			break;
		}
		case 'V':
		{
			display_setting.vorticitymode = display_setting.vorticitymode ? 0u : 1u;
			update_displaysetting();
			break;
		}
		case VK_SPACE:
		case 'P':
		{
			pause = !pause;
			break;
		}
		default:
			break;
		}

		return true;
		});
}

void LBM_Multicomponent::update()
{
	decltype(auto) device = dx11_wnd->GetDevice();
	decltype(auto) ctx = dx11_wnd->GetImCtx();
	const int width = dx11_wnd->getWidth() / grid_size;
	const int height = dx11_wnd->getHeight() / grid_size;
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
	if (!pause)
	{
		ctx->CSSetShader(cs_lbm_force_calculation.Get(), NULL, 0);
		ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
		fence();
		ctx->CSSetShader(cs_lbm_collision.Get(), NULL, 0);
		ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
		fence();
		ctx->CSSetShader(cs_lbm_streaming.Get(), NULL, 0);
		ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
		fence();
	}
	//显示
	ctx->CSSetShader(cs_lbm_visualization.Get(), NULL, 0);
	ctx->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
	fence();
}

void LBM_Multicomponent::render()
{
	decltype(auto) ctx = dx11_wnd->GetImCtx();
	constexpr float back_color[4] = { 0.f,0.f,0.f,1.f };
	ctx->ClearRenderTargetView(dx11_wnd->GetRTV(), back_color);
	ctx->ClearDepthStencilView(dx11_wnd->GetDsv(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	//解决同一个资源同时绑定srv和uav的冲突
	ID3D11ShaderResourceView* null_srv = nullptr;
	ID3D11UnorderedAccessView* null_uav = nullptr;
	ctx->CSSetUnorderedAccessViews(6, 1, &null_uav, 0);
	ctx->PSSetShaderResources(0, 1, srv_tex_display.GetAddressOf());
	//绘制
	ctx->Draw(4, 0);
	ctx->CSSetUnorderedAccessViews(6, 1, uav_tex_display.GetAddressOf(), 0);
	ctx->PSSetShaderResources(0, 1, &null_srv);
	dx11_wnd->GetSwapChain()->Present(0, 0);
}
