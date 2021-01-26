#include "LBM.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <stdexcept>

using namespace DirectX;

struct LBM::IMPL
{
public:
	ID3D11Device* p_device;
	IDXGISwapChain* p_swap_chain;
	ID3D11DeviceContext* p_context;
	void process();
};

LBM::LBM() : pimpl{ new LBM::IMPL{} }
{
}

void LBM::init(ID3D11Device* p_device, IDXGISwapChain* p_swap_chain, ID3D11DeviceContext* p_context)
{
	pimpl->p_device = p_device;
	pimpl->p_swap_chain = p_swap_chain;
	pimpl->p_context = p_context;
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
	ID3D11RenderTargetView* render_target_view;
	ID3D11Texture2D* back_buffer;
	p_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(&back_buffer));
	if (!back_buffer)
	{
		throw std::runtime_error("can't get backbuffer");
	}
	p_device->CreateRenderTargetView(back_buffer, 0, &render_target_view);
	if (!render_target_view)
	{
		throw std::runtime_error("can't create RenderTargetView");
	}
	p_context->OMSetRenderTargets(1, &render_target_view, 0);
	float clear_color[4] = { 0.5f,0.1f,0.1f,0.5f };
	p_context->ClearRenderTargetView(render_target_view, clear_color);
	back_buffer->Release();

	p_swap_chain->Present(0, 0);
}
