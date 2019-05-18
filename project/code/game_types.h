#pragma once

#include <engine/pge_types.h>

#define PI   3.14159265359f
#define PIX2 6.28318530718f
#define FRAME_DURATION .0166666666666667f

namespace game 
{
  struct Target {
    glm::vec2 position;
    glm::vec2 direction;
    pge::f32  distance;
  };

  enum Command
  {
    CMD_JUMP,
    CMD_ATTACK,
    CMD_DASH,
    NUM_COMMANDS
  };

  enum State
  {
    STATE_GROUNDED = 0,
    STATE_ASCENDING,
    STATE_FALLING,
    STATE_SLIDING,
    STATE_FLYING,
    STATE_DASHING,
    STATE_ATP,
    STATE_ATTACK,
    STATE_STUNNED
  };

  enum DTapOrientation
  {
    DTAP_NONE,
    DTAP_UP,
    DTAP_DOWN,
    DTAP_LEFT,
    DTAP_RIGHT
  };

  inline int randomize(int min, int max)
  {
    return (rand() % (max - min + 1)) + min;
  }
}