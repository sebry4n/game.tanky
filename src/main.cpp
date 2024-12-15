#include "raylib.h"
#include <cmath>
#include <ctime>
#include <vector>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1200
#define TERRAIN_SEGMENT_WIDTH 5 
#define TERRAIN_WIDTH (SCREEN_WIDTH / TERRAIN_SEGMENT_WIDTH)
#define MAX_HEIGHT 500
#define HEIGHT_DIFF 4

float RemapValue(float value, float inputMin, float inputMax, float outputMin, float outputMax) {
    return outputMin + (outputMax - outputMin) * ((value - inputMin) / (inputMax - inputMin));
}

struct Scoreboard {
    int player1Score;
    int player2Score;

    Scoreboard() : player1Score(0), player2Score(0) {}

    void AddScore(int player, int points) {
        if (player == 1) {
            player1Score += points;
        } else if (player == 2) {
            player2Score += points;
        }
    }

    void Reset() {
        player1Score = 0;
        player2Score = 0;
    }

    void Draw(int screenWidth) const {
        DrawText(TextFormat("Player 1: %d", player1Score), 20, 20, 30, BLUE);
        DrawText(TextFormat("Player 2: %d", player2Score), screenWidth - 200, 20, 30, RED);
    }
};


// Base Entity class
class Entity {
protected:
    Vector2 position;
    float rotation;
    int width;
    int height;

public:
    Entity(Vector2 startPosition, int entityWidth, int entityHeight)
        : position(startPosition), width(entityWidth), height(entityHeight), rotation(0.0f) {}

    virtual void Update() = 0;
    virtual void Draw() = 0;

    Vector2 GetPosition() const {
        return position;
    }

    void SetPosition(Vector2 newPosition) {
        position = newPosition;
    }

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
};


class Tank : public Entity {
private:
    float speed;
    float accumulatedMovement;
    float gravity;
    float verticalVelocity;
    int health; 
    float movementDistance; 
    float currentMovement;  
    int direction;          

    Texture2D tankIdleRight;

public:
    Tank(float tankSpeed, Vector2 startPosition, Texture2D idleRight)
        : Entity(startPosition, 50, 30), speed(tankSpeed), accumulatedMovement(0.0f), gravity(0.3f), verticalVelocity(0.0f),
          health(100), movementDistance(500.0f), currentMovement(0.0f), direction(0),
          tankIdleRight(idleRight) {} // Initialize health

    void ApplyGravity(int terrain[]) {
        verticalVelocity += gravity;
        position.y += verticalVelocity;

        int terrainIndex = position.x / TERRAIN_SEGMENT_WIDTH;
        if (terrainIndex >= 0 && terrainIndex < TERRAIN_WIDTH) {
            float terrainHeight = SCREEN_HEIGHT - terrain[terrainIndex] - height;
            if (position.y > terrainHeight) {
                position.y = terrainHeight;
                verticalVelocity = 0;
            }
        }
    }

    void HandleInput(int terrain[]) {
        float previousPositionX = position.x;

        if (IsKeyDown(KEY_A) && currentMovement < movementDistance) {
            position.x -= speed;
            direction = -1;
        } else if (IsKeyDown(KEY_D) && currentMovement < movementDistance) {
            position.x += speed;
            direction = 1;
        } else {
            direction = 0;
        }

        if (position.x < 0) position.x = 0;
        if (position.x > SCREEN_WIDTH - width) position.x = SCREEN_WIDTH - width;

        accumulatedMovement += fabs(position.x - previousPositionX);
        currentMovement += fabs(position.x - previousPositionX);

        int terrainIndex = position.x / TERRAIN_SEGMENT_WIDTH;
        if (accumulatedMovement >= 5.0f) {
            if (terrainIndex >= 0 && terrainIndex < TERRAIN_WIDTH - 1) {
                float heightDifference = terrain[terrainIndex + 1] - terrain[terrainIndex];
                float distance = TERRAIN_SEGMENT_WIDTH;
                rotation = atan2(heightDifference, distance) * (180.0f / PI);
            }
            accumulatedMovement = 0.0f;
        }
    }

    void TakeDamage(int damage) {
        health -= damage;
        if (health < 0) health = 0;
    }

    int GetHealth() const {
        return health;
    }

    void Update() override {
       
    }

    void Draw() override {
        Texture2D currentTexture;
        if (direction == 1) {
            currentTexture = tankIdleRight;
        } else if (direction == -1) {
            currentTexture = tankIdleRight;
        } else {
            currentTexture = tankIdleRight;
        }

        float scaleFactor = 2.0f; // Increase this value to make the tank larger

        // Adjust the width and height based on the scaling factor
        float scaledWidth = width * scaleFactor;
        float scaledHeight = height * scaleFactor;

        DrawTexturePro(
            currentTexture,
            { 0, 0, (float)currentTexture.width, (float)currentTexture.height },
            { position.x + scaledWidth / 2 , position.y + scaledHeight / 2 -20, scaledWidth, scaledHeight },
            { scaledWidth / 2, scaledHeight / 2 },
            rotation,
            WHITE
        );

        DrawText(TextFormat("%d", health), position.x, position.y - 20, 20, RED);
        DrawText(TextFormat("Move: %.0f/%.0f", currentMovement, movementDistance), position.x, position.y - 40, 20, BLUE);
    }

