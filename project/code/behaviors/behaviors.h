#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <engine/pge.h>
#include <game_types.h>

// Data structs

namespace game
{
  enum MoverState
  {
    MOVER_STATE_GROUNDED = 0,
    MOVER_STATE_FALLING,
    MOVER_STATE_MANUAL,
  };

  struct MoverData
  {
    MoverData(pge::f32 fs, pge::f32 fad);
    pge::f32   fall_speed;
    pge::f32   fall_acc_duration;
    pge::f64   time;
    MoverState state;
    bool       pinned;
  };

  extern bool     _player_found;
  extern pge::u64 player_unit;
  extern pge::u64 player_hitbox;
  extern pge::u64 raycast_to_player;

  extern bool      _ground_found;
  extern glm::vec2 ground_dir;
  extern pge::f32  ground_dist;
  extern pge::u64  raycast_to_ground;
}

// Behaviors func

namespace game
{
  inline bool player_visible(pge::u64 world, const glm::vec3 &from, const glm::vec3 &to)
  {
    pge::physics::cast_raycast(world, raycast_to_player, from, to);
    return _player_found;
  }

  inline bool ground_found(pge::u64 world, const glm::vec3 &from, const glm::vec3 &to)
  {
    pge::physics::cast_raycast(world, raycast_to_ground, from, to);
    return _ground_found;
  }

  inline bool point_in_circle(const glm::vec3 &center, const pge::f32 radius, const glm::vec3 &point){
    return glm::distance(center, point) < radius;
  }

  inline bool point_in_cone(const glm::vec3 &center, const pge::f32 angle, const pge::f32 radius, const glm::vec3 &point)
  {
    pge::f32 d = glm::distance(center, point);
    pge::f32 a = glm::angle(center, point);  // TODO: cjeck resul c'est chelou

    return d <= radius && a <= angle;
  }

  inline void run(pge::f32 &velocity_x, const glm::vec3 &from, const glm::vec3 &to, const pge::f32 &move_speed){
    velocity_x = ((from.x - to.x < 0) ? 1 : -1) * move_speed;
  }


  inline void apply_hit_bump(glm::vec2 &velocity, pge::f64 state_time,
                             const pge::f32 &HIT_ADVANCE, const pge::f32 &HIT_ADVANCE_DURATION,
                             const glm::vec2 &HIT)
  {
    velocity = HIT * HIT_ADVANCE / HIT_ADVANCE_DURATION;
    if (state_time > HIT_ADVANCE_DURATION) {
      velocity -= (pge::f32)(state_time - HIT_ADVANCE_DURATION) * HIT * HIT_ADVANCE / (pge::f32)HIT_ADVANCE_DURATION;
    }
  }

  void handle_ground_mover(pge::u64 world, pge::u64 mover, glm::vec2 &velocity, MoverData &data, const pge::f64 delta_time);
}