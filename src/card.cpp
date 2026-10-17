#include "card.h"
#include "objects.h"
#include "raylib.h"

class Card {
public:
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
  void use(Game& game) {
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
  }
};
