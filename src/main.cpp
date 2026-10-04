#include <iostream>
#include "raylib.h"

#define HEIGHT 1080
#define WIDTH 1920

float xrel(float n) {
  return n / 1920.0f * WIDTH;
}

float yrel(float n) {
  return n / 1080.0f * HEIGHT;
}

class Card {
public:
  Texture2D texture;
  float height = yrel(363);
  float width = xrel(250);
  Card(std::string name, std::string ability, std::string description, std::string image) {
    std::string path = "assets/"+image;
    texture = LoadTexture(path.c_str());
    std::cout << "Height " << texture.height << " Width " << texture.width << std::endl;
  }
};

int main (void) {
  const float screenWidth = (HEIGHT*16)/9;
  const float screenHeight = HEIGHT;

  InitWindow(screenWidth, screenHeight, "Deck Dungeon");
  SetTargetFPS(60);

  Card walk = Card("Walk", "Movement", "Move 1 forward in any direction you choose", "walk.png");

  while (!WindowShouldClose()) {
    BeginDrawing();
      ClearBackground(RAYWHITE);
      DrawTexturePro(
          walk.texture,
          {0, 0, (float)walk.texture.width, (float)walk.texture.height},
          {100, 200, walk.width, walk.height},
          {0, 0},
          0,
          WHITE
      );
    EndDrawing();
  }
  UnloadTexture(walk.texture);
  CloseWindow();
  return 0;
}
