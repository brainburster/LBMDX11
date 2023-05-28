#include "LBM.hpp"
#include "InputManager.h"
#include "header.hpp"
#include <DirectXMath.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <list>
#include <stdexcept>
#include <wrl/client.h>

using namespace DirectX;
using namespace Microsoft::WRL;
struct LBM::IMPL {
public:
  ComPtr<ID3D11Device> device;
  ComPtr<IDXGISwapChain> swap_chain;
  ComPtr<ID3D11DeviceContext> context;
  void init();
  void process();

private:
  void dispatch();

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
  void createCS_lbm_collision();
  void createCS_lbm_streaming();
  void createCS_lbm_bundary();
  void createCS_init();

  void createCS_visualization();
  void fence();

  void getBackBufferRTV();
  void getBackBuffer();
  ComPtr<ID3D11ComputeShader> cs_init;
  ComPtr<ID3D11ComputeShader> cs_lbm_collision;
  ComPtr<ID3D11ComputeShader> cs_lbm_bundary;
  ComPtr<ID3D11ComputeShader> cs_lbm_streaming;

  ComPtr<ID3D11ComputeShader> cs_visualization;

  ComPtr<ID3D11Texture2D> back_buffer;
  ComPtr<ID3D11RenderTargetView> rtv_back_buffer;

  ComPtr<ID3D11Texture2D> tex_in;
  ComPtr<ID3D11Texture2D> tex_out;
  ComPtr<ID3D11Texture2D> tex_array_f0;
  ComPtr<ID3D11Texture2D> tex_array_f1;

  ComPtr<ID3D11ShaderResourceView> srv_tex_in;

  D3D11_MAPPED_SUBRESOURCE ms_tex_in = {};
  ComPtr<ID3D11UnorderedAccessView> uav_tex_out;
  ComPtr<ID3D11UnorderedAccessView> uav_tex_array_f0;
  ComPtr<ID3D11UnorderedAccessView> uav_tex_array_f1;

  struct Spot {
    Pos pos;
    UINT size;
    BYTE color[4];
  };

  std::list<Spot> point_buffer;
  Pos last = {-1, -1};
};

LBM::LBM() : pimpl{std::make_unique<IMPL>()} {}

void LBM::init(ID3D11Device *device, IDXGISwapChain *swap_chain,
               ID3D11DeviceContext *context) {
  pimpl->device = device;
  pimpl->swap_chain = swap_chain;
  pimpl->context = context;
  pimpl->init();
}

void LBM::process() { pimpl->process(); }

LBM::~LBM() {}

void LBM::IMPL::process() {
  draw();
  handleInput();
  update();
}

void LBM::IMPL::init() {
  // init back_buffer_render_target_view

  getBackBuffer();
  getBackBufferRTV();

  // 创建计算着色器
  createCS_init();
  createCS_lbm_collision();
  createCS_lbm_bundary();
  createCS_lbm_streaming();
  createCS_visualization();

  buildResource();

  context->CSSetShaderResources(0, 1, srv_tex_in.GetAddressOf());
  context->CSSetUnorderedAccessViews(0, 1, uav_tex_out.GetAddressOf(), 0);
  context->CSSetUnorderedAccessViews(1, 1, uav_tex_array_f0.GetAddressOf(), 0);
  context->CSSetUnorderedAccessViews(2, 1, uav_tex_array_f1.GetAddressOf(), 0);
  context->CSSetShader(cs_init.Get(), 0, 0);
  dispatch();
}

void LBM::IMPL::buildResource() {
  // 创建纹理0, cpu可读写，用于鼠标描绘
  createInTexture();
  createInTextureSRV();

  // 创建纹理1，Gpu可读写，用于输出
  createOutTexture();
  createOutTextureUAV();

  //
  createF0();
  createF0UAV();
  createF1();
  createF1UAV();
}

