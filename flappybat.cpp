#include <raylib.h>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <ctime>
// Window setup
const int SCREEN_WIDTH = 400;
const int SCREEN_HEIGHT = 500;
// Physics parameters
const float GRAVITY = 0.35f;
const float JUMP_STRENGTH = -6.5f;
const float PIPE_SPEED = 2.5f;
const int PIPE_WIDTH = 55;
const int PIPE_GAP = 145;
// Bat pixels configuration
const int PIXEL_SIZE = 3;
const int BAT_COLS = 8;
const int BAT_ROWS = 7;
const int BAT_WIDTH = BAT_COLS * PIXEL_SIZE;   // 24
const int BAT_HEIGHT = BAT_ROWS * PIXEL_SIZE;  // 21
// Bat animation frames
const char* bat_frame_1[BAT_ROWS] = {
    "10000001",
    "11000011",
    "11111111",
    "11011011",
    "01111110",
    "00111100",
    "00111100"
};
const char* bat_frame_2[BAT_ROWS] = {
    "00111100",
    "01111110",
    "11111111",
    "11011011",
    "11111111",
    "01100110",
    "00111100"
};
const char* bat_frame_3[BAT_ROWS] = {
    "00111100",
    "00111100",
    "01111110",
    "11011011",
    "11111111",
    "11000011",
    "10000001"
};
// Particles setup
struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float alpha;
    float size;
    bool active;
};
const int MAX_PARTICLES = 120;
Particle particles[MAX_PARTICLES] = { 0 };
// Background stars
struct Star {
    Vector2 position;
    float size;
    float alpha;
    float twinkleSpeed;
};
const int STAR_COUNT = 30;
Star stars[STAR_COUNT];
// Game states
enum GameState { STATE_MENU, STATE_PLAYING, STATE_GAMEOVER };
GameState gameState = STATE_MENU;
// High Score management
int highScore = 0;
void LoadHighScore() {
    std::ifstream file("highscore.txt");
    if (file.is_open()) {
        file >> highScore;
    }
}
void SaveHighScore() {
    std::ofstream file("highscore.txt");
    if (file.is_open()) {
        file << highScore;
    }
}
// Procedural Audio Generators
Sound GenerateFlapSound() {
    int sampleRate = 44100;
    float duration = 0.08f;
    int sampleCount = (int)(sampleRate * duration);
    short* samples = (short*)malloc(sampleCount * sizeof(short));   
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / sampleRate;
        float freq = 160.0f + 240.0f * (t / duration);
        float phase = 2.0f * PI * freq * t;
        float envelope = 1.0f - (t / duration);
        samples[i] = (short)(sinf(phase) * envelope * 8000.0f);
    }    
    Wave wave = { (unsigned int)sampleCount, (unsigned int)sampleRate, 16, 1, samples };
    Sound sound = LoadSoundFromWave(wave);
    free(samples);
    return sound;
}
Sound GeneratePointSound() {
    int sampleRate = 44100;
    float duration = 0.22f;
    int sampleCount = (int)(sampleRate * duration);
    short* samples = (short*)malloc(sampleCount * sizeof(short));    
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / sampleRate;
        float freq = 523.25f; // C5
        if (t > 0.08f) {
            freq = 659.25f; // E5
        }        
        float phase = 2.0f * PI * freq * t;
        float envelope = 1.0f;
        if (t <= 0.08f) {
            envelope = 1.0f - (t / 0.08f) * 0.3f;
        } else {
            envelope = 1.0f - ((t - 0.08f) / 0.14f);
        }
        
        samples[i] = (short)(sinf(phase) * envelope * 6000.0f);
    }    
    Wave wave = { (unsigned int)sampleCount, (unsigned int)sampleRate, 16, 1, samples };
    Sound sound = LoadSoundFromWave(wave);
    free(samples);
    return sound;
}
Sound GenerateDieSound() {
    int sampleRate = 44100;
    float duration = 0.40f;
    int sampleCount = (int)(sampleRate * duration);
    short* samples = (short*)malloc(sampleCount * sizeof(short));    
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / sampleRate;
        float freq = 340.0f - 260.0f * (t / duration); // downward sweep
        float phase = 2.0f * PI * freq * t;        
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float sineVal = sinf(phase);
        float blend = 0.6f * sineVal + 0.4f * noise;        
        float envelope = 1.0f - (t / duration);
        samples[i] = (short)(blend * envelope * 9000.0f);
    }    
    Wave wave = { (unsigned int)sampleCount, (unsigned int)sampleRate, 16, 1, samples };
    Sound sound = LoadSoundFromWave(wave);
    free(samples);
    return sound;
}
// Particle System Helpers
void SpawnParticles(Vector2 pos, Color color, int count) {
    for (int k = 0; k < count; k++) {
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!particles[i].active) {
                particles[i].position = pos;
                particles[i].velocity.x = -1.5f - ((float)(rand() % 15) / 10.0f);
                particles[i].velocity.y = ((float)(rand() % 30) - 15.0f) / 10.0f;
                particles[i].color = color;
                particles[i].alpha = 0.9f;
                particles[i].size = 2.0f + (float)(rand() % 3);
                particles[i].active = true;
                break;
            }
        }
    }
}
void UpdateParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].position.x += particles[i].velocity.x;
            particles[i].position.y += particles[i].velocity.y;
            particles[i].alpha -= 0.025f;
            if (particles[i].alpha <= 0.0f) {
                particles[i].active = false;
            }
        }
    }
}
void DrawParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            DrawCircleV(particles[i].position, particles[i].size, Fade(particles[i].color, particles[i].alpha));
        }
    }
}
void ClearParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
    }
}
// Star Background Initialization & Draw
void InitStars() {
    for (int i = 0; i < STAR_COUNT; i++) {
        stars[i].position.x = (float)(rand() % SCREEN_WIDTH);
        stars[i].position.y = (float)(rand() % SCREEN_HEIGHT);
        stars[i].size = 1.0f + (float)(rand() % 2);
        stars[i].alpha = (float)(rand() % 100) / 100.0f;
        stars[i].twinkleSpeed = 0.005f + ((float)(rand() % 10) / 1000.0f);
        if (rand() % 2 == 0) stars[i].twinkleSpeed = -stars[i].twinkleSpeed;
    }
}
void UpdateAndDrawStars() {
    for (int i = 0; i < STAR_COUNT; i++) {
        stars[i].alpha += stars[i].twinkleSpeed;
        if (stars[i].alpha >= 1.0f || stars[i].alpha <= 0.15f) {
            stars[i].twinkleSpeed = -stars[i].twinkleSpeed;
        }
        DrawCircleV(stars[i].position, stars[i].size, Fade(RAYWHITE, stars[i].alpha));
    }
}
// Drawing Helper
void DrawTextWithShadow(const char* text, int posX, int posY, int fontSize, Color color) {
    DrawText(text, posX + 2, posY + 2, fontSize, BLACK);
    DrawText(text, posX, posY, fontSize, color);
}
// Pipe Struct
struct Pipe {
    float x;
    float gapY;
    bool passed;
};
int main() {
    // Random seed
    srand(time(NULL));
    // Initialize Raylib Window and Audio
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Flappy Bat 🦇");
    InitAudioDevice();
    SetTargetFPS(60);
    // Load High Score
    LoadHighScore();
    // Generate Sounds
    Sound sfxFlap = GenerateFlapSound();
    Sound sfxPoint = GeneratePointSound();
    Sound sfxDie = GenerateDieSound();
    // Sound toggle state
    bool soundOn = true;
    bool paused = false;
    // Bat state
    float batX = 60.0f;
    float batY = 200.0f;
    float batVelocity = 0.0f;
    // Score pop animation scale
    float scorePopScale = 1.0f;
    // Pipes vector
    std::vector<Pipe> pipes;
    float pipeTimer = 0.0f;
    int score = 0;
    // Initialize stars
    InitStars();
    // Helper lambda to spawn a pipe
    auto SpawnPipe = [&]() {
        Pipe p;
        p.x = SCREEN_WIDTH + 10.0f;
        p.gapY = (float)(110 + rand() % 230); // Random height between 110 and 340
        p.passed = false;
        pipes.push_back(p);
    };
    // Helper lambda to reset game
    auto ResetGame = [&]() {
        batY = 200.0f;
        batVelocity = 0.0f;
        score = 0;
        scorePopScale = 1.0f;
        pipes.clear();
        ClearParticles();
        pipeTimer = 0.0f;
        paused = false;
        SpawnPipe();
    };
    // Initial pipe setup
    SpawnPipe();
    // Game loop
    while (!WindowShouldClose()) {
        // --- INPUT ---
        if (gameState == STATE_MENU) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                ResetGame();
                gameState = STATE_PLAYING;
            }
        } 
        else if (gameState == STATE_PLAYING) {
            if (IsKeyPressed(KEY_P)) {
                paused = !paused;
            }
            if (IsKeyPressed(KEY_M)) {
                soundOn = !soundOn;
            }
            if (!paused) {
                if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    batVelocity = JUMP_STRENGTH;
                    if (soundOn) PlaySound(sfxFlap);
                    // Spawn particles behind bat when flapping
                    SpawnParticles({ batX + 4, batY + BAT_HEIGHT / 2.0f }, Color{ 255, 100, 255, 255 }, 8);
                }
            }
        } 
        else if (gameState == STATE_GAMEOVER) {
            if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                ResetGame();
                gameState = STATE_PLAYING;
            }
        }
        // --- UPDATE ---
        if (gameState == STATE_PLAYING && !paused) {
            // Apply gravity
            batVelocity += GRAVITY;
            batY += batVelocity;
            // Spawn particles periodically as bat flies
            if (GetFrameTime() > 0.0f && (rand() % 4 == 0)) {
                SpawnParticles({ batX, batY + BAT_HEIGHT / 2.0f }, Color{ 150, 80, 255, 200 }, 1);
            }
            // Spawn pipes
            pipeTimer += 1.0f;
            if (pipeTimer >= 110.0f) { // roughly every 1.83 seconds
                SpawnPipe();
                pipeTimer = 0.0f;
            }
            // Move pipes & check score
            for (size_t i = 0; i < pipes.size(); i++) {
                pipes[i].x -= PIPE_SPEED;
                // Pass bat check for score
                if (!pipes[i].passed && (pipes[i].x + PIPE_WIDTH < batX)) {
                    pipes[i].passed = true;
                    score++;
                    scorePopScale = 1.5f;
                    if (soundOn) PlaySound(sfxPoint);
                }
            }
            // Remove off-screen pipes
            if (!pipes.empty() && pipes[0].x < -PIPE_WIDTH) {
                pipes.erase(pipes.begin());
            }
            // Handle score scale decay
            if (scorePopScale > 1.0f) {
                scorePopScale -= 0.04f;
                if (scorePopScale < 1.0f) scorePopScale = 1.0f;
            }
            // Collision check
            Rectangle batBox = { batX, batY, (float)BAT_WIDTH, (float)BAT_HEIGHT };
            bool collided = false;
            // Collision with boundaries
            if (batY <= 0 || batY + BAT_HEIGHT >= SCREEN_HEIGHT) {
                collided = true;
            }
            // Collision with pipes
            for (const auto& p : pipes) {
                Rectangle topPipe = { p.x, 0.0f, (float)PIPE_WIDTH, p.gapY - PIPE_GAP / 2.0f };
                Rectangle bottomPipe = { p.x, p.gapY + PIPE_GAP / 2.0f, (float)PIPE_WIDTH, (float)SCREEN_HEIGHT - (p.gapY + PIPE_GAP / 2.0f) };                
                // Add caps offset to make drawing and collision aligned
                if (CheckCollisionRecs(batBox, topPipe) || CheckCollisionRecs(batBox, bottomPipe)) {
                    collided = true;
                    break;
                }
            }
            if (collided) {
                if (soundOn) PlaySound(sfxDie);
                gameState = STATE_GAMEOVER;                
                // Save high score if necessary
                if (score > highScore) {
                    highScore = score;
                    SaveHighScore();
                }
            }
            // Update particles
            UpdateParticles();
        }
        // --- DRAW ---
        BeginDrawing();        
        // Vertical Indigo Gradient background
        DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{ 19, 15, 38, 255 }, Color{ 42, 26, 74, 255 });        
        // Stars
        UpdateAndDrawStars();
        // Draw particles
        DrawParticles();
        // Draw pipes
        for (const auto& p : pipes) {
            float topHeight = p.gapY - PIPE_GAP / 2.0f;
            float bottomY = p.gapY + PIPE_GAP / 2.0f;
            float bottomHeight = SCREEN_HEIGHT - bottomY;
            Color pipeBody = { 20, 15, 35, 230 };
            Color pipeBorder = { 0, 220, 255, 255 }; // Neon Cyan
            // Top Pipe Body
            DrawRectangleRec({ p.x, 0.0f, (float)PIPE_WIDTH, topHeight }, pipeBody);
            DrawRectangleLinesEx({ p.x, 0.0f, (float)PIPE_WIDTH, topHeight }, 2.0f, pipeBorder);
            // Bottom Pipe Body
            DrawRectangleRec({ p.x, bottomY, (float)PIPE_WIDTH, bottomHeight }, pipeBody);
            DrawRectangleLinesEx({ p.x, bottomY, (float)PIPE_WIDTH, bottomHeight }, 2.0f, pipeBorder);
            // Draw caps (wider rim at the end of the pipes)
            int capHeight = 16;
            int capOffset = 4;            
            // Top Cap (at the bottom end of top pipe)
            DrawRectangleRec({ p.x - capOffset, topHeight - capHeight, (float)(PIPE_WIDTH + capOffset * 2), (float)capHeight }, pipeBody);
            DrawRectangleLinesEx({ p.x - capOffset, topHeight - capHeight, (float)(PIPE_WIDTH + capOffset * 2), (float)capHeight }, 2.0f, pipeBorder);
            // Bottom Cap (at the top end of bottom pipe)
            DrawRectangleRec({ p.x - capOffset, bottomY, (float)(PIPE_WIDTH + capOffset * 2), (float)capHeight }, pipeBody);
            DrawRectangleLinesEx({ p.x - capOffset, bottomY, (float)(PIPE_WIDTH + capOffset * 2), (float)capHeight }, 2.0f, pipeBorder);
        }
        // Draw Bat
        if (gameState == STATE_PLAYING || gameState == STATE_GAMEOVER) {
            // Select frame based on vertical speed
            const char** currentFrame = bat_frame_2;
            if (batVelocity < -1.5f) {
                currentFrame = bat_frame_1; // Rising wings up
            } else if (batVelocity > 1.5f) {
                currentFrame = bat_frame_3; // Falling wings down
            }
            // Draw pixel grid
            for (int r = 0; r < BAT_ROWS; r++) {
                for (int c = 0; c < BAT_COLS; c++) {
                    if (currentFrame[r][c] == '1') {
                        DrawRectangle((int)batX + c * PIXEL_SIZE, (int)batY + r * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE, Color{ 230, 220, 255, 255 });
                    }
                    // Glowing pink/red eyes on Row 3, Cols 2 & 5
                    if (r == 3 && (c == 2 || c == 5)) {
                        DrawRectangle((int)batX + c * PIXEL_SIZE, (int)batY + r * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE, Color{ 255, 50, 120, 255 });
                    }
                }
            }
        }
        // Draw Score (UI)
        if (gameState == STATE_PLAYING || gameState == STATE_GAMEOVER) {
            int dynamicFontSize = (int)(24 * scorePopScale);
            std::string scoreText = "SCORE: " + std::to_string(score);
            int textWidth = MeasureText(scoreText.c_str(), dynamicFontSize);
            DrawTextWithShadow(scoreText.c_str(), SCREEN_WIDTH / 2 - textWidth / 2, 40 - (dynamicFontSize - 24) / 2, dynamicFontSize, Color{ 255, 240, 100, 255 });
        }
        // Draw States Overlays
        if (gameState == STATE_MENU) {
            // Title
            int titleWidth = MeasureText("FLAPPY BAT", 38);
            DrawTextWithShadow("FLAPPY BAT 🦇", SCREEN_WIDTH / 2 - titleWidth / 2 - 12, 130, 38, Color{ 255, 50, 150, 255 });
            // High Score
            std::string hsText = "HIGH SCORE: " + std::to_string(highScore);
            int hsWidth = MeasureText(hsText.c_str(), 18);
            DrawTextWithShadow(hsText.c_str(), SCREEN_WIDTH / 2 - hsWidth / 2, 210, 18, Color{ 200, 200, 200, 255 });
            // Pulsing "Press Enter" text
            float pulse = sinf(GetTime() * 5.0f);
            Color enterColor = pulse > 0.0f ? Color{ 0, 255, 240, 255 } : Color{ 0, 180, 200, 255 };
            int enterWidth = MeasureText("PRESS ENTER TO START", 18);
            DrawTextWithShadow("PRESS ENTER TO START", SCREEN_WIDTH / 2 - enterWidth / 2, 280, 18, enterColor);
            int instrWidth = MeasureText("SPACE / CLICK TO JUMP", 14);
            DrawTextWithShadow("SPACE / CLICK TO JUMP", SCREEN_WIDTH / 2 - instrWidth / 2, 320, 14, GRAY);            
            // Audio info icon
            DrawText("M: TOGGLE SOUND", 20, SCREEN_HEIGHT - 35, 12, DARKGRAY);
            DrawText("P: PAUSE", SCREEN_WIDTH - 90, SCREEN_HEIGHT - 35, 12, DARKGRAY);
        }
        else if (gameState == STATE_PLAYING) {
            // Mute / Pause subtle indicator
            if (!soundOn) {
                DrawText("MUTED", 15, SCREEN_HEIGHT - 25, 12, RED);
            } else {
                DrawText("SOUND ON", 15, SCREEN_HEIGHT - 25, 12, DARKGRAY);
            }
            // Pause Menu Overlay
            if (paused) {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{ 15, 10, 25, 180 }); // Glassmorphic darken
                int pauseWidth = MeasureText("PAUSED", 30);
                DrawTextWithShadow("PAUSED", SCREEN_WIDTH / 2 - pauseWidth / 2, SCREEN_HEIGHT / 2 - 40, 30, Color{ 255, 50, 150, 255 });
                int resumeWidth = MeasureText("Press P to Resume", 16);
                DrawTextWithShadow("Press P to Resume", SCREEN_WIDTH / 2 - resumeWidth / 2, SCREEN_HEIGHT / 2 + 10, 16, RAYWHITE);
            }
        }
        else if (gameState == STATE_GAMEOVER) {
            // Glassmorphic background darken
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{ 20, 10, 30, 200 });
            int goWidth = MeasureText("GAME OVER", 36);
            DrawTextWithShadow("GAME OVER 🦇", SCREEN_WIDTH / 2 - goWidth / 2 - 10, 140, 36, Color{ 255, 50, 100, 255 });
            std::string finalScore = "SCORE: " + std::to_string(score);
            int scoreWidth = MeasureText(finalScore.c_str(), 24);
            DrawTextWithShadow(finalScore.c_str(), SCREEN_WIDTH / 2 - scoreWidth / 2, 210, 24, Color{ 255, 240, 100, 255 });
            std::string hsText = "HIGH SCORE: " + std::to_string(highScore);
            int hsWidth = MeasureText(hsText.c_str(), 18);
            DrawTextWithShadow(hsText.c_str(), SCREEN_WIDTH / 2 - hsWidth / 2, 250, 18, Color{ 200, 200, 200, 255 });
            float pulse = sinf(GetTime() * 5.0f);
            Color restartColor = pulse > 0.0f ? Color{ 0, 255, 240, 255 } : Color{ 0, 180, 200, 255 };
            int restartWidth = MeasureText("PRESS R TO RESTART", 18);
            DrawTextWithShadow("PRESS R TO RESTART", SCREEN_WIDTH / 2 - restartWidth / 2, 320, 18, restartColor);
        }
        EndDrawing();
    }
    // Unload assets
    UnloadSound(sfxFlap);
    UnloadSound(sfxPoint);
    UnloadSound(sfxDie);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}