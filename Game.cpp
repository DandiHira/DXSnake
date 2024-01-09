#include <stdlib.h>
#include <stdio.h>
#include "pch.h"
#include "Game.h"

static HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
static inline Vec4 ConvertPosToScreenOffset(Pos pos);


Game::Game(uint32 numCellsPerRow, uint32 numCellsPerCol)
	: mNumCellsPerCol(numCellsPerCol)
	, mNumCellsPerRow(numCellsPerRow)
	, mSnakeLength(1)
	, mMaxSnakeLength(numCellsPerRow* numCellsPerCol)
	, mSnakePosLog(reinterpret_cast<Pos*>(malloc(sizeof(Pos)* numCellsPerRow * numCellsPerCol)))
	, mCellInfos(reinterpret_cast<eCellInfo*>(calloc(numCellsPerRow * numCellsPerCol, sizeof(eCellInfo))))
	, mHeadIdx(0)
	, mCurrDir(eDirection::RIGHT)
	, mPosToEraseOrNull(nullptr)
	, mPrevMoveResult(eMoveResult::JUST_MOVE)
{
	static_assert(static_cast<int>(eCellInfo::EMPTY) == 0, "if (EMPTY != 0), do not use calloc");

	if (!mSnakePosLog)
	{
		return;
	}

	Pos InitialSnakePos = Pos(static_cast<int>(numCellsPerRow >> 1), static_cast<int>(numCellsPerCol >> 1));
	mSnakePosLog[mHeadIdx] = InitialSnakePos;
	setCellInfoAt(*mSnakePosLog, eCellInfo::SNAKE);

	generateFoodAtRandomPos();
}

bool Game::InitD3D(HWND hWnd)
{

#define CHECK(hr)   if ((FAILED(hr))) return false
	HRESULT hr;

	// 1. Create device and swap chain
	{
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));

		sd.BufferDesc.Width = WINDOW_WIDTH;
		sd.BufferDesc.Height = WINDOW_HEIGHT;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		sd.SampleDesc.Quality = 0;
		sd.SampleDesc.Count = 1;

		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;
		sd.OutputWindow = hWnd;
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		HRESULT hr = D3D11CreateDeviceAndSwapChain
		(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&sd,
			&mSwapChain,
			&mDevice,
			nullptr,
			&mContext
		);

		CHECK(hr);
	}

	// 2. Create a render target view 
	{
		ID3D11Texture2D* pBackBuffer;
		hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
		CHECK(hr);

		hr = mDevice->CreateRenderTargetView(pBackBuffer, nullptr, &mRenderTargetView);
		pBackBuffer->Release();
		CHECK(hr);

		mContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);
	}

	// 3. Set the viewport
	{
		D3D11_VIEWPORT vp;
		vp.Width = (FLOAT)WINDOW_WIDTH;
		vp.Height = (FLOAT)WINDOW_HEIGHT;
		vp.MinDepth = 0.f;
		vp.MaxDepth = 1.f;
		vp.TopLeftX = 0.f;
		vp.TopLeftY = 0.f;
		mContext->RSSetViewports(1, &vp);
	}

	// 4. Create the vertex shader and input layout
	{
		ID3DBlob* pVSBlob = nullptr;
		hr = CompileShaderFromFile(L"Shaders.hlsl", "VS", "vs_4_0", &pVSBlob);
		CHECK(hr);

		hr = mDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &mVertexShader);
		if (FAILED(hr))
		{
			pVSBlob->Release();
			return false;
		}

		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		hr = mDevice->CreateInputLayout(layout, ARRAYSIZE(layout),
			pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &mVertexLayout);

		pVSBlob->Release();
		CHECK(hr);

	}

	// 5. Create the pixel shader
	{
		ID3DBlob* pPSBlob = nullptr;
		hr = CompileShaderFromFile(L"Shaders.hlsl", "PS", "ps_4_0", &pPSBlob);
		CHECK(hr);

		hr = mDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &mPixelShader);
		pPSBlob->Release();
		CHECK(hr);
	}

	// 6. Create the constant buffer
	{
		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.ByteWidth = sizeof(Vec4);
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		hr = mDevice->CreateBuffer(&desc, nullptr, &mColorBuffer);
		CHECK(hr);

		hr = mDevice->CreateBuffer(&desc, nullptr, &mOffsetBuffer);
		CHECK(hr);
	}

	// 7. Create the geometry
	{
		XMFLOAT3 vertices[6];
		vertices[0] = { -1.f, 1.f, 0.f };
		vertices[1] = { -1.f + CELL_WIDTH, 1.f - CELL_HEIGHT, 0.f };
		vertices[2] = { -1.f, 1.f - CELL_HEIGHT, 0.f };
		vertices[5] = { -1.f, 1.f, 0.f };
		vertices[4] = { -1.f + CELL_WIDTH, 1.f - CELL_HEIGHT, 0.f };
		vertices[3] = { -1.f + CELL_WIDTH, 1.f, 0.f };

		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.ByteWidth = sizeof(XMFLOAT3) * 6;

		D3D11_SUBRESOURCE_DATA data;
		ZeroMemory(&data, sizeof(data));
		data.pSysMem = vertices;

		mDevice->CreateBuffer(&desc, &data, &mVertexBuffer);
	}

