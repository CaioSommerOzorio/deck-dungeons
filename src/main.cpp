#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <cstddef>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <array>

#include "raylib.h"

#define HEIGHT 300*2
#define WIDTH 480*2

using namespace std::chrono_literals;

class Game;
std::unordered_map<std::string, Texture2D> textures;

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
  std::string textureName;

  uiElement(std::string textureName, int x, int y, float rotation, int width, int height) {
    this->texture = textures[textureName];
    this->textureName = textureName;
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
  std::string text;
  float x, y;

  Text(std::string text, float x, float y) {
    this->text = text;
    this->x = x;
    this->y = y;
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

      std::string textureName = std::to_string(c);
      if (textureName == std::to_string('/')) {
        textureName = "slash";
      }

      Texture2D texture = textures[textureName];

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
};

class Card {
public:
  Texture2D texture = textures["scroll"];

  Texture2D top_left      = LoadTexture("assets/ui/sizeable_scroll/top_left.png");
  Texture2D middle_left   = LoadTexture("assets/ui/sizeable_scroll/middle_left.png");
  Texture2D bottom_left   = LoadTexture("assets/ui/sizeable_scroll/bottom_left.png");
  Texture2D top_middle    = LoadTexture("assets/ui/sizeable_scroll/top_middle.png");
  Texture2D middle_middle = LoadTexture("assets/ui/sizeable_scroll/middle_middle.png");
  Texture2D bottom_middle = LoadTexture("assets/ui/sizeable_scroll/bottom_middle.png");
  Texture2D top_right     = LoadTexture("assets/ui/sizeable_scroll/top_right.png");
  Texture2D middle_right  = LoadTexture("assets/ui/sizeable_scroll/middle_right.png");
  Texture2D bottom_right  = LoadTexture("assets/ui/sizeable_scroll/bottom_right.png");

  std::array<Texture2D, static_cast<int>(Actions::COUNT)> actionTextures = {
    textures["move_scroll"],   // "move in any chosen direction by X",
    textures["move_scroll"],   // "move forwards by X",
    textures["move_scroll"],   // "move backwards by X",
    textures["move_scroll"],   // "turn left",
    textures["move_scroll"],   // "turn right",
    textures["melee_scroll"],  // "damage an enemy in front",
    textures["melee_scroll"],  // "damage an enemy adjacent",
    textures["defend_scroll"], // "you do not take damage for X turns",
    textures["defend_scroll"], // "deal X damage to an enemy that attacked you",
    textures["potion_scroll"], // "deal X burn damage to an enemy in front",
    textures["potion_scroll"], // "the enemy in front of you is frozen for X turns"
  };

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

    for (int i = 0; i < actions.size(); i++) {
      int actionIndex = static_cast<int>(actions.at(i).action);
      int yOffset = texture.height / (actions.size() + 1) - 16;
      DrawTexturePro(
        actionTextures[actionIndex],
        {0, 0, (float)actionTextures[actionIndex].width, (float)actionTextures[actionIndex].height},
        {xrel(x + (texture.width / 2)), yrel(y + yOffset), xrel(16), yrel(16)},
        {0, 0}, 0, WHITE
      );
    }
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
    DrawTexturePro(textures["top_left"],
      {0, 0, (float)textures["top_left"].width, (float)textures["top_left"].height},
      {xrel(10), yrel(10), xrel(textures["top_left"].width), yrel(textures["top_left"].height)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["top_middle"],
      {0, 0, (float)textures["top_middle"].width, (float)textures["top_middle"].height},
      {xrel(10) + xrel(textures["top_left"].width), yrel(27),
      xrel(textWidth), yrel(textures["top_middle"].height)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["top_right"],
      {0, 0, (float)textures["top_right"].width, (float)textures["top_right"].height},
      {xrel(10) + xrel( textures["top_left"].width + textWidth), yrel(27),
       xrel(textures["top_right"].width), yrel(textures["top_right"].height)},
      {0, 0}, rotation, WHITE);

    // Middle row
    DrawTexturePro(textures["middle_left"],
      {0, 0, (float)textures["middle_left"].width, (float)textures["middle_left"].height},
      {xrel(10), yrel(10) + yrel(textures["top_left"].height),
       xrel(textures["middle_left"].width), yrel(textHeight)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["middle_middle"],
      {0, 0, (float)textures["middle_middle"].width, (float)textures["middle_middle"].height},
      {xrel(10) + xrel( textures["middle_left"].width), yrel(10) + yrel( textures["top_left"].height),
       xrel(textWidth), yrel(textHeight)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["middle_right"],
      {0, 0, (float)textures["middle_right"].width, (float)textures["middle_right"].height},
      {xrel(10) + xrel( textures["middle_left"].width + textWidth), yrel(10) + yrel( textures["top_left"].height),
       xrel(textures["middle_right"].width), yrel(textHeight)},
      {0, 0}, rotation, WHITE);

    // Bottom row
    DrawTexturePro(textures["bottom_left"],
      {0, 0, (float)textures["bottom_left"].width, (float)textures["bottom_left"].height},
      {xrel(10), yrel(10) + yrel( textures["top_left"].height + textHeight),
       xrel(textures["bottom_left"].width), yrel(textures["bottom_left"].height)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["bottom_middle"],
      {0, 0, (float)textures["bottom_middle"].width, (float)textures["bottom_middle"].height},
      {xrel(10) + xrel( textures["bottom_left"].width), yrel(10) + yrel( textures["top_left"].height + textHeight),
       xrel(textWidth), yrel(textures["bottom_middle"].height)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["bottom_right"],
      {0, 0, (float)textures["bottom_right"].width, (float)textures["bottom_right"].height},
      {xrel(10) + xrel( textures["bottom_left"].width + textWidth),
       yrel(10) + yrel( textures["top_left"].height + textHeight),
       xrel(textures["bottom_right"].width), yrel(textures["bottom_right"].height)},
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

  uiElement screen{"screen", 0, 0, 0, 480, 300};
  uiElement dungeon{"mystery_dungeon", 56, 8, 0, 288, 144};
  uiElement frame{"frame", 46, 0, 0, 309, 160};
  uiElement deck{"deck", 409, 172, 0, 70, 30};
  uiElement actionBar{"action_bar", 384, 102, 0, 80, 16};
  uiElement drawIcon{"draw_icon", 382, 100, 0, 20, 20};
  uiElement dealIcon{"deal_icon", 382, 120, 0, 20, 20};
  uiElement lookIcon{"look_icon", 382, 140, 0, 20, 20};

  Text drawText{"draw 1 turn", 402, 107};
  Text dealText{"deal 1/5", 403, 127};
  Text lookText{"look 1 turn", 403, 147};


  int turnCount;

  void init(void) {
    turnCount = 0;

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

    // make deck
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
      textures["cursor"],
      {0,0, (float)textures["cursor"].width, (float)textures["cursor"].height},
      {x, y, xrel(12), yrel(12)},
      {0, 0},
      0,
      WHITE
    );
    EndDrawing();
  }

  void deinit() {
    for (const auto& [key, value]: textures) {
      UnloadTexture(value);
    }
  }

  void drawCard() {
    if (deckList.size() == 0) {
      return;
    }
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
  //SetTraceLogLevel(LOG_NONE);
  const float screenWidth = WIDTH;
  const float screenHeight = HEIGHT;

  InitWindow(screenWidth, screenHeight, "Deck Dungeon");
  SetTargetFPS(60);

  HideCursor();

  textures = {
    {"mystery_dungeon", LoadTexture("assets/dungeons/mystery_dungeon.png")},
    {"slash", LoadTexture("assets/fonts/slash.png")},

    // Card related
    {"scroll", LoadTexture("assets/ui/scroll.png")},

    {"bomb_scroll", LoadTexture("assets/scroll_icons/bomb_scroll.png")},
    {"defend_scroll", LoadTexture("assets/scroll_icons/defend_scroll.png")},
    {"heal_heart_scroll", LoadTexture("assets/scroll_icons/heal_heart_scroll.png")},
    {"insight_scroll", LoadTexture("assets/scroll_icons/insight_scroll.png")},
    {"melee_scroll", LoadTexture("assets/scroll_icons/melee_scroll.png")},
    {"movement_scroll", LoadTexture("assets/scroll_icons/movement_scroll.png")},
    {"move_scroll", LoadTexture("assets/scroll_icons/move_scroll.png")},
    {"potion_scroll", LoadTexture("assets/scroll_icons/potion_scroll.png")},
    {"ranged_scroll", LoadTexture("assets/scroll_icons/ranged_scroll.png")},
    {"spell_scroll", LoadTexture("assets/scroll_icons/spell_scroll.png")},
    {"turn_scroll", LoadTexture("assets/scroll_icons/turn_scroll.png")},

    {"top_left", LoadTexture("assets/ui/sizeable_scroll/top_left.png")},
    {"middle_left", LoadTexture("assets/ui/sizeable_scroll/middle_left.png")},
    {"bottom_left", LoadTexture("assets/ui/sizeable_scroll/bottom_left.png")},
    {"top_middle", LoadTexture("assets/ui/sizeable_scroll/top_middle.png")},
    {"middle_middle", LoadTexture("assets/ui/sizeable_scroll/middle_middle.png")},
    {"bottom_middle", LoadTexture("assets/ui/sizeable_scroll/bottom_middle.png")},
    {"top_right", LoadTexture("assets/ui/sizeable_scroll/top_right.png")},
    {"middle_right", LoadTexture("assets/ui/sizeable_scroll/middle_right.png")},
    {"bottom_right", LoadTexture("assets/ui/sizeable_scroll/bottom_right.png")},
 
    // UI elements
    {"action_bar", LoadTexture("assets/ui/action_bar.png")},
    {"cursor", LoadTexture("assets/ui/cursor.png")},
    {"deal_icon", LoadTexture("assets/ui/deal_icon.png")},
    {"look_icon", LoadTexture("assets/ui/look_icon.png")},
    {"draw_icon", LoadTexture("assets/ui/draw_icon.png")},
    {"heart", LoadTexture("assets/ui/heart.png")},
    {"frame", LoadTexture("assets/ui/frame.png")},
    {"screen", LoadTexture("assets/ui/screen.png")},
    {"deck", LoadTexture("assets/ui/scroll_deck.png")}
  };

  // init font textures
  for (char letter = 'a'; letter <= 'z'; letter++) {
    std::string path = "assets/fonts/" + std::string(1, letter) + ".png";
    textures[std::to_string(letter)] = LoadTexture(path.c_str());
  }
  for (char num = '0'; num <= '9'; num++) {
    std::string path = "assets/fonts/" + std::string(1, num) + ".png";
    textures[std::to_string(num)] = LoadTexture(path.c_str());
  }

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
