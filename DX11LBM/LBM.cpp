#include "LBM.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <stdexcept>
#include <list>
#include <wrl/client.h>
#include "InputManager.h"
#include "header.hpp"

using namespace DirectX;
using namespace Microsoft::WRL;
struct LBM::IMPL
{
public:
	ComPtr<ID3D11Device> device;
	ComPtr<IDXGISwapChain> swap_chain;
	ComPtr<ID3D11DeviceContext> context;
	void init();
	void createUAV1();
	void createOutTexrue();
	void createSRV0();
	void createInTexrue();
	void createCS0();
	void getBackBufferRTV();
	void getBackBuffer();
	void process();
private:
	void draw();
	void handleInput();
	void draw_point();
	void update();
	void clear();

	ComPtr<ID3D11ComputeShader> cs0;

	ComPtr<ID3D11Texture2D> back_buffer;
	ComPtr<ID3D11RenderTargetView> back_buffer_rtv;

	ComPtr<ID3D11Texture2D> in_texrue;
	ComPtr<ID3D11Texture2D> out_texrue;
	ComPtr<ID3D11ShaderResourceView> in_texrue_srv;

	D3D11_MAPPED_SUBRESOURCE in_texrue_ms0 = {};
	ComPtr<ID3D11UnorderedAccessView> out_texrue_uav;

	struct Spot
	{
		Pos pos;
		UINT size;
		BYTE color[4];
	};

	void smooth_add_point(Spot&& a, Spot&& b);

	std::list<Spot> point_buffer;
};

LBM::LBM() : pimpl{ new LBM::IMPL{} }
{
}

void LBM::init(ID3D11Device* device, IDXGISwapChain* swap_chain, ID3D11DeviceContext* context)
{
	pimpl->device = device;
	pimpl->swap_chain = swap_chain;
	pimpl->context = context;
	pimpl->init();
}

void LBM::process()
{
	pimpl->process();
}

LBM::~LBM()
{
}

void LBM::IMPL::process()
{
	draw();
	handleInput();
	update();
}

void LBM::IMPL::init()
{
	//init back_buffer_render_target_view

	getBackBuffer();
	getBackBufferRTV();

	//创建计算着色器
	createCS0();

	//创建纹理0, cpu可读写，用于鼠标描绘
	createInTexrue();
	createSRV0();

	//创建纹理1，Gpu可读写，用于输出
	createOutTexrue();
	createUAV1();
}

void LBM::IMPL::createUAV1()
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC desc_uav1 = {};
	desc_uav1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc_uav1.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	desc_uav1.Texture2D.MipSlice = 0;

	if (FAILED(device->CreateUnorderedAccessView(out_texrue.Get(), &desc_uav1, out_texrue_uav.GetAddressOf())))
	{
		throw std::runtime_error("Failed to create unordered access view");
	}
}

void LBM::IMPL::createOutTexrue()
{
	D3D11_TEXTURE2D_DESC desc_out_texrue = { 0 };
	desc_out_texrue.Width = EWndSize::width;
	desc_out_texrue.Height = EWndSize::height;
	desc_out_texrue.ArraySize = 1;
	desc_out_texrue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc_out_texrue.Usage = D3D11_USAGE_DEFAULT;
	desc_out_texrue.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc_out_texrue.CPUAccessFlags = 0;// D3D11_CPU_ACCESS_WRITE;
	desc_out_texrue.MiscFlags = 0;
	desc_out_texrue.MipLevels = 1;
	desc_out_texrue.SampleDesc.Count = 1;
	//desc_out_texrue.SampleDesc.Quality = 0;

	if (FAILED(device->CreateTexture2D(&desc_out_texrue, 0, out_texrue.GetAddressOf())))
	{
		throw std::runtime_error("Failed to create out_texrue");
	}
}

void LBM::IMPL::createSRV0()
{
	D3D11_SHADER_RESOURCE_VIEW_DESC desc_srv0 = {};
	desc_srv0.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc_srv0.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	desc_srv0.Texture2D.MipLevels = 1;
	desc_srv0.Texture2D.MostDetailedMip = 0;

	if (FAILED(device->CreateShaderResourceView(in_texrue.Get(), &desc_srv0, in_texrue_srv.GetAddressOf())))
	{
		throw std::runtime_error("Faild to create srv0");
	}
}

void LBM::IMPL::createInTexrue()
{
	D3D11_TEXTURE2D_DESC desc_in_texrue = { 0 };
	desc_in_texrue.Width = EWndSize::width;
	desc_in_texrue.Height = EWndSize::height;
	desc_in_texrue.ArraySize = 1;
	desc_in_texrue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc_in_texrue.Usage = D3D11_USAGE_DYNAMIC;
	desc_in_texrue.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc_in_texrue.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc_in_texrue.MiscFlags = 0;
	desc_in_texrue.MipLevels = 1;
	desc_in_texrue.SampleDesc.Count = 1;

	if (FAILED(device->CreateTexture2D(&desc_in_texrue, 0, &in_texrue)))
	{
		throw std::runtime_error("Failed to create in_texrue");
	}
}

