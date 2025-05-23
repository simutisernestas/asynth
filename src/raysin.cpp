#ifndef RAYSIN_IMPLEMENTATION
#define RAYSIN_IMPLEMENTATION

#include <raylib.h>
#include <cmath>
#include <vector>
#include <array>
#include <thread>
#include <map>
#include <atomic>
#include <algorithm>
#include "constants.hpp"

class RaySin
{
public:
    RaySin(int initialScreenWidth = 800, int initialScreenHeight = 600) // Renamed params
        : initialScreenWidth(initialScreenWidth), initialScreenHeight(initialScreenHeight) // Store initial values
    {
        // screenWidth and screenHeight will be set in appLoop by updateLayout
        // centerY, centerX etc. will be set in updateLayout
        zoomLevel = 1.0f;
        numDivisions = 10;
        // waveAmplitude will be calculated in updateLayout
        waveFrequency = 2.1f;
        lineThickness = 2;
        // visibleWidth, offset, waveStep, diff will be calculated in updateLayout
        displayTimeWindowMs = 200.0f; // Initialize new member
        letters = "ABCDEFGHIJ";
        key2rect = {{KEY_Z, 0}, {KEY_X, 1}, {KEY_C, 2}, {KEY_V, 3},
                    {KEY_B, 4}, {KEY_N, 5}, {KEY_M, 6}};
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
    int initialScreenWidth; // To store constructor values for first InitWindow
    int initialScreenHeight; // To store constructor values for first InitWindow
    int screenWidth;      // Current screen width
    int screenHeight;     // Current screen height
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
    float displayTimeWindowMs; // New member variable
    std::array<float, SAMPLE_RATE>* currentBuffer;
    std::vector<Vector2> threadsData;
    std::thread thread;
    std::atomic_bool running;

    void updateLayout()
    {
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        centerY = screenHeight / 2;
        centerX = screenWidth / 2;
        startX = 0; // Assuming startX is always 0, adjust if it can change
        endX = screenWidth;
        waveAmplitude = (screenHeight * 0.3f) / 2; // Adjusted to use current screenHeight
        visibleWidth = screenWidth / zoomLevel;
        offset = startX / zoomLevel; // Assuming startX is 0, so offset is 0 unless zoom changes how startX is perceived
        waveStep = .1f / zoomLevel; // waveStep seems to be independent of screen size, only zoom.
        diff = static_cast<int>(visibleWidth / waveStep); // Recalculate based on new visibleWidth
    }

    void appLoop()
    {
        // SetConfigFlags should be called *before* InitWindow
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(initialScreenWidth, initialScreenHeight, "APLay"); // Use initial values for the first window creation
        ClearBackground(BLACK);
        SetTargetFPS(60);

        updateLayout(); // Initial layout calculation after window is created and configured

        while (!WindowShouldClose() && running.load())
        {
            if (IsWindowResized())
            {
                updateLayout();
            }
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

        if (IsKeyDown(KEY_UP))
        {
            displayTimeWindowMs -= 10.0f;
        }
        if (IsKeyDown(KEY_DOWN))
        {
            displayTimeWindowMs += 10.0f;
        }
        displayTimeWindowMs = std::max(10.0f, std::min(displayTimeWindowMs, 1000.0f)); // Clamp value

    }

    void draw()
    {
        BeginDrawing();
        ClearBackground(BLACK);

        DrawLineEx({static_cast<float>(centerX), 0},
                   {static_cast<float>(centerX), static_cast<float>(screenHeight - 50)}, // Use screenHeight
                   lineThickness, Fade(LIME, 0.3f));
        DrawText("Amplitude", centerX + 10, 10, 20, WHITE);
        DrawText(TextFormat("Time Window: %.0f ms", displayTimeWindowMs), centerX + 10, 40, 20, WHITE); // Display current time window

        if (currentBuffer != nullptr && SAMPLE_RATE > 1)
        {
            int samplesToDisplay = static_cast<int>((displayTimeWindowMs / 1000.0f) * SAMPLE_RATE);
            samplesToDisplay = std::min(samplesToDisplay, static_cast<int>(SAMPLE_RATE));
            samplesToDisplay = std::max(samplesToDisplay, 2); // Need at least 2 samples to draw a line

            if (samplesToDisplay > 1) // Ensure we have at least two points to draw lines
            {
                for (int i = 0; i < samplesToDisplay - 1; i++)
                {
                    // Ensure i and i+1 are within bounds of currentBuffer, which is guaranteed by samplesToDisplay <= SAMPLE_RATE
                    float x0 = static_cast<float>(i) / (samplesToDisplay - 1) * screenWidth;
                    float y0 = centerY + (*currentBuffer)[i] * waveAmplitude;
                    float x1 = static_cast<float>(i + 1) / (samplesToDisplay - 1) * screenWidth;
                    float y1 = centerY + (*currentBuffer)[i + 1] * waveAmplitude;
                    DrawLineEx({x0, y0}, {x1, y1}, 2, LIME);
                }
            }
        }

        DrawLineEx({static_cast<float>(startX), static_cast<float>(centerY)},
                   {startX + visibleWidth, static_cast<float>(centerY)}, lineThickness,
                   Fade(LIME, 0.3f));
        DrawText("Time [s]", screenWidth - 80, centerY - 25, 20, WHITE); // Use screenWidth

        const int NUM_RECTANGLES = key2rect.size();
        const int RECTANGLE_WIDTH = screenWidth / NUM_RECTANGLES; // Use screenWidth
        const int RECTANGLE_HEIGHT = 50;
        const int RECTANGLE_Y = screenHeight - RECTANGLE_HEIGHT; // Use screenHeight
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

    std::map<int, int> key2rect;
};

#endif
