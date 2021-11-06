#include "LBM_Multicomponent.h"

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

	hr = D3DReadFileToBlob(L"shaders/cs_lbm.cso", blob.ReleaseAndGetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_lbm.GetAddressOf());
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

void LBM_Multicomponent::init_resources()
{
	decltype(auto) device = dx11_wnd->GetDevice();
	HRESULT hr = NULL;
	D3D11_TEXTURE2D_DESC tex_desc = {};
	D3D11_BUFFER_DESC buf_desc = {};
	D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};

	tex_desc.Width = dx11_wnd->getWidth() / 4;
	tex_desc.Height = dx11_wnd->getHeight() / 4;
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
	tex_desc.Width = dx11_wnd->getWidth() / 4;
	tex_desc.Height = dx11_wnd->getHeight() / 4;
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
	hr = device->CreateTexture2D(&tex_desc, 0, tex_array_f_out[0].GetAddressOf());
	hr = device->CreateTexture2D(&tex_desc, 0, tex_array_f_out[1].GetAddressOf());
	assert(SUCCEEDED(hr));
	hr = device->CreateUnorderedAccessView(tex_array_f_in[0].Get(), &uav_desc, uav_tex_array_f_in[0].GetAddressOf());
	hr = device->CreateUnorderedAccessView(tex_array_f_in[1].Get(), &uav_desc, uav_tex_array_f_in[1].GetAddressOf());
	hr = device->CreateUnorderedAccessView(tex_array_f_out[0].Get(), &uav_desc, uav_tex_array_f_out[0].GetAddressOf());
	hr = device->CreateUnorderedAccessView(tex_array_f_out[1].Get(), &uav_desc, uav_tex_array_f_out[1].GetAddressOf());
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
	ctx->CSSetUnorderedAccessViews(2, 1, uav_tex_array_f_out[0].GetAddressOf(), 0);
	ctx->CSSetUnorderedAccessViews(3, 1, uav_tex_array_f_out[1].GetAddressOf(), 0);
	ctx->CSSetUnorderedAccessViews(4, 1, uav_tex_display.GetAddressOf(), 0);
	ctx->CSSetShaderResources(0, 1, srv_control_points.GetAddressOf());
	ctx->CSSetConstantBuffers(0, 1, cbuf_num_control_points.GetAddressOf());
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
		const XMFLOAT2 pos = { (float)p.x / 4,(float)p.y / 4 };
		if (wparam & MK_LBUTTON) {
			if (wparam & MK_SHIFT) {
				add_control_point(pos, { 5.f, 0.f });
			}
			else {
				add_control_point(pos, { 10.f,1.f });
			}
		}
		else if (wparam & MK_RBUTTON) {
			add_control_point(pos, { 10.f,2.f });
		}
		else if (wparam & MK_MBUTTON) {
			add_control_point(pos, { 1.f,3.f });
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
