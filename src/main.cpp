#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>

#include "raylib.h"

#define HEIGHT 300*3
#define WIDTH 480*3

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

class uiElement {
public:
  float x, y;
  float width, height;
  float rotation;
  Texture2D texture;

  uiElement(std::string image, int x, int y, int rotation, int width, int height) {
    std::string path = "assets/"+image;
    texture = LoadTexture(path.c_str());
    this->x = x;
    this->y = y;
    this->rotation = rotation;
    this->width = width;
    this->height = height;
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

// a.png is just a placeholder.
class Text {
public:
  Texture2D texture;
  std::string text;
  std::string id;
  float x, y;

  Text(std::string text, float x, float y) {
    this->text = text;
    this->x = x;
    this->y = y;
  }

  void draw() {
    std::string path;
    // spaces are smaller than other characters
    int offset = 0;
    for (int i = 0; i < text.length(); i++) {
      if (text.at(i) == '/') {
        path = "assets/fonts/slash.png";
      }
      else if (text.at(i) == ' ') {
        offset += 4;
        continue;
      }
      else {
        path = "assets/fonts/";
        path += text.at(i);
        path += ".png";
      }
      texture = LoadTexture(path.c_str());
      DrawTexturePro(
        texture,
        {0,0, (float)texture.width, (float)texture.height},
        {xrel(x) + i * xrel(6) - xrel(offset), yrel(y), xrel(6), yrel(6)},
        {0, 0},
        0,
        WHITE
      );
    }
  }
};

class Card {
public:
  std::vector<icon> icons;
  std::vector<cardAction> actions;
  int width = 64;
  int height = 95;
  int rotation = 0;
  int y = 210;
  int x = 0;
  Texture2D texture = LoadTexture("assets/Scroll.png");

  void addAction(cardAction action) {
    actions.push_back(action);
  }

  void draw() {
    DrawTexturePro(
      texture,
      {0,0, (float)texture.width, (float)texture.height},
      {xrel(x), yrel(y), xrel(width), yrel(height)},
      {0, 0},
      rotation,
      WHITE
    );
  }

  void use(Game& game);
};

class Game {
public:
  std::vector<Card> hand;
  std::vector<Card> deckList;
  std::vector<uiElement> ui;
  std::vector<Text> text;

  uiElement screen{"Screen.png", 0, 0, 0, 480, 300};
  uiElement dungeon{"Dungeon.png", 56, 8, 0, 288, 144};
  uiElement frame{"Frame.png", 46, 0, 0, 309, 160};
  uiElement deck{"ScrollDeck.png", 409, 172, 0, 70, 30};
  uiElement actionBar{"action_bar.png", 384, 102, 0, 80, 16};
  uiElement drawIcon{"draw_icon.png", 382, 100, 0, 20, 20};
  uiElement dealIcon{"deal_icon.png", 382, 120, 0, 20, 20};
  uiElement lookIcon{"look_icon.png", 382, 140, 0, 20, 20};

  Text drawText{"draw 1 turn", 402, 107};
  Text dealText{"deal 0/5", 403, 127};
  Text lookText{"look 1 turn", 403, 147};

  Texture2D cursor;

  void init(void) {
    cursor = LoadTexture("assets/Cursor.png");

    ui.push_back(screen);
    ui.push_back(dungeon);
    ui.push_back(frame);
    ui.push_back(deck);
    for (int i = 0; i < 3; i++) {
      ui.push_back(actionBar);
      ui.back().y = 102 + 20 * i;
    }
    ui.push_back(drawIcon);
    ui.push_back(dealIcon);
    ui.push_back(lookIcon);

    text.push_back(drawText);
    text.push_back(dealText);
    text.push_back(lookText);

    // Make deck
    deckList.push_back(Card());
    deckList.back().addAction({Actions::MOVE_FORWARD, 1});
    deckList.push_back(Card());
    deckList.back().addAction({Actions::MOVE_BACKWARD, 2});
    deckList.push_back(Card());
    deckList.back().addAction({Actions::TURN_LEFT, 0});
  }

  void drawObjects(float x, float y) {
    BeginDrawing();
    ClearBackground(WHITE);
    for (int i = 0; i < ui.size(); i++) {
      ui.at(i).draw();
    }
    for (int i = 0; i < hand.size(); i++) {
      hand.at(i).draw();
    }
    for (int i = 0; i < text.size(); i++) {
      text.at(i).draw();
    }
    // draw cursor
    DrawTexturePro(
      cursor,
      {0,0, (float)cursor.width, (float)cursor.height},
      {x, y, xrel(12), yrel(12)},
      {0, 0},
      0,
      WHITE
    );
    EndDrawing();
  }

  void deinit() {
    for (int i = 0; i < ui.size(); i++) {
      UnloadTexture(ui.at(i).texture);
    }
    for (int i = 0; i < hand.size(); i++) {
      UnloadTexture(hand.at(i).texture);
    }
    for (int i = 0; i < text.size(); i++) {
      UnloadTexture(text.at(i).texture);
    }
    // unload cursor
    UnloadTexture(cursor);
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
    std::cout << x << " " << y << std::endl;
    if (CheckCollisionPointRec({x, y}, {xrel(deck.x), yrel(deck.y), xrel(deck.width), yrel(deck.height)})) {
      drawCard();
    }
    for (int i = 0; i < hand.size(); i++) {
      if (CheckCollisionPointRec({x, y}, {xrel(hand.at(i).x), yrel(hand.at(i).y), xrel(hand.at(i).width), yrel(hand.at(i).height)})) {
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

  HideCursor();

  Game game;
  game.init();

  while (!WindowShouldClose()) {
    Vector2 mousePos = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      game.click(mousePos.x, mousePos.y);
    }
    game.drawObjects(mousePos.x, mousePos.y);
  }
  game.deinit();
  CloseWindow();
  return 0;
}