#undef CHECK

	// 8. Set render environment
	{
		mContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);
		mContext->ClearRenderTargetView(mRenderTargetView, BG_COLOR);
		mSwapChain->Present(1, 0);

		// IA
		const uint32 stride = sizeof(Vec3);
		const uint32 offset = 0;
		mContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);
		mContext->IASetInputLayout(mVertexLayout);
		mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// VS
		mContext->VSSetShader(mVertexShader, nullptr, 0);
		mContext->VSSetConstantBuffers(0, 1, &mOffsetBuffer);

		// PS
		mContext->PSSetShader(mPixelShader, nullptr, 0);
		mContext->PSSetConstantBuffers(1, 1, &mColorBuffer);
	}

	return true;
}


eGameStatus Game::Update(eDirection desiredDirection)
{
	if (mCurrDir != OPPOSITES[static_cast<int>(desiredDirection)] || mSnakeLength == 1)
	{
		mCurrDir = desiredDirection;
	}

	int xMovement = X_MOVEMENTS[static_cast<int>(mCurrDir)];
	int yMovement = Y_MOVEMENTS[static_cast<int>(mCurrDir)];

	Pos desiredHeadPos = mSnakePosLog[mHeadIdx];
	desiredHeadPos.x += xMovement;
	desiredHeadPos.y += yMovement;

	if ((desiredHeadPos.x < 0 || desiredHeadPos.x >= mNumCellsPerRow)
		|| (desiredHeadPos.y < 0 || desiredHeadPos.y >= mNumCellsPerCol)
		|| getCellInfoAt(desiredHeadPos) == eCellInfo::SNAKE)
	{
		return eGameStatus::DEAD;
	}

	eMoveResult currentMoveResult = eMoveResult::JUST_MOVE;

	if (getCellInfoAt(desiredHeadPos) == eCellInfo::FOOD)
	{
		setCellInfoAt(desiredHeadPos, eCellInfo::SNAKE);
		currentMoveResult = eMoveResult::ATE_FOOD;

		generateFoodAtRandomPos();
	}

	setCellInfoAt(desiredHeadPos, eCellInfo::SNAKE);

	mHeadIdx = (mHeadIdx + 1) % mMaxSnakeLength;
	mSnakePosLog[mHeadIdx] = desiredHeadPos;

	if (mPrevMoveResult == eMoveResult::ATE_FOOD)
	{
		mPrevMoveResult = eMoveResult::JUST_MOVE;
		mPosToEraseOrNull = nullptr;

		++mSnakeLength;
	}
	else
	{
		uint32 tailIdx = (mHeadIdx - mSnakeLength + mMaxSnakeLength) % mMaxSnakeLength;
		mPosToEraseOrNull = mSnakePosLog + tailIdx;

		setCellInfoAt(*mPosToEraseOrNull, eCellInfo::EMPTY);
	}

	mPrevMoveResult = currentMoveResult;

	return eGameStatus::ALIVE;
}