void LBM::IMPL::createCS0()
{
	ComPtr<ID3DBlob> blob;
	D3DReadFileToBlob(L"test.cso", blob.GetAddressOf());

	if (FAILED(device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs0.GetAddressOf())))
	{
		throw std::runtime_error("Failed to create compute shader");
	}
}

void LBM::IMPL::getBackBufferRTV()
{
	if (FAILED(device->CreateRenderTargetView(back_buffer.Get(), 0, back_buffer_rtv.GetAddressOf())))
	{
		throw std::runtime_error("Failed to create RenderTargetView");
	}

	//context->OMSetRenderTargets(1, back_buffer_rtv.GetAddressOf(), 0);

	//float clear_color[4] = { 0.1f,0.1f,0.5f,0.5f };
	//context->ClearRenderTargetView(back_buffer_rtv.Get(), clear_color);
}

void LBM::IMPL::getBackBuffer()
{
	if (FAILED(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(back_buffer.GetAddressOf()))))
	{
		throw std::runtime_error("Failed to get backbuffer");
	}
}

void LBM::IMPL::draw()
{
	context->CSSetShader(cs0.Get(), 0, 0);
	context->CSSetShaderResources(0, 1, in_texrue_srv.GetAddressOf());
	context->CSSetUnorderedAccessViews(0, 1, out_texrue_uav.GetAddressOf(), 0);
	context->Dispatch(EWndSize::width, EWndSize::height, 1);
	context->CopyResource(back_buffer.Get(), out_texrue.Get());
	//context->CopyResource(back_buffer.Get(), in_texrue.Get());
	swap_chain->Present(0, 0);
}

void LBM::IMPL::handleInput()
{
	static bool last_lbtn_down = false;
	InputManager& input = InputManager::getInstance();

	if (input.mouseBtnDown(0))
	{
		const Pos pos = input.getMousePos();
		if (point_buffer.size() > 0)
		{
			auto old = point_buffer.back();
			point_buffer.pop_back();
			smooth_add_point(std::move(old), { pos,20,{213,213,213,255} });
		}
		point_buffer.push_back({ pos,20,{213,213,213,255} });
	}
	else if (input.mouseBtnDown(2))
	{
		const Pos pos = input.getMousePos();
		if (point_buffer.size() > 0)
		{
			auto old = point_buffer.back();
			point_buffer.pop_back();
			smooth_add_point(std::move(old), { pos,20,{0,0,0,0} });
		}
		point_buffer.push_back({ pos,20,{0,0,0,0} });
	}
}

void LBM::IMPL::update()
{
	draw_point();
	//...
}

void LBM::IMPL::clear()
{
	//...
}

void LBM::IMPL::draw_point()
{
	static size_t time = 0;

	if (time++ % 10)
	{
		return;
	}

	if (FAILED(context->Map(in_texrue.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &in_texrue_ms0)))
	{
		throw std::runtime_error("Failed to Map");
	}

	BYTE* p_data = (BYTE*)in_texrue_ms0.pData;
	int x = 0;
	int y = 0;

	for (const Spot& spot : point_buffer)
	{
		std::tie(x, y) = spot.pos;
		const int size = spot.size;
		for (int i = -size / 2; i < size / 2; i++)
		{
			for (int j = -size / 2; j < size / 2; j++)
			{
				int xx = x + j;
				int yy = y + i;

				if (xx >= EWndSize::width || xx < 0 || yy >= EWndSize::height || yy < 0)
				{
					continue;
				}

				size_t index = in_texrue_ms0.RowPitch * yy + xx * 4;
				memcpy(p_data + index, &spot.color, sizeof(BYTE) * 4);
			}
		}
	}
	context->Unmap(in_texrue.Get(), 0);

	point_buffer.clear();
}

void LBM::IMPL::smooth_add_point(Spot&& a, Spot&& b)
{
	const Pos a_pos = a.pos;
	const Pos b_pos = b.pos;
	const int a_x = std::get<0>(a_pos);
	const int a_y = std::get<1>(a_pos);
	const int b_x = std::get<0>(b_pos);
	const int b_y = std::get<1>(b_pos);
	const int d = (a_x - b_x) * (a_x - b_x) + (a_y - b_y) * (a_y - b_y);
	if (d > 50)
	{
		const int _x = b_x - a_x;
		const int _y = b_y - a_y;
		const int _x_1 = a_x + _x / 3;
		const int _y_1 = a_y + _y / 3;
		const int _x_2 = a_x + _x * 2 / 3;
		const int _y_2 = a_y + _y * 2 / 3;
		Spot _a = { {_x_1,_y_1},a.size,{a.color[0],a.color[1],a.color[2],a.color[3]} };
		Spot _b = { {_x_2,_y_2},b.size,{b.color[0],b.color[1],b.color[2],b.color[3]} };

		smooth_add_point(std::move(a), std::move(_a));
		smooth_add_point(std::move(_b), std::move(b));
		return;
	}
	point_buffer.push_back(std::move(a));
	point_buffer.push_back(std::move(b));
}
