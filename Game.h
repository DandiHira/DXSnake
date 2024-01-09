#pragma once
#include "Struct.h"
#include "enums.h"


class Game final
{
public:
	Game(uint32 numCellsPerRow, uint32 numCellsPerCol);
	bool InitD3D(HWND hWnd);

	eGameStatus Update(eDirection dir);
	void Render();
	~Game();

	uint32 GetSnakeLength() const;

	Game& operator=(Game& rhs) = delete;
	Game(Game& other) = delete;

private:
	// Game Logic

	Pos mFoodPos = Pos(0,0);
	const int mNumCellsPerRow;
	const int mNumCellsPerCol;

	uint32 mSnakeLength;
	const uint32 mMaxSnakeLength;

	// circular queue to log snake position
	Pos* const mSnakePosLog;
	uint32 mHeadIdx;

	eCellInfo* mCellInfos;
	eDirection mCurrDir;
	eMoveResult mPrevMoveResult;

	// Render

	ID3D11Device* mDevice = nullptr;
	ID3D11DeviceContext* mContext = nullptr;
	IDXGISwapChain* mSwapChain = nullptr;
	ID3D11RenderTargetView* mRenderTargetView = nullptr;
	ID3D11VertexShader* mVertexShader = nullptr;
	ID3D11InputLayout* mVertexLayout = nullptr;
	ID3D11PixelShader* mPixelShader = nullptr;
	ID3D11Buffer* mVertexBuffer = nullptr;
	ID3D11Buffer* mOffsetBuffer = nullptr;
	ID3D11Buffer* mColorBuffer = nullptr;

	Pos* mPosToEraseOrNull;

	// Helper

	eCellInfo getCellInfoAt(Pos pos) const;
	void setCellInfoAt(Pos pos, eCellInfo cellInfo);

	void setRenderingColor(const float* color);
	void setRenderingPos(Vec4* screenPos);
	void generateFoodAtRandomPos(void);
};