void Game::Render()
{
	Vec4 screenOffset;

#define DRAW() mContext->Draw(6,0)

	ASSERT(getCellInfoAt(mFoodPos) == eCellInfo::FOOD);

	screenOffset = ConvertPosToScreenOffset(mFoodPos);
	setRenderingColor(FOOD_COLOR);
	setRenderingPos(&screenOffset);
	DRAW();

	if (mPosToEraseOrNull)
	{
		ASSERT(getCellInfoAt(*mPosToEraseOrNull) == eCellInfo::EMPTY);

		screenOffset = ConvertPosToScreenOffset(*mPosToEraseOrNull);
		setRenderingColor(BG_COLOR);
		setRenderingPos(&screenOffset);
		DRAW();
	}

	ASSERT(getCellInfoAt(*(mSnakePosLog + mHeadIdx)) == eCellInfo::SNAKE);

	screenOffset = ConvertPosToScreenOffset(*(mSnakePosLog + mHeadIdx));
	setRenderingColor(SNAKE_COLOR);
	setRenderingPos(&screenOffset);
	DRAW();

#undef DRAW()

	mSwapChain->Present(1, 0);
}

void Game::setRenderingColor(const float* color)
{
	D3D11_MAPPED_SUBRESOURCE subres = { 0, };

	mContext->Map(mColorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subres);
	memcpy(subres.pData, color, sizeof(float) * 4);
	mContext->Unmap(mColorBuffer, 0);

}

void Game::setRenderingPos(Vec4* screenPos)
{
	D3D11_MAPPED_SUBRESOURCE subres = { 0, };

	mContext->Map(mOffsetBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subres);
	memcpy(subres.pData, screenPos, sizeof(Vec4));
	mContext->Unmap(mOffsetBuffer, 0);
}

void Game::generateFoodAtRandomPos(void)
{

	do
	{
		mFoodPos = Pos(rand() % mNumCellsPerRow, rand() % mNumCellsPerRow);
	} while (getCellInfoAt(mFoodPos) == eCellInfo::SNAKE);
	
		
	setCellInfoAt(mFoodPos, eCellInfo::FOOD);
}

Game::~Game()
{
	if (mContext)			mContext->ClearState();
	if (mColorBuffer)		mColorBuffer->Release();
	if (mOffsetBuffer)	mOffsetBuffer->Release();
	if (mVertexBuffer)		mVertexBuffer->Release();
	if (mPixelShader)		mPixelShader->Release();
	if (mVertexLayout)		mVertexLayout->Release();
	if (mVertexShader)		mVertexShader->Release();
	if (mRenderTargetView)	mRenderTargetView->Release();
	if (mSwapChain)			mSwapChain->Release();
	if (mContext)			mContext->Release();
	if (mDevice)			mDevice->Release();

	free(mSnakePosLog);
	free(mCellInfos);
}

uint32 Game::GetSnakeLength() const
{
	return mSnakeLength;
}

eCellInfo Game::getCellInfoAt(Pos pos) const
{
	return mCellInfos[pos.y * NUM_CELLS_PER_ROW + pos.x];
}

void Game::setCellInfoAt(Pos pos, eCellInfo cellInfo)
{
	mCellInfos[pos.y * NUM_CELLS_PER_ROW + pos.x] = cellInfo;
}

static inline Vec4 ConvertPosToScreenOffset(Pos pos)
{
	float convertedX = (pos.x * CELL_WIDTH);
	float convertedY = -(pos.y * CELL_HEIGHT);

	return Vec4(convertedX, convertedY, 0.f, 0.f);
}

static HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}
