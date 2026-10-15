#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <cstddef>
#include <string>
#include <vector>
#include <array>

#include "raylib.h"

#define HEIGHT 300*2
#define WIDTH 480*2

class Game;

void replaceAll(std::string& str, const std::string& target, const std::string& replacement) {
  size_t pos = 0;
  while ((pos = str.find(target, pos)) != std::string::npos) {
    str.replace(pos, target.length(), replacement);
    pos += replacement.length();
  }
}

int substrCount(const std::string& main_str, const std::string& target) {
  if (main_str.empty() || target.empty()) return 0;

  int count = 0;
  size_t pos = main_str.find(target, 0);
  while (pos != std::string::npos) {
    count++;
    pos = main_str.find(target, pos + target.length()); 
  }
  return count;
}

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
  FREEZE,
  COUNT
};

std::array<std::string, static_cast<int>(Actions::COUNT)> actionDescriptions = {
  "move in any chosen direction by X",
  "move forwards by X",
  "move backwards by X",
  "turn left",
  "turn right",
  "damage an enemy in front",
  "damage an enemy adjacent",
  "you do not take damage for X turns",
  "deal X damage to an enemy that attacked you",
  "deal X burn damage to an enemy in front",
  "the enemy in front of you is frozen for X turns"
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

class Text {
public:
  std::unordered_map<char, Texture2D> textures;
  std::string text;
  std::string id;
  float x, y;

  Text(std::string text, float x, float y) {
    this->text = text;
    this->x = x;
    this->y = y;
    textures['/'] = LoadTexture("assets/fonts/slash.png");
  }

  void draw() {
    int offset = 0;
    int line = 0;
    int increment = 0;

    for (int i = 0; i < text.length(); i++) {
      char c = text[i];

      if (c == ' ') {
        offset += 4;
        increment ++;
        continue;
      }

      if (c == '\n') {
        line++;
        offset = 0;
        increment = 0;
        continue;
      }

      if (textures.find(c) == textures.end()) {
        std::string path = "assets/fonts/";
        path += c;
        path += ".png";

        textures[c] = LoadTexture(path.c_str());
      }

      Texture2D texture = textures.at(c);

      DrawTexturePro(
        texture,
        {0, 0, (float)texture.width, (float)texture.height},
        {
          xrel(x) + i * xrel(6) - xrel(offset),
          yrel(y) + line * yrel(8),
          xrel(6), yrel(6)
        },
        {0, 0}, 0, WHITE
      );
      increment++;
    }
  }

  void unload(void) {
    for (auto& [character, texture] : textures) {
      UnloadTexture(texture);
    }
  }
};

class Card {
public:
  Texture2D texture = LoadTexture("assets/ui/scroll.png");

  Texture2D top_left = LoadTexture("assets/ui/sizeable_scroll/top_left.png");
  Texture2D middle_left = LoadTexture("assets/ui/sizeable_scroll/middle_left.png");
  Texture2D bottom_left = LoadTexture("assets/ui/sizeable_scroll/bottom_left.png");
  Texture2D top_middle = LoadTexture("assets/ui/sizeable_scroll/top_middle.png");
  Texture2D middle_middle = LoadTexture("assets/ui/sizeable_scroll/middle_middle.png");
  Texture2D bottom_middle = LoadTexture("assets/ui/sizeable_scroll/bottom_middle.png");
  Texture2D top_right = LoadTexture("assets/ui/sizeable_scroll/top_right.png");
  Texture2D middle_right = LoadTexture("assets/ui/sizeable_scroll/middle_right.png");
  Texture2D bottom_right = LoadTexture("assets/ui/sizeable_scroll/bottom_right.png");

  int width = 64;
  int height = 95;
  int rotation = 0;
  int y = 210;
  int x = 0;

  std::vector<cardAction> actions;

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

  void hover() {
    if (actions.empty()) return;

    const std::string& description =
      actionDescriptions[static_cast<std::size_t>(actions.at(0).action)];

    int textHeight = static_cast<int>(actions.size()) * 8;

    int spaces = static_cast<int>(
      std::count(description.begin(), description.end(), ' ')
    );

    int textWidth = static_cast<int>(description.size()) * 6 - spaces * 2;

    // Top row
    DrawTexturePro(top_left,
      {0, 0, (float)top_left.width, (float)top_left.height},
      {xrel(10), yrel(10), xrel(top_left.width), yrel(top_left.height)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(top_middle,
      {0, 0, (float)top_middle.width, (float)top_middle.height},
      {xrel(10) + xrel(top_left.width), yrel(27),
      xrel(textWidth), yrel(top_middle.height)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(top_right,
      {0, 0, (float)top_right.width, (float)top_right.height},
      {xrel(10) + xrel( top_left.width + textWidth), yrel(27),
       xrel(top_right.width), yrel(top_right.height)},
      {0, 0}, rotation, WHITE);

    // Middle row
    DrawTexturePro(middle_left,
      {0, 0, (float)middle_left.width, (float)middle_left.height},
      {xrel(10), yrel(10) + yrel(top_left.height),
       xrel(middle_left.width), yrel(textHeight)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(middle_middle,
      {0, 0, (float)middle_middle.width, (float)middle_middle.height},
      {xrel(10) + xrel( middle_left.width), yrel(10) + yrel( top_left.height),
       xrel(textWidth), yrel(textHeight)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(middle_right,
      {0, 0, (float)middle_right.width, (float)middle_right.height},
      {xrel(10) + xrel( middle_left.width + textWidth), yrel(10) + yrel( top_left.height),
       xrel(middle_right.width), yrel(textHeight)},
      {0, 0}, rotation, WHITE);

    // Bottom row
    DrawTexturePro(bottom_left,
      {0, 0, (float)bottom_left.width, (float)bottom_left.height},
      {xrel(10), yrel(10) + yrel( top_left.height + textHeight),
       xrel(bottom_left.width), yrel(bottom_left.height)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(bottom_middle,
      {0, 0, (float)bottom_middle.width, (float)bottom_middle.height},
      {xrel(10) + xrel( bottom_left.width), yrel(10) + yrel( top_left.height + textHeight),
       xrel(textWidth), yrel(bottom_middle.height)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(bottom_right,
      {0, 0, (float)bottom_right.width, (float)bottom_right.height},
      {xrel(10) + xrel( bottom_left.width + textWidth),
       yrel(10) + yrel( top_left.height + textHeight),
       xrel(bottom_right.width), yrel(bottom_right.height)},
      {0, 0}, rotation, WHITE);

    for (int i = 0; i < actions.size(); i++) {
      std::string description =
        actionDescriptions[static_cast<std::size_t>(actions.at(i).action)];

      replaceAll(description, "X", std::to_string(actions.at(i).value));
      Text hoverText(description, 35, 30 + (i + 1) * 8);
      hoverText.draw();
    }
  }
  void use(Game& game);
};

class Game {
public:
  std::vector<Card> hand;
  std::vector<Card> deckList;
  std::vector<uiElement> ui;
  std::vector<Text> text;

  uiElement screen{"ui/screen.png", 0, 0, 0, 480, 300};
  uiElement dungeon{"env/dungeon.png", 56, 8, 0, 288, 144};
  uiElement frame{"ui/frame.png", 46, 0, 0, 309, 160};
  uiElement deck{"ui/scroll_deck.png", 409, 172, 0, 70, 30};
  uiElement actionBar{"ui/action_bar.png", 384, 102, 0, 80, 16};
  uiElement drawIcon{"ui/draw_icon.png", 382, 100, 0, 20, 20};
  uiElement dealIcon{"ui/deal_icon.png", 382, 120, 0, 20, 20};
  uiElement lookIcon{"ui/look_icon.png", 382, 140, 0, 20, 20};

  Text drawText{"draw 1 turn", 402, 107};
  Text dealText{"deal 1/5", 403, 127};
  Text lookText{"look 1 turn\nthis is some text!", 403, 147};

  Texture2D cursor;

  int turnCount;

  void init(void) {
    turnCount = 0;

    cursor = LoadTexture("assets/ui/cursor.png");

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
      if (CheckCollisionPointRec({x, y}, {xrel(hand.at(i).x), yrel(hand.at(i).y), xrel(hand.at(i).width), yrel(hand.at(i).height)})) {
        hand.at(i).hover();
      }
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
    for (auto& t : text) {
      t.unload();
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
    if (CheckCollisionPointRec({x, y}, {xrel(ui.at(4).x), yrel(ui.at(4).y), xrel(ui.at(4).width), yrel(ui.at(4).height)})) {
      drawCard();
      passTurn();
    }
    else if (CheckCollisionPointRec({x, y}, {xrel(ui.at(5).x), yrel(ui.at(5).y), xrel(ui.at(5).width), yrel(ui.at(5).height)})) {
      // Will add look functionality later
      passTurn();
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

  void passTurn() {
    turnCount += 1;
    int dealCount = turnCount % 5 + 1;
    text.at(1).text = "deal " + std::to_string(dealCount) + "/5";
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
