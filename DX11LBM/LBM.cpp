#include "LBM.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <stdexcept>

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
	ID3D11Texture2D* tex1;
	ID3D11UnorderedAccessView* tex1_uav;
	ID3D11ComputeShader* cs;
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
	Sleep(1);
}

void LBM::IMPL::init()
{
	//init back_buffer_render_target_view
	swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(&back_buffer));
	if (!back_buffer)
	{
		throw std::runtime_error("can't get backbuffer");
	}
	device->CreateRenderTargetView(back_buffer, 0, &back_buffer_rtv);
	if (!back_buffer_rtv)
	{
		throw std::runtime_error("can't create RenderTargetView");
	}
	context->OMSetRenderTargets(1, &back_buffer_rtv, 0);
	//back_buffer->Release();
	float clear_color[4] = { 0.1f,0.1f,0.5f,0.5f };
	context->ClearRenderTargetView(back_buffer_rtv, clear_color);

	//创建计算着色器
	ID3DBlob* blob;
	D3DReadFileToBlob(L"test.cso", &blob);
	device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &cs);
	if (!cs)
	{
		throw std::runtime_error("can't create compute shader");
	}

	//创建纹理以及UAV
	D3D11_TEXTURE2D_DESC desc_tex1 = { 0 };
	desc_tex1.Width = 800;
	desc_tex1.Height = 600;
	desc_tex1.ArraySize = 1;
	desc_tex1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc_tex1.Usage = D3D11_USAGE_DEFAULT;
	desc_tex1.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc_tex1.CPUAccessFlags = 0;// D3D11_CPU_ACCESS_WRITE;
	desc_tex1.MipLevels = 1;
	desc_tex1.SampleDesc.Count = 1;
	desc_tex1.MiscFlags = 0;
	//desc_tex1.SampleDesc.Quality = 0;

	device->CreateTexture2D(&desc_tex1, 0, &tex1);

	if (!tex1)
	{
		throw std::runtime_error("can't create texture2d");
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc_uav1 = { };
	desc_uav1.Format = desc_tex1.Format;
	desc_uav1.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	desc_uav1.Texture2D.MipSlice = 0;

	device->CreateUnorderedAccessView(tex1, &desc_uav1, &tex1_uav);

	if (!tex1_uav)
	{
		throw std::runtime_error("can't create unordered access view");
	}

	swap_chain->Present(0, 0);
}

void LBM::IMPL::draw()
{
	context->CSSetShader(cs, 0, 0);
	context->CSSetUnorderedAccessViews(0u, 1u, &tex1_uav, 0);
	context->Dispatch(800, 600, 1);
	context->CopyResource(back_buffer, tex1);
	swap_chain->Present(0, 0);
}

void LBM::IMPL::handleInput()
{
}

void LBM::IMPL::update()
{
}
