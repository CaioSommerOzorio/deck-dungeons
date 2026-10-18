#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <cstddef>
#include <random>
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
  QUICK_DRAW,
  AIM,
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
  "deal X damage to an enemy in front",
  "deal X damage to an enemy adjacent",
  "you do not take damage for X turns",
  "deal X damage to enemies that attacked you",
  "deal X damage to all enemies in front of you",
  "change direction and deal X damage to an enemy in front",
  "deal X burn damage to an enemy in front",
  "the enemy in front of you is frozen for X turns"
};

typedef struct cardAction {
  Actions action;
  int value;
} cardAction;

std::unordered_map<Actions, Texture2D> actionTextures;

std::unordered_map<std::string, std::vector<cardAction>> cardActions = {
  // movement
  {"walk", {{Actions::MOVE_CHOICE, 1}}},
  {"run", {{Actions::MOVE_FORWARD, 2}}},
  {"dodge", {{Actions::MOVE_BACKWARD, 2}}},
  {"turn right", {{Actions::TURN_RIGHT, 1}, {Actions::MOVE_FORWARD, 1}}},
  {"turn left", {{Actions::TURN_LEFT, 1}, {Actions::MOVE_FORWARD, 1}}},
  // melee
  {"stab", {{Actions::DAMAGE_FRONT, 2}}},
  {"slash", {{Actions::DAMAGE_ADJACENT, 1}}},
  {"defend", {{Actions::DEFEND, 2}}},
  {"parry", {{Actions::PARRY, 1}}},
  // ranged
  {"quick draw", {{Actions::QUICK_DRAW, 1}}},
  {"aim", {{Actions::AIM, 1}}},
  // potions
  {"burn", {{Actions::BURN, 1}}},
  {"freeze", {{Actions::FREEZE, 1}}}
};


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
      else if (textureName == std::to_string(':')) {
        textureName = "colon";
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

  int width = 64;
  int height = 95;
  int rotation = 0;
  int y = 210;
  int x = 0;

  std::vector<cardAction> actions;

  Card(std::string cardName = "") {
    if (cardName != "") {
      if (!cardActions.contains(cardName)) {
        std::cout << "action not found: " << cardName << std::endl;
        return;
      }
      for (int i = 0; i < cardActions[cardName].size(); i++) {
        actions.push_back(cardActions[cardName][i]);
      }
    }
  }

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
      Texture2D actionTexture = actionTextures[actions.at(i).action];
      float spacing = (float)texture.height / (actions.size() + 1);
      float yOffset = spacing * (i + 1) - 16;
      DrawTexturePro(
        actionTexture,
        {0, 0, (float)actionTexture.width, (float)actionTexture.height},
        {xrel(x + (texture.width / 2) - 8), yrel(y + yOffset), xrel(16), yrel(16)},
        {0, 0}, 0, WHITE
      );
    }
  }

  void hover() {
    if (actions.empty()) return;

    auto longest = std::max_element(
      actions.begin(), actions.end(),
      [](const cardAction& a, const cardAction& b) {
        return actionDescriptions[static_cast<std::size_t>(a.action)].length()
          < actionDescriptions[static_cast<std::size_t>(b.action)].length();
      }
    );
    const std::string& description =
      actionDescriptions[static_cast<std::size_t>(longest->action)];

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
      {xrel(10) + xrel(textures["top_left"].width + textWidth), yrel(27),
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
      {xrel(10) + xrel(textures["middle_left"].width), yrel(10) + yrel(textures["top_left"].height),
       xrel(textWidth), yrel(textHeight)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["middle_right"],
      {0, 0, (float)textures["middle_right"].width, (float)textures["middle_right"].height},
      {xrel(10) + xrel(textures["middle_left"].width + textWidth), yrel(10) + yrel(textures["top_left"].height),
       xrel(textures["middle_right"].width), yrel(textHeight)},
      {0, 0}, rotation, WHITE);

    // Bottom row
    DrawTexturePro(textures["bottom_left"],
      {0, 0, (float)textures["bottom_left"].width, (float)textures["bottom_left"].height},
      {xrel(10), yrel(10) + yrel(textures["top_left"].height + textHeight),
       xrel(textures["bottom_left"].width), yrel(textures["bottom_left"].height)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["bottom_middle"],
      {0, 0, (float)textures["bottom_middle"].width, (float)textures["bottom_middle"].height},
      {xrel(10) + xrel(textures["bottom_left"].width), yrel(10) + yrel(textures["top_left"].height + textHeight),
       xrel(textWidth), yrel(textures["bottom_middle"].height)},
      {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["bottom_right"],
      {0, 0, (float)textures["bottom_right"].width, (float)textures["bottom_right"].height},
      {xrel(10) + xrel(textures["bottom_left"].width + textWidth),
       yrel(10) + yrel(textures["top_left"].height + textHeight),
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
  // TEMPORARY will only load when needed
  Sound mystery_soundtrack = LoadSound("soundtrack/mystery_dungeon.mp3");

  std::vector<Card> hand;
  std::vector<Card> deckList;
  std::vector<uiElement> ui;
  std::vector<Text> text;
  std::vector<uiElement> minimap;

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
  Text deckCount{"deck: X/Y", 340, 189};

  std::random_device rd;

  int turnCount;
  int deckSize;
  int hearts;

  void init(void) {
    PlaySound(mystery_soundtrack);
    this->turnCount = 0;

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
    text.push_back(deckCount);

    // make deck
    addCards("walk", 5);
    addCards("run", 4);
    addCards("dodge", 2);
    addCards("turn right", 2);
    addCards("turn left", 2);
    addCards("stab", 3);
    addCards("slash", 3);
    addCards("defend", 2);
    addCards("parry", 1);
    addCards("quick draw", 3);
    addCards("aim", 3);

    this->deckSize = deckList.size();
    std::string deckString = "deck: " + std::to_string(this->deckSize) + "/" + std::to_string(this->deckSize);
    text.at(3).text = deckString;
    shuffleDeck();
  }

  void addCards(std::string card, int amount) {
    for (int i = 0; i < amount; i++) {
      deckList.push_back(Card(card));
    }
  }

  void shuffleDeck() {
    std::mt19937 g(rd());
    std::shuffle(deckList.begin(), deckList.end(), g);
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
    text.at(3).text = "deck: " + std::to_string(this->deckList.size()) + "/" + std::to_string(this->deckSize);
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
    UnloadSound(mystery_soundtrack);
  }

  void drawCard() {
    if (deckList.size() == 0) {
      return;
    }
    else if (hand.size() == 7) {
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
    for (int i = 0; i < hand.size(); i++) {
      hand.at(i).x = 480 - (hand.at(i).width * (i + 1));
    }
    std::cout << "card discarded" << std::endl;
  }

  void passTurn() {
    turnCount += 1;
    int dealCount = turnCount % 5 + 1;
    text.at(1).text = "deal " + std::to_string(dealCount) + "/5";
    if (dealCount == 1)
      while (hand.size() < 7) {
        drawCard();
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
  //SetTraceLogLevel(LOG_NONE);
  const float screenWidth = WIDTH;
  const float screenHeight = HEIGHT;

  InitWindow(screenWidth, screenHeight, "Deck Dungeon");
  InitAudioDevice();
  SetTargetFPS(60);

  HideCursor();

  textures = {
    {"mystery_dungeon", LoadTexture("assets/dungeons/mystery_dungeon.png")},

    // Mini map
    {"room map", LoadTexture("assets/minimap/room.png")},
    {"connection map", LoadTexture("assets/minimap/connection.png")},
    {"enemy map", LoadTexture("assets/minimap/enemy.png")},
    {"plauer map", LoadTexture("assets/minimap/player.png")},

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
    {"turn_left_scroll", LoadTexture("assets/scroll_icons/turn_scroll.png")},
    {"turn_right_scroll", LoadTexture("assets/scroll_icons/turn_scroll.png")},

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
    {"deck", LoadTexture("assets/ui/scroll_deck.png")},

    // Additional Fonts
    {"colon", LoadTexture("assets/fonts/colon.png")},
    {"slash", LoadTexture("assets/fonts/slash.png")}
  };

  actionTextures = {
    {Actions::MOVE_CHOICE, textures["move_scroll"]},
    {Actions::MOVE_FORWARD, textures["move_scroll"]},
    {Actions::MOVE_BACKWARD, textures["move_scroll"]},
    {Actions::TURN_LEFT, textures["turn_left_scroll"]},
    {Actions::TURN_RIGHT, textures["turn_right_scroll"]},
    {Actions::DAMAGE_FRONT, textures["melee_scroll"]},
    {Actions::DAMAGE_ADJACENT, textures["melee_scroll"]},
    {Actions::DEFEND, textures["defend_scroll"]},
    {Actions::PARRY, textures["defend_scroll"]},
    {Actions::QUICK_DRAW, textures["ranged_scroll"]},
    {Actions::AIM, textures["ranged_scroll"]},
    {Actions::BURN, textures["ranged_scroll"]},
    {Actions::FREEZE, textures["potion_scroll"]}
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
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
