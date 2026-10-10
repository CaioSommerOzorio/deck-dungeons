#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>

#include "raylib.h"

#define HEIGHT 300
#define WIDTH 480

class Game;

float xrel(float n) {
  return n / 480.0f * WIDTH;
}

float yrel(float n) {
  return n / 300.0f * HEIGHT;
}

typedef struct icon {
  float x, y;
  float width, height;
  float rotation;
  Texture2D texture;
} icon;

enum class Actions {
  MOVE_CHOICE,
  MOVE_FORWARD,
  MOVE_BACKWARD,
  TURN_LEFT,
  TURN_RIGHT,
  DAMAGE_FRONT,
  DAMAGE_ADJACENT,
  DEFEND,
  PARRY,
  BURN,
  FREEZE
};

typedef struct cardAction {
  Actions action;
  int value;
} cardAction;

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
      {xrel(x), yrel(y), xrel(width), yrel(height)},
      {0, 0},
      rotation,
      WHITE
    );
  }
};

class Card: public GameObject {
public:
  std::vector<icon> icons;
  std::vector<cardAction> actions;

  Card() : GameObject("Scroll.png") {
    width = 64;
    height = 95;
    rotation = 0;
    y = 210;
  }

  void addAction(cardAction action) {
    actions.push_back(action);
  }

  void use(Game& game);
};

class Game {
public:
  std::vector<Card> hand;
  std::vector<Card> deckList;
  std::vector<GameObject> objects;

  GameObject deck{"ScrollDeck.png"};
  GameObject dungeon{"Dungeon.png"};

  void init(void) {
    dungeon.x = 56;
    dungeon.y = 8;
    dungeon.rotation = 0;
    dungeon.width = 288;
    dungeon.height = 144;
    objects.push_back(dungeon);

    deck.width = 70;
    deck.height = 30;
    deck.x = 409;
    deck.y = 172;
    deck.rotation = 0;
    objects.push_back(deck);

    // Make deck
    deckList.push_back(Card());
    deckList.back().addAction({Actions::MOVE_FORWARD, 1});
    deckList.push_back(Card());
    deckList.back().addAction({Actions::MOVE_BACKWARD, 2});
    deckList.push_back(Card());
    deckList.back().addAction({Actions::TURN_LEFT, 0});
  }

  void drawObjects(void) {
    BeginDrawing();
    ClearBackground(WHITE);
    for (int i = 0; i < objects.size(); i++) {
      objects.at(i).draw();
    }
    for (int i = 0; i < hand.size(); i++) {
      hand.at(i).draw();
    }
    EndDrawing();
  }

  void deinit() {
    for (int i = 0; i < objects.size(); i++) {
      UnloadTexture(objects.at(i).texture);
    }
  }

  void drawCard() {
    if (deckList.size() == 0) {
      std::cout << "Deck empty" << std::endl;
      return;
    }
    std::cout << "Drawing card" << std::endl;
    hand.push_back(deckList.at(deckList.size()-1));
    hand.back().x = 480 - (hand.back().width * (hand.size()));
    deckList.pop_back();
  }

  void click(float x, float y) {
    //std::cout << x << " " << y << std::endl;
    if (CheckCollisionPointRec({x, y}, {deck.x, deck.y, deck.width, deck.height})) {
      drawCard();
    }
    for (int i = 0; i < hand.size(); i++) {
      if (CheckCollisionPointRec({x, y}, {hand.at(i).x, hand.at(i).y, hand.at(i).width, hand.at(i).height})) {
        hand.at(i).use(*this);
        return;
      }
    }
  }

  void discard(Card* card) {
    for (int i = 0; i < hand.size(); i++) {
      if (&hand[i] == card) {
        hand.erase(hand.begin()+i);
        break;
      }
    }
  }
};

void Card::use(Game& game) {
  for (int i = 0; i < actions.size(); i++) {
    switch (actions.at(i).action) {
      case Actions::MOVE_CHOICE:
        std::cout << "move choice" << std::endl;
        break;
      case Actions::MOVE_FORWARD:
        std::cout << "move forward by " << actions.at(i).value << std::endl;
        break;
      case Actions::MOVE_BACKWARD:
        std::cout << "move backward by " << actions.at(i).value << std::endl;
        break;
      case Actions::TURN_LEFT:
        std::cout << "turn left" << std::endl;
        break;
      case Actions::TURN_RIGHT:
        std::cout << "turn right" << std::endl;
        break;
    }
  }
  game.discard(this);
};

int main (void) {
  const float screenWidth = WIDTH;
  const float screenHeight = HEIGHT;

  InitWindow(screenWidth, screenHeight, "Deck Dungeon");
  SetTargetFPS(60);

  Game game;
  game.init();

  while (!WindowShouldClose()) {
    game.drawObjects();
    Vector2 mousePoint = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      game.click(mousePoint.x, mousePoint.y);
    }
  }
  game.deinit();
  CloseWindow();
  return 0;
}
