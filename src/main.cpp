#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "raylib.h"

#define HEIGHT 300 * 2
#define WIDTH 480 * 2

using namespace std::chrono_literals;

class Game;
std::unordered_map<std::string, Texture2D> textures;

void replaceAll(std::string &str, const std::string &target,
                const std::string &replacement) {
  size_t pos = 0;
  while ((pos = str.find(target, pos)) != std::string::npos) {
    str.replace(pos, target.length(), replacement);
    pos += replacement.length();
  }
}

int substrCount(const std::string &main_str, const std::string &target) {
  if (main_str.empty() || target.empty())
    return 0;

  int count = 0;
  size_t pos = main_str.find(target, 0);
  while (pos != std::string::npos) {
    count++;
    pos = main_str.find(target, pos + target.length());
  }
  return count;
}

float xrel(float n) { return n / 480.0f * WIDTH; }

float yrel(float n) { return n / 300.0f * HEIGHT; }

typedef struct icon {
  float x, y;
  float width, height;
  float rotation;
  Texture2D texture;
} icon;

enum class Direction { UP, RIGHT, DOWN, LEFT };

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
    "the enemy in front of you is frozen for X turns"};

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
    {"freeze", {{Actions::FREEZE, 1}}}};

class uiElement {
public:
  float x, y;
  float width, height;
  float rotation;
  Texture2D texture;
  std::string textureName;

  uiElement(std::string textureName, int x, int y, float rotation, int width,
            int height) {
    this->texture = textures[textureName];
    this->textureName = textureName;
    this->x = x;
    this->y = y;
    this->rotation = rotation;
    this->width = width;
    this->height = height;
  }

  virtual void draw(void) {
    DrawTexturePro(texture, {0, 0, (float)texture.width, (float)texture.height},
                   {xrel(x), yrel(y), xrel(width), yrel(height)}, {0, 0},
                   rotation, WHITE);
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
        increment++;
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
      } else if (textureName == std::to_string(':')) {
        textureName = "colon";
      }

      Texture2D texture = textures[textureName];

      DrawTexturePro(texture,
                     {0, 0, (float)texture.width, (float)texture.height},
                     {xrel(x) + i * xrel(6) - xrel(offset),
                      yrel(y) + line * yrel(8), xrel(6), yrel(6)},
                     {0, 0}, 0, WHITE);
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

  void addAction(cardAction action) { actions.push_back(action); }

  void draw() {
    DrawTexturePro(texture, {0, 0, (float)texture.width, (float)texture.height},
                   {xrel(x), yrel(y), xrel(width), yrel(height)}, {0, 0},
                   rotation, WHITE);

    for (int i = 0; i < actions.size(); i++) {
      Texture2D actionTexture = actionTextures[actions.at(i).action];
      float spacing = (float)texture.height / (actions.size() + 1);
      float yOffset = spacing * (i + 1) - 16;
      DrawTexturePro(
          actionTexture,
          {0, 0, (float)actionTexture.width, (float)actionTexture.height},
          {xrel(x + (texture.width / 2) - 8), yrel(y + yOffset), xrel(16),
           yrel(16)},
          {0, 0}, 0, WHITE);
    }
  }