    void ResetMovement() {
        currentMovement = 0.0f;
    }
};

// Projectile class inheriting from Entity
class Projectile : public Entity {
private:
    Vector2 velocity;
    float gravity;
    bool isActive;

public:
    Projectile() : Entity({ 0, 0 }, 10, 10), gravity(0.0f), isActive(false) {}

    void Shoot(const Vector2& startPos, const Vector2& targetPos, float initialVelocity, float grav) {
        position = startPos;
        Vector2 direction = { targetPos.x - startPos.x, targetPos.y - startPos.y };
        float distance = sqrt(direction.x * direction.x + direction.y * direction.y);
        velocity = { (direction.x / distance) * initialVelocity, (direction.y / distance) * initialVelocity };
        gravity = grav;
        isActive = true;
    }

    void Update(int terrain[], Tank& player1, Tank& player2, void (*HandleTerrainImpact)(int[], Vector2, float)) {
        if (isActive) {
            velocity.y += gravity;
            position.x += velocity.x;
            position.y += velocity.y;

            int terrainIndex = position.x / TERRAIN_SEGMENT_WIDTH;
            if (terrainIndex >= 0 && terrainIndex < TERRAIN_WIDTH && position.y >= SCREEN_HEIGHT - terrain[terrainIndex]) {
                isActive = false;
                HandleTerrainImpact(terrain, position, 50.0f);
            }

            if (position.x < 0 || position.x > SCREEN_WIDTH) {
                isActive = false;
            }

            // Check collision with players
            Rectangle projectileRect = { position.x - width / 2, position.y - height / 2, (float)width, (float)height };
            Rectangle player1Rect = { player1.GetPosition().x, player1.GetPosition().y, (float)player1.GetWidth(), (float)player1.GetHeight() };
            Rectangle player2Rect = { player2.GetPosition().x, player2.GetPosition().y, (float)player2.GetWidth(), (float)player2.GetHeight() };

            if (CheckCollisionRecs(projectileRect, player1Rect)) {
                isActive = false;
                player1.TakeDamage(20); // Reduce player 1 health
            }
            if (CheckCollisionRecs(projectileRect, player2Rect)) {
                isActive = false;
                player2.TakeDamage(20); // Reduce player 2 health
            }
        }
    }

    bool IsActive() const {
        return isActive;
    }

    void Update() override {
        // Custom projectile update logic
    }

    void Draw() override {
        if (isActive) {
            DrawCircle(position.x, position.y, 10, RED);
        }
    }
};

// Game class managing the game state
class Game {
public:
    Game();
    void Update();
    void Draw();
    void HandlePlayerShooting(Tank& player);
    void Terrain();
    static void HandleTerrainImpact(int terrain[], Vector2 impactPos, float radius);

private:
    Tank player1;
    Tank player2;
    Projectile currentProjectile;
    bool isPlayer1Turn;
    bool projectileActive;
    int terrain[TERRAIN_WIDTH];
    Texture2D background; // Add background texture
    Texture2D mainMenuBackground;
    Scoreboard scoreboard;
};

enum GameState {
    MAIN_MENU,
    GAME_PLAY,
    GAME_OVER
};

GameState currentState = MAIN_MENU;

Game::Game()
    : player1(5.0f, Vector2{ 100, 800 }, LoadTexture("bluetank/tankblue.png")),
      player2(5.0f, Vector2{ 1700, 800 }, LoadTexture("bluetank/tank_left.png")),
      isPlayer1Turn(true), projectileActive(false) {
    Terrain();
    background = LoadTexture("bluetank/background.png"); // Load background texture
    mainMenuBackground = LoadTexture("bluetank/tankblue.png");
}

void Game::HandlePlayerShooting(Tank& player) {
    if (!projectileActive && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        Vector2 playerPos = player.GetPosition();

        // Adjust the power calculation to ensure a proper range
        float distance = sqrt(pow(mousePos.x - playerPos.x, 2) + pow(mousePos.y - playerPos.y, 2));
        float power = RemapValue(distance, 0.0f, 500.0f, 1.0f, 10.0f); // Increased the range for higher power

        // Set a higher initial velocity if needed
        currentProjectile.Shoot(playerPos, mousePos, power, 0.3f);
        projectileActive = true;
    }
}

