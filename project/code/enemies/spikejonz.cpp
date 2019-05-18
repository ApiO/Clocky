#include "enemies.h"

#include <runtime/trace.h>
#include <engine/pge.h>
#include <units/clocky.h>

using namespace pge;


// Consts
namespace
{
  using namespace game;

  // Attack parameters
  const i32 ATTACK_POINTS = 2;
  const f32 ATTACK_RANGE = 40.f;
  const f32 ATTACK_CD = .5f;
}

namespace game
{
  Spikejonz::Spikejonz() :EnemyBase(10.f), state(NONE){};

  void Spikejonz::update(f64 delta_time, glm::vec3 player_position)
  {
    glm::vec3 position;
    unit::get_local_position(world, unit, 0, position);

    switch (state)
    {
    case NONE:
      if (point_in_circle(position, ATTACK_RANGE, player_position)){
        state = ATTACK;
        state_time = ATTACK_CD;
        glm::vec2 direction = glm::normalize(glm::vec2(player_position - position));
        clocky::recieve_damage(direction, ATTACK_POINTS);
      }
      break;
    case ATTACK:
      state_time -= delta_time;
      if (state_time <= 0.f)
        state = NONE;
      break;
    }
  }

}