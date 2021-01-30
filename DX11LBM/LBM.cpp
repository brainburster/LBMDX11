#include "LBM.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <stdexcept>
#include "InputManager.h"
#include "header.hpp"
#include <list>
//#pragma comment(lib,"dinput8.lib")
//#pragma comment(lib,"dxguid.lib")

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
		point_buffer.push_back({ pos,5,{233,233,233,255} });
	}

	if (input.mouseBtnDown(2))
	{
		const Pos pos = input.getMousePos();
		point_buffer.push_back({ pos,10,{0,0,0,0} });
	}
}

void LBM::IMPL::update()
{
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
		for (size_t i = 0; i < size; i++)
		{
			for (size_t j = 0; j < size; j++)
			{
				size_t index = ms0.RowPitch * (y + i) + (x + j) * 4;
				memcpy(p_data + index, &spot.color, sizeof(BYTE) * 4);
			}
		}
	}

	point_buffer.clear();

	context->Unmap(tex0, 0);
	//...
}
