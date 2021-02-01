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
	void process();
private:
	void draw();
	void handleInput();
	void draw_point();
	void update();
	void clear();

	void buildResource();
	void createF0UAV();
	void createF1UAV();
	void createOutTextureUAV();
	void createF0();
	void createF1();
	void createOutTexture();
	void createInTextureSRV();
	void createInTexture();
	void createCS_LbmD2Q9();
	void getBackBufferRTV();
	void getBackBuffer();

	ComPtr<ID3D11ComputeShader> cs_lbm_d2q4;

	ComPtr<ID3D11Texture2D> back_buffer;
	ComPtr<ID3D11RenderTargetView> rtv_back_buffer;

	ComPtr<ID3D11Texture2D> tex_in;
	ComPtr<ID3D11Texture2D> tex_out;
	ComPtr<ID3D11Texture2D> tex_array_f0;
	ComPtr<ID3D11Texture2D> tex_array_f1;

	ComPtr<ID3D11ShaderResourceView> srv_tex_in;

	D3D11_MAPPED_SUBRESOURCE ms_tex_in = {};
	ComPtr<ID3D11UnorderedAccessView> uav_tex_out;
	ComPtr<ID3D11UnorderedAccessView> uav_tex_f0;
	ComPtr<ID3D11UnorderedAccessView> uav_tex_f1;

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
	createCS_LbmD2Q9();

	buildResource();
}

void LBM::IMPL::buildResource()
{
	//创建纹理0, cpu可读写，用于鼠标描绘
	createInTexture();
	createInTextureSRV();

	//创建纹理1，Gpu可读写，用于输出
	createOutTexture();
	createOutTextureUAV();

	//
	createF0();
	createF0UAV();
	createF1();
	createF1UAV();
}

void LBM::IMPL::createF0()
{
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = EWndSize::width;
	desc.Height = EWndSize::height;
	desc.ArraySize = 9;
	desc.Format = DXGI_FORMAT_R8_UNORM;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	//desc_out_texrue.SampleDesc.Quality = 0;

	if (FAILED(device->CreateTexture2D(&desc, 0, tex_array_f0.GetAddressOf())))
	{
		throw std::runtime_error("Failed to create out_texrue");
	}
}

void LBM::IMPL::createF1()
{
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = EWndSize::width;
	desc.Height = EWndSize::height;
	desc.ArraySize = 9;
	desc.Format = DXGI_FORMAT_R8_UNORM;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	//desc_out_texrue.SampleDesc.Quality = 0;

	if (FAILED(device->CreateTexture2D(&desc, 0, tex_array_f1.GetAddressOf())))
	{
		throw std::runtime_error("Failed to create texture f1");
	}
}

void LBM::IMPL::createF0UAV()
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_R8_UNORM;
	desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	desc.Texture2DArray.ArraySize = 9;
	desc.Texture2DArray.MipSlice = 0;
	desc.Texture2DArray.FirstArraySlice = 0;
	if (FAILED(device->CreateUnorderedAccessView(tex_array_f0.Get(), &desc, uav_tex_f0.GetAddressOf())))
	{
		throw std::runtime_error("Failed to create f0 uav");
	}
}
void LBM::IMPL::createF1UAV()
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_R8_UNORM;
	desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	desc.Texture2DArray.ArraySize = 9;
	desc.Texture2DArray.MipSlice = 0;
	desc.Texture2DArray.FirstArraySlice = 0;

	if (FAILED(device->CreateUnorderedAccessView(tex_array_f1.Get(), &desc, uav_tex_f1.GetAddressOf())))
	{
		throw std::runtime_error("Failed to create f1 uav");
	}
}

void LBM::IMPL::createOutTexture()
{
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = EWndSize::width;
	desc.Height = EWndSize::height;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;// D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	//desc_out_texrue.SampleDesc.Quality = 0;

	if (FAILED(device->CreateTexture2D(&desc, 0, tex_out.GetAddressOf())))
	{
		throw std::runtime_error("Failed to create texture f0");
	}
}

void LBM::IMPL::createOutTextureUAV()
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;

	if (FAILED(device->CreateUnorderedAccessView(tex_out.Get(), &desc, uav_tex_out.GetAddressOf())))
	{
		throw std::runtime_error("Failed to create out texture uav");
	}
}

void LBM::IMPL::createInTextureSRV()
{
	D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = 1;
	desc.Texture2D.MostDetailedMip = 0;

	if (FAILED(device->CreateShaderResourceView(tex_in.Get(), &desc, srv_tex_in.GetAddressOf())))
	{
		throw std::runtime_error("Faild to create srv0");
	}
}

void LBM::IMPL::createInTexture()
{
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = EWndSize::width;
	desc.Height = EWndSize::height;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;

	if (FAILED(device->CreateTexture2D(&desc, 0, &tex_in)))
	{
		throw std::runtime_error("Failed to create in_texrue");
	}
}

void LBM::IMPL::createCS_LbmD2Q9()
{
	ComPtr<ID3DBlob> blob;
	D3DReadFileToBlob(L"lbm_d2q9.cso", blob.GetAddressOf());

	if (FAILED(device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, cs_lbm_d2q4.GetAddressOf())))
	{
		throw std::runtime_error("Failed to create compute shader");
	}
}

void LBM::IMPL::getBackBufferRTV()
{
	if (FAILED(device->CreateRenderTargetView(back_buffer.Get(), 0, rtv_back_buffer.GetAddressOf())))
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
	context->CopyResource(back_buffer.Get(), tex_out.Get());
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
			smooth_add_point(std::move(old), { pos,10,{213,213,213,255} });
		}
		point_buffer.push_back({ pos,10,{213,213,213,255} });
	}
	else if (input.mouseBtnDown(2))
	{
		const Pos pos = input.getMousePos();
		if (point_buffer.size() > 0)
		{
			auto old = point_buffer.back();
			point_buffer.pop_back();
			smooth_add_point(std::move(old), { pos,10,{0,0,0,0} });
		}
		point_buffer.push_back({ pos,10,{0,0,0,0} });
	}

	draw_point();
}

void LBM::IMPL::update()
{
	context->CSSetShader(cs_lbm_d2q4.Get(), 0, 0);
	context->CSSetShaderResources(0, 1, srv_tex_in.GetAddressOf());
	context->CSSetUnorderedAccessViews(0, 1, uav_tex_out.GetAddressOf(), 0);
	context->CSSetUnorderedAccessViews(1, 1, uav_tex_f0.GetAddressOf(), 0);
	context->CSSetUnorderedAccessViews(2, 1, uav_tex_f1.GetAddressOf(), 0);

	context->Dispatch(EWndSize::width, EWndSize::height, 1);
	//...
}

void LBM::IMPL::clear()
{
	//...
}

void LBM::IMPL::draw_point()
{
	static size_t time = 0;

	if (time++ % 4)
	{
		return;
	}

	if (FAILED(context->Map(tex_in.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms_tex_in)))
	{
		throw std::runtime_error("Failed to Map");
	}

	BYTE* p_data = (BYTE*)ms_tex_in.pData;
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

				size_t index = ms_tex_in.RowPitch * yy + xx * 4;
				memcpy(p_data + index, &spot.color, sizeof(BYTE) * 4);
			}
		}
	}
	context->Unmap(tex_in.Get(), 0);

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
