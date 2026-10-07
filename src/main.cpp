#include <iostream>
#include <string>
#include <vector>
#include "raylib.h"

#define HEIGHT 300
#define WIDTH 480

float xrel(float n) {
  return n / 480.0f * WIDTH;
}

float yrel(float n) {
  return n / 300.0f * HEIGHT;
}

class GameObject {
public:
  float x, y;
  float width, height;
  float rotation;
  Texture2D texture;

  GameObject(std::string image) {
    std::string path = "assets/"+image;
    texture = LoadTexture(path.c_str());
  }

  virtual void draw(void) {
    DrawTexturePro(
      texture,
      {0,0, (float)texture.width, (float)texture.height},
      {xrel(0), yrel(0), xrel(width), yrel(height)},
      {0, 0},
      rotation,
      WHITE
    );
  }
};

class Card {
public:
  Texture2D texture;
  float height = yrel(0);
  float width = xrel(0);
  Card(std::string name, std::string ability, std::string description) {
    std::string path = "assets/Scroll.png";
    texture = LoadTexture(path.c_str());
    std::cout << "Height " << texture.height << " Width " << texture.width << std::endl;
  }
};


class Game {
public:
  std::vector<Card> hand;
  std::vector<Card> deck;
  std::vector<GameObject> objects;

  void init(void) {
    objects.emplace_back("ScrollDeck.png");
    objects.back().width = 32;
    objects.back().height = 76;
    objects.back().x = 500;
    objects.back().y = 205;
    objects.back().rotation = 90;
  }

  void drawObjects(void) {
    for (int i = 0; i < objects.size(); i++) {
      objects.at(i).draw();
    }
  }

  void deinit() {
    for (int i = 0; i < objects.size(); i++) {
      UnloadTexture(objects.at(i).texture);
    }
  }

  void draw() {
    hand.push_back(deck.at(deck.size()-1));
    deck.pop_back();
    objects.push_back(hand.at(hand.size()-1));
  }
};

int main (void) {
  const float screenWidth = (HEIGHT*16)/9;
  const float screenHeight = HEIGHT;

  InitWindow(screenWidth, screenHeight, "Deck Dungeon");
  SetTargetFPS(60);

  Game game;
  game.init();

  while (!WindowShouldClose()) {
    game.drawObjects();
  }
  game.deinit();
  CloseWindow();
  return 0;
}
