#include <engine/pge.h>
#include <runtime/trace.h>

#include "enemies.h"
#include <units/clocky.h>

using namespace pge;

// Consts
namespace
{
  using namespace game;

  // enemy params
  const i32 HP = 4;
  const f32 DESPAN_SPEED = 0.3f;
  const f32 STUN_DURATION = .5f;
  const f32 HIT_ADVANCE = clocky::ATTACK_ADVANCE;
  const f32 HIT_ADVANCE_DURATION = (f32)clocky::ATTACK_ADVANCE_DURATION;

  const f32 UNIT_WIDTH = 40.0f;
  const f32 UNIT_HEIGHT = 80.0f;

  // Run parameter
  const f32 MOVE_SPEED = 200.f;

  // Attack parameters
  const i32 ATTACK_POINTS = 2;
  const f32 ATTACK_RANGE = 80.f;
  const f32 ATTACK_CONE = PI * .8f;
  const f32 ATTACK_CD = 1.f;

  // Aggro parameters
  //const f32 DETECT_ANGLE = PI * .25f;
  const f32 DETECT_RANGE = 280.f;
  const f32 DELTA_WIDTH = (clocky::UNIT_WIDTH + UNIT_WIDTH)*.5f;

  // Mover parameters
  const f32 FALL_SPEED = 2048.0f;
  const f32 FALL_ACCELERATION_DURATION = 1.0f;

  // IA
  const f32 FLEE_UNTIL = 10.f;  // % chance to flee
  const f32 ATTACK_RATE = 30.f; // % chance to attack
  const f32 IDLE_ABOVE = FLEE_UNTIL + ATTACK_RATE;
  const f32 PATROL_RATE = 10.f;

  const i32 IDLE_DURATION_MIN = 30;
  const i32 IDLE_DURATION_MAX = 60;
  const i32 FLEE_DURATION_MIN = 10;
  const i32 FLEE_DURATION_MAX = 30;
  const i32 PATROL_DURATION_MIN = 10;
  const i32 PATROL_DURATION_MAX = 30;
  const f32 PATROL_SPEED = MOVE_SPEED * .25f;


  inline bool player_found_and_reachable(const Walker &walker, const glm::vec3 &position, const glm::vec3 &player_position)
  {
    if (!point_in_circle(position, DETECT_RANGE, player_position))
      return false;

    if (!player_visible(walker.world, position, player_position))
      return false;

    glm::vec3 to = position;
    to.x += UNIT_WIDTH * .5f * (walker.look_right ? 1 : -1);
    to.y -= UNIT_HEIGHT;

    return ground_found(walker.world, position, to);
  }

}

// enemy definition
namespace game
{
  void Walker::recieve_damage(const glm::vec2 &direction, pge::i32 damage)
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

  void Walker::update(f64 delta_time, glm::vec3 player_position)
  {
    glm::vec3 position;
    unit::get_local_position(world, unit, 0, position);

    switch (state) {
    case NONE:
      if (point_in_circle(position, DETECT_RANGE, player_position)) {
        if (point_in_cone(position, ATTACK_CONE, ATTACK_RANGE, player_position)) {
          i32 rand = randomize(0, 100);
          if (rand < FLEE_UNTIL) {
            state = FLEE;
            state_time = randomize(FLEE_DURATION_MIN, FLEE_DURATION_MAX) * FRAME_DURATION;
            set_look_direction(*this, position.x < player_position.x);
            velocity.x = (look_right ? -1 : 1) * MOVE_SPEED;
          } else if (rand >= IDLE_ABOVE) {
            state = WAIT;
            state_time = randomize(IDLE_DURATION_MIN, IDLE_DURATION_MAX) * FRAME_DURATION;
            velocity.x = 0;
          } else {
            state = WAIT;
            state_time = ATTACK_CD;
            velocity.x = 0;

            glm::vec2 direction = glm::normalize(glm::vec2(player_position - position));
            clocky::recieve_damage(direction, ATTACK_POINTS);
          }
        } else if (player_visible(world, position, player_position)) {

          glm::vec3 to = position;
          to.x += UNIT_WIDTH * .5f * (look_right ? 1 : -1);
          to.y -= UNIT_HEIGHT;

          if (ground_found(world, position, to)) {
            state = TRACK;
            state_time = 0;
          }
        }
      } else {
        i32 rand = randomize(0, 100);
        bool patrol = rand < PATROL_RATE;

        if (patrol) {
          set_look_direction(*this, rand % 2 == 0);

          glm::vec3 to = position;
          to.x += UNIT_WIDTH * .5f * (look_right ? 1 : -1);
          to.y -= UNIT_HEIGHT;

          if (!ground_found(world, position, to)) {
            set_look_direction(*this, !look_right);

            to.x += UNIT_WIDTH * (look_right ? 1 : -1);
            if (!ground_found(world, position, to))
              patrol = false;
          }
        }

        if (!patrol) {
          state = IDLE;
          state_time = randomize(IDLE_DURATION_MIN, IDLE_DURATION_MAX) * FRAME_DURATION;
        } else {
          state = PATROL;
          state_time = randomize(PATROL_DURATION_MIN, PATROL_DURATION_MAX) * FRAME_DURATION;
          velocity.x = (look_right ? -1 : 1) * PATROL_SPEED;
        }
      }
      break;
    case TRACK:
    {
      set_look_direction(*this, position.x < player_position.x);
      run(velocity.x, position, player_position, MOVE_SPEED);

      bool track = player_found_and_reachable(*this, position, player_position);

      if (track) {
        if (point_in_circle(position, DETECT_RANGE, player_position)) {
          i32 range = randomize((i32)DELTA_WIDTH, (i32)ATTACK_RANGE);
          track = !point_in_cone(position, ATTACK_CONE, (f32)range, player_position);
        }
      }

      if (!track) {
        state = NONE;
        velocity.x = 0;
      }
    }
      break;
    case WAIT:
      state_time -= delta_time;
      if (state_time <= 0.f)
        state = NONE;
      break;
    case HIT:
      if (state_time < HIT_ADVANCE_DURATION) {
        mover_state = MOVER_STATE_MANUAL;
        apply_hit_bump(velocity, state_time, HIT_ADVANCE, HIT_ADVANCE_DURATION, hit);
        state_time += delta_time;
      } else {
        mover_state = MOVER_STATE_FALLING;
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
    case STUN: case IDLE: case PATROL:
      state_time -= delta_time;
      if (state_time <= 0.f)
        state = NONE;
      break;
    case FLEE:
    {
      state_time -= delta_time;

      if (state_time <= 0.f) {
        state = NONE;
        velocity.x = 0;
        break;
      }

      glm::vec3 to = position;
      to.x += UNIT_WIDTH * .5f * (velocity.x >= 0.f ? 1 : -1);
      to.y -= UNIT_HEIGHT;

      if (!ground_found(world, position, to)) {
        velocity.x = -velocity.x;
      } else {
        set_look_direction(*this, position.x < player_position.x);
        velocity.x = (look_right ? -1 : 1) * MOVE_SPEED;
      }
    }
      break;
    case DEAD:
      despawn_time -= delta_time;
      break;
    default:
      OUTPUT("state not handle : %d", state);
    }

    // finalize mover
    handle_ground_mover(world, mover, velocity, mover_data, delta_time);
  }

  Walker::Walker() : EnemyBase(DESPAN_SPEED),
    state(NONE), mover_state(MOVER_STATE_FALLING), state_time(0), velocity(0), mover_data(FALL_SPEED, FALL_ACCELERATION_DURATION) { //, state_duration(-1)
    hp = HP;
    look_right = false;
  };
}