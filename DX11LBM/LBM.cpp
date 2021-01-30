#include "LBM.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <stdexcept>
#include "InputManager.h"
#include "header.hpp"
#include <list>

using namespace DirectX;

struct LBM::IMPL
{
public:
	ID3D11Device* device;
	IDXGISwapChain* swap_chain;
	ID3D11DeviceContext* context;
	void init();
	void process();
private:
	void draw();
	void handleInput();
	void draw_point();
	void update();

	ID3D11Texture2D* back_buffer;
	ID3D11RenderTargetView* back_buffer_rtv;
	ID3D11Texture2D* tex0;
	ID3D11Texture2D* tex1;

	D3D11_MAPPED_SUBRESOURCE ms0;
	ID3D11UnorderedAccessView* tex1_uav;
	ID3D11ComputeShader* cs;

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
	if (pimpl)
	{
		delete pimpl;
		pimpl = nullptr;
	}
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

	if (FAILED(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(&back_buffer))))
	{
		throw std::runtime_error("Failed to get backbuffer");
	}

	if (FAILED(device->CreateRenderTargetView(back_buffer, 0, &back_buffer_rtv)))
	{
		throw std::runtime_error("Failed to create RenderTargetView");
	}

	context->OMSetRenderTargets(1, &back_buffer_rtv, 0);
	//back_buffer->Release();
	float clear_color[4] = { 0.1f,0.1f,0.5f,0.5f };
	context->ClearRenderTargetView(back_buffer_rtv, clear_color);

	//创建计算着色器
	ID3DBlob* blob;
	D3DReadFileToBlob(L"test.cso", &blob);

	if (FAILED(device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &cs)))
	{
		throw std::runtime_error("Failed to create compute shader");
	}
	//创建纹理0以及MappedResouce
	D3D11_TEXTURE2D_DESC desc_tex0 = { 0 };
	desc_tex0.Width = EWndSize::width;
	desc_tex0.Height = EWndSize::height;
	desc_tex0.ArraySize = 1;
	desc_tex0.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc_tex0.Usage = D3D11_USAGE_DYNAMIC;
	desc_tex0.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc_tex0.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc_tex0.MiscFlags = 0;
	desc_tex0.MipLevels = 1;
	desc_tex0.SampleDesc.Count = 1;

	if (FAILED(device->CreateTexture2D(&desc_tex0, 0, &tex0)))
	{
		throw std::runtime_error("Failed to create tex0");
	}

	//创建纹理1以及UAV
	D3D11_TEXTURE2D_DESC desc_tex1 = { 0 };
	desc_tex1.Width = EWndSize::width;
	desc_tex1.Height = EWndSize::height;
	desc_tex1.ArraySize = 1;
	desc_tex1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc_tex1.Usage = D3D11_USAGE_DEFAULT;
	desc_tex1.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc_tex1.CPUAccessFlags = 0;// D3D11_CPU_ACCESS_WRITE;
	desc_tex1.MiscFlags = 0;
	desc_tex1.MipLevels = 1;
	desc_tex1.SampleDesc.Count = 1;
	//desc_tex1.SampleDesc.Quality = 0;

	if (FAILED(device->CreateTexture2D(&desc_tex1, 0, &tex1)))
	{
		throw std::runtime_error("Failed to create tex1");
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc_uav1 = { };
	desc_uav1.Format = desc_tex1.Format;
	desc_uav1.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	desc_uav1.Texture2D.MipSlice = 0;

	if (FAILED(device->CreateUnorderedAccessView(tex1, &desc_uav1, &tex1_uav)))
	{
		throw std::runtime_error("Failed to create unordered access view");
	}
	swap_chain->Present(0, 0);
}

void LBM::IMPL::draw()
{
	//context->CSSetShader(cs, 0, 0);
	//context->CSSetUnorderedAccessViews(0u, 1u, &tex1_uav, 0);
	//context->Dispatch(EWndSize::width, EWndSize::height, 1);
	//context->CopyResource(back_buffer, tex1);
	context->CopyResource(back_buffer, tex0);
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

void LBM::IMPL::draw_point()
{
	static size_t time = 0;

	if (time++ % 123)
	{
		return;
	}

	if (FAILED(context->Map(tex0, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms0)))
	{
		throw std::runtime_error("Failed to Map");
	}

	BYTE* p_data = (BYTE*)ms0.pData;
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

				size_t index = ms0.RowPitch * yy + xx * 4;
				memcpy(p_data + index, &spot.color, sizeof(BYTE) * 4);
			}
		}
	}
	context->Unmap(tex0, 0);

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
