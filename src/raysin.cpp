#ifndef RAYSIN_IMPLEMENTATION
#define RAYSIN_IMPLEMENTATION

#include <raylib.h>
#include <cmath>
#include <vector>
#include <array>
#include <thread>
#include <atomic>
#include <algorithm>
#include "constants.hpp"

class RaySin
{
public:
    RaySin(int screenWidth = 800, int screenHeight = 600)
        : SCREEN_WIDTH(screenWidth), SCREEN_HEIGHT(screenHeight)
    {
        centerY = SCREEN_HEIGHT / 2;
        centerX = SCREEN_WIDTH / 2;
        startX = 0;
        endX = SCREEN_WIDTH;
        zoomLevel = 1.0f;
        numDivisions = 10;
        waveAmplitude = (SCREEN_HEIGHT * 0.3f) / 2;
        waveFrequency = 2.1f;
        lineThickness = 2;
        visibleWidth = SCREEN_WIDTH / zoomLevel;
        offset = startX / zoomLevel;
        waveStep = .1 / zoomLevel;
        diff = static_cast<int>(visibleWidth / waveStep);
        letters = "ABCDEFGHIJ";
    }

    void start()
    {
        running.store(true);
        thread = std::thread(&RaySin::appLoop, this);
    }

    void stop()
    {
        running.store(false);
        thread.join();
    }

    void setCurrentBuffer(std::array<float, SAMPLE_RATE>* buffer) { currentBuffer = buffer; }

    void getPressedKeys(std::vector<int>& keys_pressed)
    {
        for (int i = 0; i < 256; i++)
        {
            if (IsKeyDown(i))
                keys_pressed.push_back(i);
        }
    }

private:
    const int SCREEN_WIDTH;
    const int SCREEN_HEIGHT;
    int centerY;
    int centerX;
    int startX;
    int endX;
    float zoomLevel;
    int numDivisions;
    float waveAmplitude;
    float waveFrequency;
    int lineThickness;
    float visibleWidth;
    float offset;
    float waveStep;
    int diff;
    const char* letters;
    std::array<float, SAMPLE_RATE>* currentBuffer;
    std::vector<Vector2> threadsData;
    std::thread thread;
    std::atomic_bool running;

    void appLoop()
    {
        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "APLay");
        ClearBackground(BLACK);
        SetTargetFPS(60);

        while (!WindowShouldClose() && running.load())
        {
            handleInput();
            draw();
        }

        // check if window is still open
        if (IsWindowReady())
            CloseWindow();
    }

    void handleInput()
    {
        int wheelValue = GetMouseWheelMove();
        if (wheelValue > 0)
            zoomLevel *= 0.95f;
        else if (wheelValue < 0)
            zoomLevel *= 1.05f;

        if (IsKeyDown(KEY_Q))
        {
            running.store(false);
            CloseWindow();
        }

        if (zoomLevel > 1.0f)
            zoomLevel = 1.0f;
    }

    void draw()
    {
        BeginDrawing();
        ClearBackground(BLACK);

        DrawLineEx({static_cast<float>(centerX), 0},
                   {static_cast<float>(centerX), static_cast<float>(SCREEN_HEIGHT - 50)},
                   lineThickness, Fade(LIME, 0.3f));
        DrawText("Amplitude", centerX + 10, 10, 20, WHITE);

        for (int i = 0; i < SAMPLE_RATE - 1; i++)
        {
            float x0 = static_cast<float>(i) / SAMPLE_RATE * SCREEN_WIDTH;
            float y0 = centerY + (*currentBuffer)[i] * waveAmplitude;
            float x1 = static_cast<float>(i + 1) / SAMPLE_RATE * SCREEN_WIDTH;
            float y1 = centerY + (*currentBuffer)[i + 1] * waveAmplitude;
            DrawLineEx({x0, y0}, {x1, y1}, 1, BLUE);
        }

        DrawLineEx({static_cast<float>(startX), static_cast<float>(centerY)},
                   {startX + visibleWidth, static_cast<float>(centerY)}, lineThickness,
                   Fade(LIME, 0.3f));
        DrawText("Time [s]", SCREEN_WIDTH - 80, centerY - 25, 20, WHITE);

        const int NUM_RECTANGLES = key2rect.size();
        const int RECTANGLE_WIDTH = SCREEN_WIDTH / NUM_RECTANGLES;
        const int RECTANGLE_HEIGHT = 50;
        const int RECTANGLE_Y = SCREEN_HEIGHT - RECTANGLE_HEIGHT;
        const char* letters = "ABCDEFGHIJ";

        for (auto& [key, rect] : key2rect)
        {
            if (IsKeyDown(key))
                DrawRectangle(rect * RECTANGLE_WIDTH, RECTANGLE_Y, RECTANGLE_WIDTH,
                              RECTANGLE_HEIGHT, Fade(LIME, 0.3f));

            int rectangleX = rect * RECTANGLE_WIDTH;
            DrawRectangleLines(rectangleX, RECTANGLE_Y, RECTANGLE_WIDTH, RECTANGLE_HEIGHT,
                               Fade(LIME, 0.3f));
            const char* the_letter = TextFormat("%c", letters[rect]);
            DrawTextEx(GetFontDefault(), the_letter,
                       {static_cast<float>(rectangleX) + RECTANGLE_WIDTH / 2 - 10,
                        static_cast<float>(RECTANGLE_Y) + RECTANGLE_HEIGHT / 2 - 10},
                       20, 1, WHITE);
        }

        EndDrawing();
    }

    std::map<int, int> key2rect = {{KEY_Z, 0}, {KEY_X, 1}, {KEY_C, 2}, {KEY_V, 3},
                                   {KEY_B, 4}, {KEY_N, 5}, {KEY_M, 6}};
};

#endif
