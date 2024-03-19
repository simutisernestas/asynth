#include <raylib.h>
#include <cmath>
#include <vector>

const int SCREEN_WIDTH = 600 * 2;
const int SCREEN_HEIGHT = 420 * 2;

int main()
{
    // Initialize Raylib with black background
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "APLay");
    ClearBackground(BLACK);

    int centerY = SCREEN_HEIGHT / 2;
    int centerX = SCREEN_WIDTH / 2;

    // Axis variables - start and end positions for scaling
    int startX = 0;
    int endX = SCREEN_WIDTH;
    float zoomLevel = 1.0f; // Adjust zoom level here (start at 1.0 for no zoom)

    SetTargetFPS(60);

    // make transparent LIME
    Color lime = LIME;
    lime.a = 64;
    const int line_thickness = 2;

    // Calculate sine wave points
    float waveAmplitude = (SCREEN_HEIGHT * 0.8f) / 2;
    float waveFrequency = 2.1f;

    // Time axis variables
    float secondsPerDivision = 0.5f; // Adjust this for desired units
    const int numDivisions = 10;     // Number of divisions to display

    while (!WindowShouldClose())
    {
        // Handle zoom with mouse wheel
        int wheelValue = GetMouseWheelMove();
        if (wheelValue > 0)
            zoomLevel *= 0.95f; // Zoom in
        else if (wheelValue < 0)
            zoomLevel *= 1.05f; // Zoom out

        // exit on Q key
        if (IsKeyDown(KEY_Q))
            break;

        // max out zoom level at 1.0f
        if (zoomLevel > 1.0f)
            zoomLevel = 1.0f;

        // change secondsPerDivision based on zoom level
        secondsPerDivision = 0.5f * zoomLevel;

        // Calculate visible portion of the sine wave based on zoom and scroll
        float visibleWidth = SCREEN_WIDTH / zoomLevel;
        float offset = startX / zoomLevel;

        // Begin drawing
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw Y-axis (spanning entire height)
        DrawLineEx({static_cast<float>(centerX), 0}, {static_cast<float>(centerX), SCREEN_HEIGHT},
                   line_thickness, lime);
        DrawText("Amplitude", centerX + 10, 10, 20, WHITE);

        waveAmplitude = waveAmplitude + GetRandomValue(-20, 20);
        // Draw Sine Wave with Smoothing
        float waveStep = .1 / zoomLevel; // Adjust to control smoothness
        int diff = static_cast<int>(visibleWidth / waveStep);

        std::vector<Vector2> threadsData;
        threadsData.resize(diff);
#pragma omp parallel for
        for (int x = 0; x < diff; x++)
        {
            float angle = ((x - offset) / visibleWidth) * waveFrequency * PI * 2.0f;
            int y = centerY + sin(angle) * waveAmplitude;
            threadsData[x] = (Vector2){static_cast<float>(x), static_cast<float>(y)};
        }
        for (auto& point : threadsData)
            DrawPixelV(point, BLUE);

        // Draw X-axis (spanning visible range)
        DrawLineEx({static_cast<float>(startX), static_cast<float>(centerY)},
                   {startX + visibleWidth, static_cast<float>(centerY)}, line_thickness, lime);
        DrawText("Time [s]", SCREEN_WIDTH - 80, centerY - 25, 20, WHITE);

        // Draw Time Labels (adjust positioning based on offset)
        // float divisionWidth = visibleWidth / numDivisions;
        // for (int i = 1; i <= numDivisions - 1; i++)
        // {
        //     float x = centerX - (visibleWidth / 2.0f) + i * divisionWidth;
        //     float timeValue = (i - (numDivisions / 2.0f)) * secondsPerDivision;
        //     if (timeValue < 0.0f)
        //         timeValue *= -1.0f;
        //     DrawText(TextFormat("%.1f", timeValue), x - 10, centerY + 10, 20, WHITE);
        //     if (timeValue == 0.0f)
        //         continue;
        //     DrawLineEx({x, centerY - 5}, {x, centerY + 5}, line_thickness, lime);
        // }

        // Display zoom level for reference
        DrawText(TextFormat("Zoom: %.2f", zoomLevel), 10, 10, 20, WHITE);

        // Add five rectangles side by side, spanning the width of the screen on the bottom,
        // each having a single bold letter in it

        const int NUM_RECTANGLES = 10;
        const int RECTANGLE_WIDTH = SCREEN_WIDTH / NUM_RECTANGLES;
        const int RECTANGLE_HEIGHT = 50;
        const int RECTANGLE_Y = SCREEN_HEIGHT - RECTANGLE_HEIGHT;
        const char* letters = "ABCDEFGHIJ";

        // if the key Z is pressed fill the background of rectangle A with the color RED
        if (IsKeyDown(KEY_Z))
        {
            DrawRectangle(0, RECTANGLE_Y, RECTANGLE_WIDTH, RECTANGLE_HEIGHT,
                          Fade(DARKPURPLE, 0.3f));
        }

        // Draw rectangles with letters
        for (int i = 0; i < NUM_RECTANGLES; i++)
        {
            int rectangleX = i * RECTANGLE_WIDTH;
            // DrawRectangle(rectangleX, RECTANGLE_Y, RECTANGLE_WIDTH, RECTANGLE_HEIGHT,
            //               Fade(RED, 0.1f));
            DrawRectangleLines(rectangleX, RECTANGLE_Y, RECTANGLE_WIDTH, RECTANGLE_HEIGHT,
                               Fade(DARKPURPLE, 0.5f));
            const char* the_letter = TextFormat("%c", letters[i]);
            DrawTextEx(GetFontDefault(), the_letter,
                       {static_cast<float>(rectangleX) + RECTANGLE_WIDTH / 2 - 10,
                        RECTANGLE_Y + RECTANGLE_HEIGHT / 2 - 10},
                       20, 1, WHITE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
