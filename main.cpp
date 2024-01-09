
#include "pch.h"
#include "Game.h"
#include "GameTimer.h"
#include "enums.h"

#define MAX_LOADSTRING 100

HINSTANCE ghInst;                         
HWND ghWnd;
WCHAR gWindowTitle[MAX_LOADSTRING] = L"DXSnake";         
WCHAR gWindowClass[MAX_LOADSTRING] = L"DXSnakeClass";

eDirection gDirection = eDirection::RIGHT;
bool gbPaused = false;

bool InitWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    srand((unsigned int)time(NULL));

    if (!InitWindow(hInstance, nCmdShow))
    {
        return -1;
    }

    Game game(NUM_CELLS_PER_ROW, NUM_CELLS_PER_COL);
    if (!game.InitD3D(ghWnd))
    {
        return -1;
    }
    game.Render();

    MSG msg = { 0, };
    GameTimer timer;

    Second elaspedSinceLastMove = 0.f;
    Second moveInterval = .7f;

    eGameStatus gameStatus;

    timer.Start();
    while (msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        timer.Lap();

        if (gbPaused)
        {
            continue;
        }
        
        elaspedSinceLastMove += timer.GetLapTime();
        if (elaspedSinceLastMove > moveInterval)
        {
            elaspedSinceLastMove = 0.f;

            gameStatus = game.Update(gDirection);
            game.Render();

            if (gameStatus == eGameStatus::DEAD)
            {
                DestroyWindow(ghWnd);
            }
        }
    }

    return (int) msg.wParam;
}

bool InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    bool res = false;

    WNDCLASSEXW wc;

    wc.cbSize = sizeof(WNDCLASSEX);

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = nullptr;
    wc.hCursor = nullptr;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = gWindowClass;
    wc.hIconSm = nullptr;

    if (!RegisterClassExW(&wc))
    {
        ASSERT(false, "failed to register window class");
        return false;
    }

    ghInst = hInstance;

    DWORD style = WS_OVERLAPPED | WS_SYSMENU;

    RECT rc;
    rc.left = 0;
    rc.right = WINDOW_WIDTH;
    rc.top = 0;
    rc.bottom = WINDOW_HEIGHT;

    AdjustWindowRect(&rc, style, false);

    ghWnd = CreateWindowW(gWindowClass, gWindowTitle, style, CW_USEDEFAULT, 0,
        rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);

    if (!ghWnd)
    {
        ASSERT(0, "failed to create window");
        return false;
    }

    ShowWindow(ghWnd, nCmdShow);
    UpdateWindow(ghWnd);

    return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
        switch (wParam)
        {
        case 'W':
            gDirection = eDirection::UP;
            break;
        case 'A':
            gDirection = eDirection::LEFT;
            break;
        case 'S':
            gDirection = eDirection::DOWN;
            break;
        case 'D':
            gDirection = eDirection::RIGHT;
            break;
        case VK_SPACE:
            gbPaused ^= true;
            break;
        case VK_ESCAPE:
            DestroyWindow(hWnd);
            break;
        default:
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

