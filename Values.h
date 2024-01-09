#pragma once


enum
{
    WINDOW_HEIGHT = 600,
    WINDOW_WIDTH = 600,
    NUM_CELLS_PER_COL = 20,
    NUM_CELLS_PER_ROW = NUM_CELLS_PER_COL
};

const float LEFT_X = -1.f;
const float RIGHT_X = 1.f;
const float TOP_Y = 1.f;
const float BOTTOM_Y = -1.f;

const float BOARD_HEIGHT = TOP_Y - BOTTOM_Y;
const float BOARD_WIDTH = RIGHT_X - LEFT_X;

const float CELL_HEIGHT = BOARD_HEIGHT / NUM_CELLS_PER_COL;
const float CELL_WIDTH = BOARD_WIDTH / NUM_CELLS_PER_ROW;

const float BG_COLOR[] = { 0.3f, 0.3f, 0.6f, 1.f };
const float SNAKE_COLOR[] = { 0.7f, 0.7f, 0.3f, 1.f };
const float FOOD_COLOR[] = { 1.f, 0.5f, 0.5f, 1.f };