void LBM::IMPL::createF0() {
  D3D11_TEXTURE2D_DESC desc = {0};
  desc.Width = Setting::width;
  desc.Height = Setting::height;
  desc.ArraySize = 9;
  desc.Format = DXGI_FORMAT_R32_FLOAT;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;
  desc.MiscFlags = 0;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  // desc_out_texrue.SampleDesc.Quality = 0;

  if (FAILED(device->CreateTexture2D(&desc, 0, tex_array_f0.GetAddressOf()))) {
    throw std::runtime_error("Failed to create f0");
  }
}

void LBM::IMPL::createF1() {
  D3D11_TEXTURE2D_DESC desc = {0};
  desc.Width = Setting::width;
  desc.Height = Setting::height;
  desc.ArraySize = 9;
  desc.Format = DXGI_FORMAT_R32_FLOAT;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;
  desc.MiscFlags = 0;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  // desc_out_texrue.SampleDesc.Quality = 0;

  if (FAILED(device->CreateTexture2D(&desc, 0, tex_array_f1.GetAddressOf()))) {
    throw std::runtime_error("Failed to create texture f1");
  }
}

void LBM::IMPL::createF0UAV() {
  D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
  desc.Format = DXGI_FORMAT_R32_FLOAT;
  desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
  desc.Texture2DArray.ArraySize = 9;
  desc.Texture2DArray.MipSlice = 0;
  desc.Texture2DArray.FirstArraySlice = 0;
  if (FAILED(device->CreateUnorderedAccessView(
          tex_array_f0.Get(), &desc, uav_tex_array_f0.GetAddressOf()))) {
    throw std::runtime_error("Failed to create f0 uav");
  }
}
void LBM::IMPL::createF1UAV() {
  D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
  desc.Format = DXGI_FORMAT_R32_FLOAT;
  desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
  desc.Texture2DArray.ArraySize = 9;
  desc.Texture2DArray.MipSlice = 0;
  desc.Texture2DArray.FirstArraySlice = 0;

  if (FAILED(device->CreateUnorderedAccessView(
          tex_array_f1.Get(), &desc, uav_tex_array_f1.GetAddressOf()))) {
    throw std::runtime_error("Failed to create f1 uav");
  }
}

void LBM::IMPL::createOutTexture() {
  D3D11_TEXTURE2D_DESC desc = {0};
  desc.Width = Setting::width;
  desc.Height = Setting::height;
  desc.ArraySize = 1;
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0; // D3D11_CPU_ACCESS_WRITE;
  desc.MiscFlags = 0;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  // desc_out_texrue.SampleDesc.Quality = 0;

  if (FAILED(device->CreateTexture2D(&desc, 0, tex_out.GetAddressOf()))) {
    throw std::runtime_error("Failed to create texture f0");
  }
}

void LBM::IMPL::createOutTextureUAV() {
  D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
  desc.Texture2D.MipSlice = 0;

  if (FAILED(device->CreateUnorderedAccessView(tex_out.Get(), &desc,
                                               uav_tex_out.GetAddressOf()))) {
    throw std::runtime_error("Failed to create out texture uav");
  }
}

void LBM::IMPL::createInTextureSRV() {
  D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  desc.Texture2D.MipLevels = 1;
  desc.Texture2D.MostDetailedMip = 0;

  if (FAILED(device->CreateShaderResourceView(tex_in.Get(), &desc,
                                              srv_tex_in.GetAddressOf()))) {
    throw std::runtime_error("Faild to create srv0");
  }
}

void LBM::IMPL::createInTexture() {
  D3D11_TEXTURE2D_DESC desc = {0};
  desc.Width = Setting::width;
  desc.Height = Setting::height;
  desc.ArraySize = 1;
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.Usage = D3D11_USAGE_DYNAMIC;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  desc.MiscFlags = 0;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;

  if (FAILED(device->CreateTexture2D(&desc, 0, tex_in.GetAddressOf()))) {
    throw std::runtime_error("Failed to create in_texrue");
  }
}

void LBM::IMPL::createCS_lbm_collision() {
  ComPtr<ID3DBlob> blob;
  D3DReadFileToBlob(L"shaders/lbm_collision.cso", blob.GetAddressOf());

  if (FAILED(device->CreateComputeShader(blob->GetBufferPointer(),
                                         blob->GetBufferSize(), nullptr,
                                         cs_lbm_collision.GetAddressOf()))) {
    throw std::runtime_error(
        "Failed to create compute shader lbm_collision.cso");
  }
}

