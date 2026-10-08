#include <unordered_map>
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

typedef struct icon {
  float x, y;
  float width, height;
  float rotation;
  Texture2D texture;
} icon;

enum class ActionType {
  // Movement         //
  Walk,               // Move 1 in any direction
  Run,                // Move 2 forwards
  Dodge,              // Move 2 backwards
  RightTurn,          // Turn right, move one forward
  LeftTurn,           // Turn left, move one forward
  // Melee            //
  Stab,               // Deals 2 damage to enemies in front
  Slash,              // Deals 1 damage to adjacent enemies
  Defend,             // You do not take damage for the next 2 turns
  Parry,              // Ignore defence for 1 turn, deal 1 damage to enemies which attacked you that turn
  // Ranged           //
  QuickDraw,          // Deal 1 damage to all enemies in front
  Aim,                // Re-orient where you're facing and deal 1 damage to enemy closest to where you're not facing
  // Potions          //
  LavaVial,           // Deal 3 burn damage to the enemy in front of you, 1 use
  FrostVial,          // The enemy in front of you is unable to act for the next 3 turns., 1 use
  VenomVial,          // Deal 1 poison to the enemy in front of you, 1 use
  InvisibilityPotion, // Move 2 forwards, can pass through enemies, 1 use
  VitalityPotion,     // Heal 1 heart, 1 use
  InvigoratingPotion, // Remove all negative status from you and give you speed, 1 use
  MugsWater,          // Draw 1 card, 1 use
  BogWater,           // Draw 1 ranged card, 1 use
  StillWater,         // Draw 1 melee card, 1 use
  Rainwater,          // Draw 1 movement card, 1 use
  // Spells           //
  MageCage,           // Chosen adjacent enemy is unable to act for the next 3 turns, 1 use
  ManaRegen,          // Heals 1 per turn until you move, 1 use
  CosmicSparks,       // Explode any bombs you have discovered, 1 use
  FlameWall,          // Deals 2 burn to chosen adjacent enemy, 1 use
  MagicMusic,         // Move an enemy two spaces in any chosen direction, 1 use
  TombSmash,          // Deal 3 damage to the enemy in front and stuns them, 1 use
  HocusFocus,         // Gives you "focus"
  // Resources        //
  Wood,               // Stuns enemy in front of you for 1 turn
  Stone,              // Stuns closest enemy in front of you for 1 turn
  Ruby,               // Looks pretty
  Gold,               // Valuable
  Gem,                // Looks pretty
  Iron,               // Stuns chosen enemy adjacent to you for 1 turn
  Copper,             // Stuns chosen enemy adjacent to you for 1 turn
  Natrium,            // Stuns chosen enemy adjacent to you for 1 turn
  Sulfur,             // Deal 1 poison to enemy in front of you and you
  Bomb,               // Place a bomb with a fuse of 3 turns behind in any empty position adjacent to you. Can be used 2 times before gone
  Apple,              // Draw 1 card and heal 2, 1 use
  GildedBoots         // Move 2 in any direction, 4 uses
};

struct Action {
  ActionType type;
  int value;
};

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
  float height = yrel(95);
  float width = xrel(64);
  float x;
  float y;
  std::vector<icon> icons;

  void draw(void) {
    DrawTexturePro(
      texture,
      {0,0, (float)texture.width, (float)texture.height},
      {xrel(x), yrel(y), xrel(width), yrel(height)},
      {0, 0},
      rotation,
      WHITE
    );
  }

  void use() {
  }
};


class Game {
public:
  std::vector<Card> hand;
  std::vector<Card> deck;
  std::vector<GameObject> objects;

  void init(void) {
    GameObject deck("ScrollDeck.png");
    deck.width = 32;
    deck.height = 76;
    deck.x = 500;
    deck.y = 205;
    deck.rotation = 90;
    objects.push_back(deck);
  }

  void drawObjects(void) {
    BeginDrawing();
    ClearBackground(WHITE);
    for (int i = 0; i < objects.size(); i++) {
      objects.at(i).draw();
    }
    EndDrawing();
  }

  void deinit() {
    for (int i = 0; i < objects.size(); i++) {
      UnloadTexture(objects.at(i).texture);
    }
  }

  void draw_card() {
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
