#pragma once

#include <unordered_map>
#include <raylib.h>
#include "objects.h"

std::unordered_map<char, Texture2D> letters;

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

Texture2D texture = LoadTexture("assets/ui/scroll.png");

Texture2D top_left      = LoadTexture("assets/ui/sizeable_scroll/top_left.png");
Texture2D middle_left   = LoadTexture("assets/ui/sizeable_scroll/middle_left.png");
Texture2D bottom_left   = LoadTexture("assets/ui/sizeable_scroll/bottom_left.png");
Texture2D top_middle    = LoadTexture("assets/ui/sizeable_scroll/top_middle.png");
Texture2D middle_middle = LoadTexture("assets/ui/sizeable_scroll/middle_middle.png");
Texture2D bottom_middle = LoadTexture("assets/ui/sizeable_scroll/bottom_middle.png");
Texture2D top_right     = LoadTexture("assets/ui/sizeable_scroll/top_right.png");
Texture2D middle_right  = LoadTexture("assets/ui/sizeable_scroll/middle_right.png");
Texture2D bottom_right  = LoadTexture("assets/ui/sizeable_scroll/bottom_right.png");

Texture2D bomb_scroll       = LoadTexture("assets/scroll_icons/bomb_scroll.png");
Texture2D defend_scroll     = LoadTexture("assets/scroll_icons/defend_scroll.png");
Texture2D heal_heart_scroll = LoadTexture("assets/scroll_icons/heal_heart_scroll.png");
Texture2D insight_scroll    = LoadTexture("assets/scroll_icons/insight_scroll.png");
Texture2D melee_scroll      = LoadTexture("assets/scroll_icons/melee_scroll.png");
Texture2D movement_scroll   = LoadTexture("assets/scroll_icons/movement_scroll.png");
Texture2D move_scroll       = LoadTexture("assets/scroll_icons/move_scroll.png");
Texture2D potion_scroll     = LoadTexture("assets/scroll_icons/potion_scroll.png");
Texture2D ranged_scroll     = LoadTexture("assets/scroll_icons/ranged_scroll.png");
Texture2D spell_scroll      = LoadTexture("assets/scroll_icons/spell_scroll.png");
Texture2D turn_scroll       = LoadTexture("assets/scroll_icons/turn_scroll.png");

std::array<Texture2D, static_cast<int>(Actions::COUNT)> actionTextures = {
  move_scroll, // "move in any chosen direction by X",
  move_scroll, // "move forwards by X",
  move_scroll, // "move backwards by X",
  move_scroll, // "turn left",
  move_scroll, // "turn right",
  melee_scroll, // "damage an enemy in front",
  melee_scroll, // "damage an enemy adjacent",
  defend_scroll, // "you do not take damage for X turns",
  defend_scroll, // "deal X damage to an enemy that attacked you",
  potion_scroll, // "deal X burn damage to an enemy in front",
  potion_scroll, // "the enemy in front of you is frozen for X turns"
};

class Card;