void LBM::IMPL::createCS_lbm_streaming() {
  ComPtr<ID3DBlob> blob;
  D3DReadFileToBlob(L"shaders/lbm_streaming.cso", blob.GetAddressOf());

  if (FAILED(device->CreateComputeShader(blob->GetBufferPointer(),
                                         blob->GetBufferSize(), nullptr,
                                         cs_lbm_streaming.GetAddressOf()))) {
    throw std::runtime_error(
        "Failed to create compute shader lbm_streaming.cso");
  }
}

void LBM::IMPL::createCS_lbm_bundary() {
  ComPtr<ID3DBlob> blob;
  D3DReadFileToBlob(L"shaders/lbm_bundary.cso", blob.GetAddressOf());

  if (FAILED(device->CreateComputeShader(blob->GetBufferPointer(),
                                         blob->GetBufferSize(), nullptr,
                                         cs_lbm_bundary.GetAddressOf()))) {
    throw std::runtime_error("Failed to create compute shader lbm_bundary.cso");
  }
}

void LBM::IMPL::createCS_init() {
  ComPtr<ID3DBlob> blob;
  D3DReadFileToBlob(L"shaders/init.cso", blob.GetAddressOf());

  if (FAILED(device->CreateComputeShader(blob->GetBufferPointer(),
                                         blob->GetBufferSize(), nullptr,
                                         cs_init.GetAddressOf()))) {
    throw std::runtime_error("Failed to create compute shader init.cso");
  }
}

void LBM::IMPL::createCS_visualization() {
  ComPtr<ID3DBlob> blob;
  D3DReadFileToBlob(L"shaders/visualization.cso", blob.GetAddressOf());

  if (FAILED(device->CreateComputeShader(blob->GetBufferPointer(),
                                         blob->GetBufferSize(), nullptr,
                                         cs_visualization.GetAddressOf()))) {
    throw std::runtime_error("Failed to create compute shader outline.cso");
  }
}

void LBM::IMPL::fence() {
  ComPtr<ID3D11Query> event_query;
  D3D11_QUERY_DESC queryDesc{};
  queryDesc.Query = D3D11_QUERY_EVENT;
  queryDesc.MiscFlags = 0;
  device->CreateQuery(&queryDesc, event_query.GetAddressOf());
  context->End(event_query.Get());
  while (context->GetData(event_query.Get(), NULL, 0, 0) == S_FALSE) {
  }
}

void LBM::IMPL::getBackBufferRTV() {
  if (FAILED(device->CreateRenderTargetView(back_buffer.Get(), 0,
                                            rtv_back_buffer.GetAddressOf()))) {
    throw std::runtime_error("Failed to create RenderTargetView");
  }

  // context->OMSetRenderTargets(1, back_buffer_rtv.GetAddressOf(), 0);

  // float clear_color[4] = { 0.1f,0.1f,0.5f,0.5f };
  // context->ClearRenderTargetView(back_buffer_rtv.Get(), clear_color);
}

void LBM::IMPL::getBackBuffer() {
  if (FAILED(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                   (void **)(back_buffer.GetAddressOf())))) {
    throw std::runtime_error("Failed to get backbuffer");
  }
}

void LBM::IMPL::dispatch() {
  context->Dispatch((Setting::width - 1) / 32 + 1,
                    (Setting::height - 1) / 32 + 1, 1);
}

void LBM::IMPL::draw() {
  fence();
  context->CSSetShader(cs_visualization.Get(), 0, 0);
  dispatch();
  fence();
  context->CopyResource(back_buffer.Get(), tex_out.Get());
  swap_chain->Present(0, 0);
}