  void hover() {
    if (actions.empty())
      return;

    auto longest = std::max_element(
        actions.begin(), actions.end(),
        [](const cardAction &a, const cardAction &b) {
          return actionDescriptions[static_cast<std::size_t>(a.action)]
                     .length() <
                 actionDescriptions[static_cast<std::size_t>(b.action)]
                     .length();
        });
    const std::string &description =
        actionDescriptions[static_cast<std::size_t>(longest->action)];

    int textHeight = static_cast<int>(actions.size()) * 8;
    int spaces = static_cast<int>(
        std::count(description.begin(), description.end(), ' '));

    int textWidth = static_cast<int>(description.size()) * 6 - spaces * 2;

    // Top row
    DrawTexturePro(textures["top_left"],
                   {0, 0, (float)textures["top_left"].width,
                    (float)textures["top_left"].height},
                   {xrel(10), yrel(10), xrel(textures["top_left"].width),
                    yrel(textures["top_left"].height)},
                   {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["top_middle"],
                   {0, 0, (float)textures["top_middle"].width,
                    (float)textures["top_middle"].height},
                   {xrel(10) + xrel(textures["top_left"].width), yrel(27),
                    xrel(textWidth), yrel(textures["top_middle"].height)},
                   {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["top_right"],
                   {0, 0, (float)textures["top_right"].width,
                    (float)textures["top_right"].height},
                   {xrel(10) + xrel(textures["top_left"].width + textWidth),
                    yrel(27), xrel(textures["top_right"].width),
                    yrel(textures["top_right"].height)},
                   {0, 0}, rotation, WHITE);

    // Middle row
    DrawTexturePro(textures["middle_left"],
                   {0, 0, (float)textures["middle_left"].width,
                    (float)textures["middle_left"].height},
                   {xrel(10), yrel(10) + yrel(textures["top_left"].height),
                    xrel(textures["middle_left"].width), yrel(textHeight)},
                   {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["middle_middle"],
                   {0, 0, (float)textures["middle_middle"].width,
                    (float)textures["middle_middle"].height},
                   {xrel(10) + xrel(textures["middle_left"].width),
                    yrel(10) + yrel(textures["top_left"].height),
                    xrel(textWidth), yrel(textHeight)},
                   {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["middle_right"],
                   {0, 0, (float)textures["middle_right"].width,
                    (float)textures["middle_right"].height},
                   {xrel(10) + xrel(textures["middle_left"].width + textWidth),
                    yrel(10) + yrel(textures["top_left"].height),
                    xrel(textures["middle_right"].width), yrel(textHeight)},
                   {0, 0}, rotation, WHITE);

    // Bottom row
    DrawTexturePro(textures["bottom_left"],
                   {0, 0, (float)textures["bottom_left"].width,
                    (float)textures["bottom_left"].height},
                   {xrel(10),
                    yrel(10) + yrel(textures["top_left"].height + textHeight),
                    xrel(textures["bottom_left"].width),
                    yrel(textures["bottom_left"].height)},
                   {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["bottom_middle"],
                   {0, 0, (float)textures["bottom_middle"].width,
                    (float)textures["bottom_middle"].height},
                   {xrel(10) + xrel(textures["bottom_left"].width),
                    yrel(10) + yrel(textures["top_left"].height + textHeight),
                    xrel(textWidth), yrel(textures["bottom_middle"].height)},
                   {0, 0}, rotation, WHITE);

    DrawTexturePro(textures["bottom_right"],
                   {0, 0, (float)textures["bottom_right"].width,
                    (float)textures["bottom_right"].height},
                   {xrel(10) + xrel(textures["bottom_left"].width + textWidth),
                    yrel(10) + yrel(textures["top_left"].height + textHeight),
                    xrel(textures["bottom_right"].width),
                    yrel(textures["bottom_right"].height)},
                   {0, 0}, rotation, WHITE);

    for (int i = 0; i < actions.size(); i++) {
      std::string description =
          actionDescriptions[static_cast<std::size_t>(actions.at(i).action)];

      replaceAll(description, "X", std::to_string(actions.at(i).value));
      Text hoverText(description, 35, 30 + (i + 1) * 8);
      hoverText.draw();
    }
  }
  void use(Game &game);
};

class Encounter {
public:
  int x, y;
  Texture2D texture;
  virtual ~Encounter() {}

  void draw(int playerx, int playery) {
    // no need to draw if not visible in minimap
    // also no need to draw if we are drawing the player arrow
    if (x - playerx < 2 || y - playery < 2 || (playerx == x && playery == y)) {
      return;
    }

    int drawx = 416;
    int drawy = 43;

    if (x < playerx) {
      drawx += 24;
    } else if (x > playerx) {
      drawx -= 24;
    }

    if (y < playery) {
      drawy += 24;
    } else if (y > playery) {
      drawy -= 24;
    }

    DrawTexturePro(
        texture, {0, 0, (float)texture.width, (float)texture.height},
        {xrel(drawx), yrel(drawy), xrel(texture.width), yrel(texture.height)},
        {0, 0}, 0, WHITE);
  }
};

class Enemy : public Encounter {
public:
  std::string type;
  int hp;
  int attack;
  int x, y;
  Texture2D mapIcon;
  Texture2D texture;

  Enemy(int hp, int attack, int x, int y) {
    mapIcon = textures["enemy map"];
    this->hp = hp;
    this->attack = attack;
    this->x = x;
    this->y = y;
  }
};

class Player {
public:
  int x, y;
  int deckSize;
  int hearts;
  Direction direction;
  std::vector<Card> hand;
  std::vector<Card> deck;
  std::vector<Card> discard;
};

class Room {
public:
  int x, y;
  bool exists;
  Texture2D texture;
  std::vector<Encounter> encounters;

  Room(bool exists) {
    this->exists = exists;
    this->texture = textures["room map"];
  }

  void draw(int playerx, int playery, std::vector<std::vector<Room>> map) {
    if (exists && (abs(playerx - x) < 2 && abs(playery - y) < 2)) {
      int drawx = 414;
      int drawy = 41;

      if (x < playerx) {
        drawx -= 23;
      } else if (x > playerx) {
        drawx += 23;
      }

      if (y < playery) {
        drawy -= 23;
      } else if (y > playery) {
        drawy += 23;
      }

      DrawTexturePro(
          texture, {0, 0, (float)texture.width, (float)texture.height},
          {xrel(drawx), yrel(drawy), xrel(texture.width), yrel(texture.height)},
          {0, 0}, 0, WHITE);

      Texture2D connectionHorizontal = textures["connection horizontal"];
      Texture2D connectionVertical = textures["connection vertical"];

      if (x != 0 && map.at(y).at(x - 1).exists) {
        DrawTexturePro(connectionHorizontal,
                       {0, 0, (float)connectionHorizontal.width,
                        (float)connectionHorizontal.height},
                       {xrel(drawx - 4), yrel(drawy + 7),
                        xrel(connectionHorizontal.width),
                        yrel(connectionHorizontal.height)},
                       {0, 0}, 0, WHITE);
      }
      if (x != map.at(0).size() - 1 && map.at(y).at(x + 1).exists) {
        DrawTexturePro(connectionHorizontal,
                       {0, 0, (float)connectionHorizontal.width,
                        (float)connectionHorizontal.height},
                       {xrel(drawx + 19), yrel(drawy + 7),
                        xrel(connectionHorizontal.width),
                        yrel(connectionHorizontal.height)},
                       {0, 0}, 0, WHITE);
      }

      if (y != 0 && map.at(y - 1).at(x).exists) {
        DrawTexturePro(connectionVertical,
                       {0, 0, (float)connectionVertical.width,
                        (float)connectionVertical.height},
                       {xrel(drawx + 7), yrel(drawy - 4),
                        xrel(connectionVertical.width),
                        yrel(connectionVertical.height)},
                       {0, 0}, 0, WHITE);
      }
      if (y != map.size() - 1 && map.at(y + 1).at(x).exists) {
        DrawTexturePro(connectionVertical,
                       {0, 0, (float)connectionVertical.width,
                        (float)connectionVertical.height},
                       {xrel(drawx + 7), yrel(drawy + 19),
                        xrel(connectionVertical.width),
                        yrel(connectionVertical.height)},
                       {0, 0}, 0, WHITE);
      }

      if (encounters.size() > 0) {
        encounters.at(0).draw(playerx, playery);
      }
    }
  }
};

class Game {
public:
  // TEMPORARY will only load when needed
  // Sound mystery_soundtrack = LoadSound("soundtrack/mystery_dungeon.mp3");

  std::vector<uiElement> ui;
  std::vector<Text> text;
  std::vector<std::vector<Room>> map;
  Player player;

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
  std::mt19937 g{rd()};

  int turnCount;

  void init(void) {
    // PlaySound(mystery_soundtrack);
    turnCount = 0;
    player.direction = Direction::UP;

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

    player.deckSize = player.deck.size();
    std::string deckString = "deck: " + std::to_string(player.deckSize) + "/" +
                             std::to_string(player.deckSize);
    text.at(3).text = deckString;
    shuffleDeck();
    for (int i = 0; i < 5; i++) {
      drawCard();
    }

    genMap(10, 10);
  }

  int findNeighbours(int x, int y) {
    int neighbours = 0;

    if (x > 0 && map.at(x - 1).at(y).exists) {
      neighbours += 1;
    }
    if (x < map.size() - 1 && map.at(x + 1).at(y).exists) {
      neighbours += 1;
    }
    if (y > 0 && map.at(x).at(y - 1).exists) {
      neighbours += 1;
    }
    if (y < map.size() - 1 && map.at(x).at(y + 1).exists) {
      neighbours += 1;
    }

    return neighbours;
  }

  int spawnRoomAround(int y, int x) {
    std::vector<Direction> directions;
    if (y > 0 && !map.at(y - 1).at(x).exists) {
      directions.push_back(Direction::UP);
    }

    if (x < map.at(y).size() - 1 && !map.at(y).at(x + 1).exists) {
      directions.push_back(Direction::RIGHT);
    }

    if (y < map.size() - 1 && !map.at(y + 1).at(x).exists) {
      directions.push_back(Direction::DOWN);
    }

    if (x > 0 && !map.at(y).at(x - 1).exists) {
      directions.push_back(Direction::LEFT);
    }

    if (directions.empty()) {
      return 0;
    }

    int centreX = map.at(0).size() / 2;
    int centreY = map.size() / 2;

    int xdist = x - centreX;
    int ydist = y - centreY;

    Direction closestDirToCenter;

    if (std::abs(xdist) > std::abs(ydist)) {
      closestDirToCenter = (xdist > 0) ? Direction::LEFT : Direction::RIGHT;
    } else {
      closestDirToCenter = (ydist > 0) ? Direction::UP : Direction::DOWN;
    }

    std::vector<Direction> candidates;

    for (Direction dir : directions) {
      if (dir != closestDirToCenter) {
        candidates.push_back(dir);
      }
    }

    // if no alternatives exist, allow the original directions
    if (candidates.empty()) {
      candidates = directions;
    }

    std::uniform_int_distribution<std::size_t> dist(0, candidates.size() - 1);

    Direction randomDir = candidates.at(dist(g));

    // spawn the room
    switch (randomDir) {
    case Direction::UP:
      map.at(y - 1).at(x).exists = true;
      map.at(y - 1).at(x).x = x;
      map.at(y - 1).at(x).y = y - 1;
      break;

    case Direction::RIGHT:
      map.at(y).at(x + 1).exists = true;
      map.at(y).at(x + 1).x = x + 1;
      map.at(y).at(x + 1).y = y;
      break;

    case Direction::DOWN:
      map.at(y + 1).at(x).exists = true;
      map.at(y + 1).at(x).x = x;
      map.at(y + 1).at(x).y = y + 1;
      break;

    case Direction::LEFT:
      map.at(y).at(x - 1).exists = true;
      map.at(y).at(x - 1).x = x - 1;
      map.at(y).at(x - 1).y = y;
      break;
    }
    return 1;
  }

  std::vector<int> getRandomRoom() {
    std::vector<std::vector<int>> roomCoords;
    for (int i = 0; i < map.size(); i++) {
      for (int j = 0; j < map.at(i).size(); j++) {
        if (map.at(i).at(j).exists) {
          roomCoords.push_back({i, j});
        }
      }
    }
    std::uniform_int_distribution<int> dist(0, roomCoords.size() - 1);
    return roomCoords.at(dist(g));
  }

  void genMap(int sizex, int sizey) {
    for (int i = 0; i < sizey; i++) {
      std::vector<Room> row;
      for (int j = 0; j < sizex; j++) {
        row.push_back(Room(false));
      }
      map.push_back(row);
    }

    player.x = sizex / 2;
    player.y = sizey / 2;
    map.at(player.y).at(player.x).exists = true;
    map.at(player.y).at(player.x).x = player.x;
    map.at(player.y).at(player.x).y = player.y;
    // generate map from middle
    int maxRooms = (sizex * sizey) / 3;
    int rooms = 0;
    int px = player.x;
    int py = player.y;
    while (rooms < maxRooms) {
      if (findNeighbours(px, py) < 3) {
        rooms += spawnRoomAround(px, py);
      }
      std::vector<int> roomCoords = getRandomRoom();
      px = roomCoords.at(0);
      py = roomCoords.at(1);
    }
    for (int i = 0; i < map.size(); i++) {
      for (int j = 0; j < map.at(i).size(); j++) {
        if (map.at(i).at(j).x == player.x && map.at(i).at(j).y == player.y) {
          std::cout << "X";
        } else if (map.at(i).at(j).exists) {
          std::cout << "@";
        } else {
          std::cout << " ";
        }
      }
      std::cout << std::endl;
    }
  }

  void addCards(std::string card, int amount) {
    for (int i = 0; i < amount; i++) {
      player.deck.push_back(Card(card));
    }
  }

  void shuffleDeck() {
    std::shuffle(player.deck.begin(), player.deck.end(), g);
  }

  void drawObjects(float x, float y) {
    BeginDrawing();
    ClearBackground(WHITE);
    for (int i = 0; i < ui.size(); i++) {
      ui.at(i).draw();
    }
    for (int i = 0; i < player.hand.size(); i++) {
      player.hand.at(i).draw();
      if (CheckCollisionPointRec({x, y}, {xrel(player.hand.at(i).x),
                                          yrel(player.hand.at(i).y),
                                          xrel(player.hand.at(i).width),
                                          yrel(player.hand.at(i).height)})) {
        player.hand.at(i).hover();
      }
    }
    text.at(3).text = "deck: " + std::to_string(player.deck.size()) + "/" +
                      std::to_string(player.deckSize);
    for (int i = 0; i < text.size(); i++) {
      text.at(i).draw();
    }

    // draw map
    for (int i = 0; i < map.size(); i++) {
      for (int j = 0; j < map.at(i).size(); j++) {
        if (map.at(i).at(j).exists) {
          map.at(i).at(j).draw(player.x, player.y, map);
        }
      }
    }
    // draw player
    DrawTexturePro(textures["player map"],
                   {0, 0, (float)textures["player map"].width,
                    (float)textures["player map"].height},
                   {xrel(416 + 8), yrel(43 + 8), xrel(16), yrel(16)},
                   {xrel(8), yrel(8)}, static_cast<int>(player.direction) * 90,
                   WHITE);

    // draw cursor
    DrawTexturePro(textures["cursor"],
                   {0, 0, (float)textures["cursor"].width,
                    (float)textures["cursor"].height},
                   {x, y, xrel(12), yrel(12)}, {0, 0}, 0, WHITE);
    EndDrawing();
  }

  void deinit() {
    for (const auto &[key, value] : textures) {
      UnloadTexture(value);
    }
    // UnloadSound(mystery_soundtrack);
  }

  void drawCard() {
    if (player.deck.size() == 0) {
      return;
    } else if (player.hand.size() == 7) {
      return;
    }
    player.hand.push_back(player.deck.at(player.deck.size() - 1));
    player.hand.back().x =
        480 - (player.hand.back().width * (player.hand.size()));
    player.deck.pop_back();
  }

  void click(float x, float y) {
    if (CheckCollisionPointRec({x, y},
                               {xrel(ui.at(4).x), yrel(ui.at(4).y),
                                xrel(ui.at(4).width), yrel(ui.at(4).height)})) {
      drawCard();
      passTurn();
    }
    for (int i = 0; i < player.hand.size(); i++) {
      if (CheckCollisionPointRec({x, y}, {xrel(player.hand.at(i).x),
                                          yrel(player.hand.at(i).y),
                                          xrel(player.hand.at(i).width),
                                          yrel(player.hand.at(i).height)})) {
        player.hand.at(i).use(*this);
        return;
      }
    }
  }

  void discard(Card *card) {
    for (int i = 0; i < player.hand.size(); i++) {
      if (&player.hand[i] == card) {
        player.hand.erase(player.hand.begin() + i);
        break;
      }
    }
    for (int i = 0; i < player.hand.size(); i++) {
      player.hand.at(i).x = 480 - (player.hand.at(i).width * (i + 1));
    }
    std::cout << "card discarded" << std::endl;
  }

  void passTurn() {
    turnCount += 1;
    int dealCount = turnCount % 5 + 1;
    text.at(1).text = "deal " + std::to_string(dealCount) + "/5";
    if (dealCount == 1)
      while (player.hand.size() < 7) {
        drawCard();
      }
  }
};

Direction oppositeDirection(Direction direction) {
  if (direction == Direction::UP) {
    return Direction::DOWN;
  }
  if (direction == Direction::DOWN) {
    return Direction::UP;
  }
  if (direction == Direction::RIGHT) {
    return Direction::LEFT;
  }
  if (direction == Direction::LEFT) {
    return Direction::RIGHT;
  }
  return Direction::UP;
}

std::string directionToString(Direction dir) {
  switch (dir) {
  case Direction::UP:
    return "UP";
  case Direction::RIGHT:
    return "RIGHT";
  case Direction::DOWN:
    return "DOWN";
  case Direction::LEFT:
    return "LEFT";
  }
  return "UNKNOWN";
}

void moveDirection(Direction direction, Game &game) {
  std::cout << "Moving " << directionToString(direction) << std::endl;
  if (direction == Direction::UP && game.player.y > 0 &&
      game.map.at(game.player.y - 1).at(game.player.x).exists) {
    game.player.y -= 1;
  } else if (direction == Direction::DOWN &&
             game.player.y < game.map.size() - 1 &&
             game.map.at(game.player.y + 1).at(game.player.x).exists) {
    game.player.y += 1;
  } else if (direction == Direction::RIGHT &&
             game.player.x < game.map.at(0).size() - 1 &&
             game.map.at(game.player.y).at(game.player.x + 1).exists) {
    game.player.x += 1;
  } else if (direction == Direction::LEFT && game.player.x > 0 &&
             game.map.at(game.player.y).at(game.player.x - 1).exists) {
    game.player.x -= 1;
  }
}

void Card::use(Game &game) {
  for (int i = 0; i < actions.size(); i++) {
    switch (actions.at(i).action) {
    case Actions::MOVE_CHOICE:
      std::cout << "move choice" << std::endl;
      break;
    case Actions::MOVE_FORWARD:
      for (int j = 0; j < actions.at(i).value; j++) {
        moveDirection(game.player.direction, game);
      }
      break;
    case Actions::MOVE_BACKWARD:
      game.player.direction = oppositeDirection(game.player.direction);
      for (int j = 0; j < actions.at(i).value; j++) {
        moveDirection(game.player.direction, game);
      }
      game.player.direction = oppositeDirection(game.player.direction);
      break;
    case Actions::TURN_LEFT:
      game.player.direction = static_cast<Direction>(
          (static_cast<int>(game.player.direction) - 1) % 4);
      std::cout << "Just turned left"
                << directionToString(game.player.direction) << std::endl;
      break;
    case Actions::TURN_RIGHT:
      game.player.direction = static_cast<Direction>(
          (static_cast<int>(game.player.direction) + 1) % 4);
      std::cout << "Just turned right"
                << directionToString(game.player.direction) << std::endl;
      break;
    default:
      break;
    }
  }
  std::cout << "Current direction: " << directionToString(game.player.direction)
            << std::endl;
  game.discard(this);
};

int main(void) {
  // SetTraceLogLevel(LOG_NONE);
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
      {"connection horizontal",
       LoadTexture("assets/minimap/connection_horizontal.png")},
      {"connection vertical",
       LoadTexture("assets/minimap/connection_vertical.png")},
      {"enemy map", LoadTexture("assets/minimap/enemy.png")},
      {"player map", LoadTexture("assets/minimap/player.png")},
      {"bomb map", LoadTexture("assets/minimap/bomb.png")},

      // Card related
      {"scroll", LoadTexture("assets/ui/scroll.png")},

      {"bomb_scroll", LoadTexture("assets/scroll_icons/bomb_scroll.png")},
      {"defend_scroll", LoadTexture("assets/scroll_icons/defend_scroll.png")},
      {"heal_heart_scroll",
       LoadTexture("assets/scroll_icons/heal_heart_scroll.png")},
      {"insight_scroll", LoadTexture("assets/scroll_icons/insight_scroll.png")},
      {"melee_scroll", LoadTexture("assets/scroll_icons/melee_scroll.png")},
      {"movement_scroll",
       LoadTexture("assets/scroll_icons/movement_scroll.png")},
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
      {"middle_middle",
       LoadTexture("assets/ui/sizeable_scroll/middle_middle.png")},
      {"bottom_middle",
       LoadTexture("assets/ui/sizeable_scroll/bottom_middle.png")},
      {"top_right", LoadTexture("assets/ui/sizeable_scroll/top_right.png")},
      {"middle_right",
       LoadTexture("assets/ui/sizeable_scroll/middle_right.png")},
      {"bottom_right",
       LoadTexture("assets/ui/sizeable_scroll/bottom_right.png")},

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
      {"slash", LoadTexture("assets/fonts/slash.png")}};

  actionTextures = {{Actions::MOVE_CHOICE, textures["move_scroll"]},
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
                    {Actions::FREEZE, textures["potion_scroll"]}};

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