void Game::Update() {
    if (currentState == MAIN_MENU) {
        if (IsKeyPressed(KEY_ENTER)) {
            currentState = GAME_PLAY;
        }
    } else if (currentState == GAME_PLAY) {
        player1.ApplyGravity(terrain);
        player2.ApplyGravity(terrain);

        if (!projectileActive) {
            if (isPlayer1Turn) {
                player1.HandleInput(terrain);
                HandlePlayerShooting(player1);
            } else {
                player2.HandleInput(terrain);
                HandlePlayerShooting(player2);
            }
        }

        currentProjectile.Update(terrain, player1, player2, HandleTerrainImpact);

        if (projectileActive && !currentProjectile.IsActive()) {
            projectileActive = false;
            if (player1.GetHealth() <= 0) {
                scoreboard.AddScore(2, 10); // Player 2 wins, gain 10 points
                currentState = GAME_OVER;
            } else if (player2.GetHealth() <= 0) {
                scoreboard.AddScore(1, 10); // Player 1 wins, gain 10 points
                currentState = GAME_OVER;
            } else {
                isPlayer1Turn = !isPlayer1Turn;
                player1.ResetMovement();
                player2.ResetMovement();
            }
        }
    }
}

void Game::Terrain() {
    srand(static_cast<unsigned int>(time(NULL)));

    terrain[0] = rand() % MAX_HEIGHT;

    for (int i = 1; i < TERRAIN_WIDTH; i++) {
        int prevHeight = terrain[i - 1];
        int change = (rand() % (2 * HEIGHT_DIFF + 1)) - HEIGHT_DIFF;
        terrain[i] = prevHeight + change;

        if (terrain[i] < 0) terrain[i] = 0;
        if (terrain[i] >= MAX_HEIGHT) terrain[i] = MAX_HEIGHT - 1;
    }
}

void Game::HandleTerrainImpact(int terrain[], Vector2 impactPos, float radius) {
    int impactIndex = impactPos.x / TERRAIN_SEGMENT_WIDTH;
    int impactRadius = radius / TERRAIN_SEGMENT_WIDTH;

    for (int i = impactIndex - impactRadius; i <= impactIndex + impactRadius; i++) {
        if (i >= 0 && i < TERRAIN_WIDTH) {
            float distanceToImpact = fabs(i * TERRAIN_SEGMENT_WIDTH - impactPos.x);
            if (distanceToImpact < radius) {
                int craterDepth = sqrt(radius * radius - distanceToImpact * distanceToImpact);
                terrain[i] = terrain[i] - craterDepth;
                if (terrain[i] < 0) terrain[i] = 0;
            }
        }
    }
}

void Game::Draw() {
    if (currentState == MAIN_MENU) {
        DrawTexturePro(
            mainMenuBackground,
            { 0, 0, (float)mainMenuBackground.width, (float)mainMenuBackground.height },
            { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT },
            { 0, 0 },
            0.0f,
            WHITE
        );
        DrawText("TANKY", SCREEN_WIDTH / 2 - 375, SCREEN_HEIGHT / 2 - 470, 200, RED);
        DrawText("Press Enter to Start", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 20, BLACK);
    } else if (currentState == GAME_PLAY) {
        DrawTexturePro(
            background,
            { 0, 0, (float)background.width, (float)background.height },
            { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT },
            { 0, 0 },
            0.0f,
            WHITE
        );

        player1.Draw();
        player2.Draw();

        if (currentProjectile.IsActive()) {
            currentProjectile.Draw();
        }

        for (int i = 0; i < TERRAIN_WIDTH; i++) {
            DrawRectangle(i * TERRAIN_SEGMENT_WIDTH, SCREEN_HEIGHT - terrain[i], TERRAIN_SEGMENT_WIDTH, terrain[i], DARKGREEN);
        }

        scoreboard.Draw(SCREEN_WIDTH); // Draw the scoreboard
    } else if (currentState == GAME_OVER) {
        DrawText("Game Over", SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 20, 40, RED);
        DrawText("Press R to Restart", SCREEN_WIDTH / 2 - 70, SCREEN_HEIGHT / 2 + 20, 20, BLACK);
        DrawText("Press M for Main Menu", SCREEN_WIDTH / 2 - 90, SCREEN_HEIGHT / 2 + 50, 20, BLACK);

        scoreboard.Draw(SCREEN_WIDTH); // Draw the scoreboard on game over screen

        if (IsKeyPressed(KEY_R)) {
            player1.SetPosition({100, 800});
            player2.SetPosition({1700, 800});
            player1.TakeDamage(-100); // Reset health
            player2.TakeDamage(-100); // Reset health
            scoreboard.Reset();      // Reset the scoreboard
            isPlayer1Turn = true;
            projectileActive = false;
            currentState = GAME_PLAY;
        }

        if (IsKeyPressed(KEY_M)) {
            currentState = MAIN_MENU;
        }
    }

}

int main() {
    const int screenWidth = GetMonitorWidth(0);
    const int screenHeight = GetMonitorHeight(0);

    InitWindow(screenWidth, screenHeight, "Tanky");

    SetTargetFPS(60);

    Game game;

    while (!WindowShouldClose()) {
        game.Update();
        BeginDrawing();
        ClearBackground(RAYWHITE);
        game.Draw();
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
