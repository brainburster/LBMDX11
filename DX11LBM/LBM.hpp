#pragma once

struct ID3D11Device;
struct IDXGISwapChain;
struct ID3D11DeviceContext;

class LBM
{
public:
	void init(ID3D11Device*, IDXGISwapChain*, ID3D11DeviceContext*);
	void process();
	LBM();
	~LBM();
private:
	LBM(const LBM&) = delete;
	LBM& operator=(const LBM&) = delete;
	LBM(LBM&&) = delete;
	LBM& operator=(LBM&&) = delete;

	struct IMPL;
	IMPL* pimpl;
};
