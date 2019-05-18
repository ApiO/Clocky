#include <engine/pge.h>
#include <engine/pge_types.h>
#include <runtime/trace.h>

#include "enemies.h"
#include <units/clocky.h>

#include <stdio.h>

using namespace pge;

// Consts
namespace
{
  using namespace game;

  const i32 HP = 2;
  const f32 DESPAN_SPEED = 0.3f;
  const f32 STUN_DURATION = .5f;
  const f32 HIT_ADVANCE = clocky::ATTACK_ADVANCE;
  const f32 HIT_ADVANCE_DURATION = (f32)clocky::ATTACK_ADVANCE_DURATION;
  const f32 BULLET_SPEED = 3;
  const f32 UNIT_WIDTH = 100.0f;

  // Attack parameters
  const i32 ATTACK_POINTS = 1;
  const f32 ATTACK_CD = 1.5f;

  // Aggro parameters
  const f32 DETECT_RANGE = 500.f;
  const f32 DELTA_WIDTH = (clocky::UNIT_WIDTH + UNIT_WIDTH)*.5f;

  // IA
  const i32 FOCUS_DURATION_MIN = 10;
  const i32 FOCUS_DURATION_MAX = 30;
}

namespace game
{
  void Flybot::recieve_damage(const glm::vec2 &direction, pge::i32 damage)
  {
    hp -= damage;
    hit = direction;

    if (hp < 1) {
      state = DEAD;
      actor::clear_kinematic(world, hitbox);
      actor::add_impulse(world, hitbox, glm::vec3(direction, 0));
      actor::set_collision_filter(world, hitbox, "corpse");
    } else {
      state = HIT;
      state_time = 0;
    }
  }

  void Flybot::update(f64 delta_time, glm::vec3 player_position)
  {
    glm::vec3 position;
    unit::get_local_position(world, unit, 0, position);

    switch (state) {
    case NONE:
      if (point_in_circle(position, DETECT_RANGE, player_position)) {
        if (!player_visible(world, position, player_position)) return;
        state = ATTACK;
        state_time = ATTACK_CD;
        glm::vec2 v = glm::normalize(glm::vec2(player_position - position));

        u64 bullet = world::spawn_unit(world, "units/bullet/bullet", position, IDENTITY_ROTATION);
        u64 bullet_actor = unit::actor(world, bullet, 0);

        actor::set_velocity(world, bullet_actor, glm::vec3(v * BULLET_SPEED, 0));
      }
      break;
    case ATTACK:
      state_time -= delta_time;
      if (state_time <= 0.f) {
        state = FOCUS;
        state_time = randomize(FOCUS_DURATION_MIN, FOCUS_DURATION_MAX) * FRAME_DURATION;
      }
      break;
    case FOCUS:
      state_time -= delta_time;
      if (state_time <= 0.f)
        state = NONE;
      else if ((look_right &&  position.x > player_position.x) ||
               (!look_right &&  position.x < player_position.x)) {
        set_look_direction(*this, position.x < player_position.x);

        break;
    case HIT:
      if (state_time < HIT_ADVANCE_DURATION) {
        apply_hit_bump(velocity, state_time, HIT_ADVANCE, HIT_ADVANCE_DURATION, hit);
        state_time += delta_time;
      } else {
        velocity = glm::vec2(0);
        if (STUN_DURATION > 0.f) {
          state = STUN;
          state_time = STUN_DURATION;
        } else {
          state = NONE;
          state_time = 0;
        }
      }
      break;
    case STUN:
      state_time -= delta_time;
      if (state_time <= 0.f)
        state = NONE;
      break;
      }
    }
    unit::set_local_position(world, unit, 0, position + glm::vec3(velocity * (f32)delta_time, 0));
  }

  Flybot::Flybot() : EnemyBase(DESPAN_SPEED),
    state(NONE), state_time(0), velocity(0) {
    hp = HP;
    look_right = false;
  };
}