void LBM::IMPL::handleInput() {
  static bool last_lbtn_down = false;
  static HWND hwnd = FindWindow(Setting::cls_name, Setting::wnd_name);
  static bool btndown = false;
  InputManager &input = InputManager::getInstance();

  if (input.mouseBtnDown(0)) {
    Pos pos = input.getMousePos();
    btndown = true;
    RECT rect = {};
    DXGI_OUTPUT_DESC *desc = nullptr;
    GetClientRect(hwnd, &rect);
    std::get<0>(pos) =
        std::get<0>(pos) * Setting::width / (rect.right - rect.left);
    std::get<1>(pos) =
        std::get<1>(pos) * Setting::height / (rect.bottom - rect.top);

    if (point_buffer.size() > 0) {
      decltype(auto) old = point_buffer.back();
      point_buffer.pop_back();
    }
    point_buffer.push_back({pos, 20, {213, 213, 213, 255}});
  } else if (input.mouseBtnDown(2) || input.mouseBtnDown(1)) {
    Pos pos = input.getMousePos();
    btndown = true;
    // auto [x, y] = pos;
    RECT rect = {};
    GetClientRect(hwnd, &rect);
    std::get<0>(pos) =
        std::get<0>(pos) * Setting::width / (rect.right - rect.left);
    std::get<1>(pos) =
        std::get<1>(pos) * Setting::height / (rect.bottom - rect.top);

    if (point_buffer.size() > 0) {
      decltype(auto) old = point_buffer.back();
      point_buffer.pop_back();
    }
    point_buffer.push_back({pos, 20, {0, 0, 0, 0}});
  }
  if (btndown && !input.mouseBtnDown(0) && !input.mouseBtnDown(2)) {
    btndown = false;
    last = {-1, -1};
  }
  draw_point();
}

void LBM::IMPL::update() {
  fence();
  context->CSSetShader(cs_lbm_streaming.Get(), 0, 0);
  dispatch();

  fence();
  context->CSSetShader(cs_lbm_bundary.Get(), 0, 0);
  context->Dispatch(2, 1, 1);

  fence();
  context->CSSetShader(cs_lbm_collision.Get(), 0, 0);
  dispatch();

  Sleep(1); // 减少cpu占用
  //...
}

void LBM::IMPL::clear() {
  //...
}

void LBM::IMPL::draw_point() {
  fence();
  if (FAILED(context->Map(tex_in.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                          &ms_tex_in))) {
    throw std::runtime_error("Failed to Map");
  }

  BYTE *p_data = (BYTE *)ms_tex_in.pData;
  int x = 0;
  int y = 0;

  const auto plot = [&](int x, int y, int size, const void *color) {
    for (int i = -size / 2; i < size / 2; i++) {
      for (int j = -size / 2; j < size / 2; j++) {
        if ((i * i + j * j) > (size / 2.f) * (size / 2.f) - 0.5f) {
          continue;
        }
        int xx = x + j;
        int yy = y + i;

        if (xx >= Setting::width || xx < 0 || yy >= Setting::height || yy < 0) {
          continue;
        }
        int index = ms_tex_in.RowPitch * yy + xx * 4;
        memcpy(p_data + index, color, sizeof(BYTE) * 4);
      }
    }
  };

  // bresenham's line算法,
  // https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
  const auto plot_line = [&](int x0, int y0, int x1, int y1, int size,
                             const void *color) {
    const int dx = abs(x1 - x0);
    const int dy = -abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy; /* error value e_xy */
    while (true) {
      plot(x0, y0, size, color);
      if (x0 == x1 && y0 == y1)
        break;
      const int e2 = 2 * err;
      /* e_xy+e_x > 0 */
      if (e2 >= dy) {
        err += dy;
        x0 += sx;
      }
      /* e_xy+e_y < 0 */
      if (e2 <= dx) {
        err += dx;
        y0 += sy;
      }
    }
  };

  for (const Spot &spot : point_buffer) {
    std::tie(x, y) = spot.pos;
    if (std::get<0>(last) < 0) {
      last = {x, y};
    }
    plot_line(std::get<0>(last), std::get<1>(last), x, y, spot.size,
              &spot.color);
    // plot(xx, yy, &spot.color);
    last = {x, y};
  }
  context->Unmap(tex_in.Get(), 0);

  point_buffer.clear();
}
