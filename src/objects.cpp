#inlude "objects.h"
#include "card.h"

float xrel(float n) {
  return n / 480.0f * WIDTH;
}

float yrel(float n) {
  return n / 300.0f * HEIGHT;
}

class Game {
public:
  std::vector<Card> hand;
  std::vector<Card> deckList;
  std::vector<UIelement> ui;
  std::vector<Text> text;

  UIelement screen{"ui/screen.png", 0, 0, 0, 480, 300};
  UIelement dungeon{"env/dungeon.png", 56, 8, 0, 288, 144};
  UIelement frame{"ui/frame.png", 46, 0, 0, 309, 160};
  UIelement deck{"ui/scroll_deck.png", 409, 172, 0, 70, 30};
  UIelement actionBar{"ui/action_bar.png", 384, 102, 0, 80, 16};
  UIelement drawIcon{"ui/draw_icon.png", 382, 100, 0, 20, 20};
  UIelement dealIcon{"ui/deal_icon.png", 382, 120, 0, 20, 20};
  UIelement lookIcon{"ui/look_icon.png", 382, 140, 0, 20, 20};

  Text drawText{"draw 1 turn", 402, 107};
  Text dealText{"deal 1/5", 403, 127};
  Text lookText{"look 1 turn", 403, 147};

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

    // init font textures
    for (char letter = 'a'; letter <= 'z'; letter++) {
      std::string path = "assets/fonts/" + std::string(1, letter) + ".png";
      letters[letter] = LoadTexture(path.c_str());
    }

    for (char letter = '0'; letter <= '9'; letter++) {
      std::string path = "assets/fonts/" + std::string(1, letter) + ".png";
      letters[letter] = LoadTexture(path.c_str());
    }

    letters.insert({'/', LoadTexture("assets/fonts/slash.png")});

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
    for (const auto& [key, font] : letters) {
      UnloadTexture(font);
    }
    // unload cursor
    UnloadTexture(cursor);
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

class UIelement {
public:
  float x, y;
  float width, height;
  float rotation;
  Texture2D texture;

  UIelement(std::string image, int x, int y, int rotation, int width, int height) {
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
  std::string text;
  std::string id;
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

      Texture2D texture = letters.at(c);

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
