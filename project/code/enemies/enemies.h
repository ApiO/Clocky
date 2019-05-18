#pragma once

#include <glm/glm.hpp>
#include <runtime/types.h>
#include <runtime/trace.h>

#include <game_types.h>
#include <behaviors/behaviors.h>

namespace game
{
  const pge::i32 WALKER_LIFETIME_GAIN = 4;
  const pge::i32 FLYBOT_LIFETIME_GAIN = 2;
}

namespace game
{
  struct EnemyBase
  {
    EnemyBase(pge::f64 d);
    pge::i32 hp;
    pge::f64 despawn_time;
    pge::u64 world;
    pge::u64 unit;
    bool     look_right;
    void update(pge::f64 delta_time, glm::vec3 player_position);
    void recieve_damage(const glm::vec2 &direction, pge::i32 damage);
  };


  struct Walker : EnemyBase
  {
    enum State
    {
      NONE = 0,
      ATTACK,
      HIT,
      STUN,
      IDLE,
      WAIT,
      FLEE,
      PATROL,
      TRACK,
      DEAD
    };
    Walker();
    void update (pge::f64 delta_time, glm::vec3 player_position);
    void recieve_damage (const glm::vec2 &direction, pge::i32 damage);
    State      state;
    pge::f64   state_time;
    pge::u64   hitbox;
    pge::u64   mover;
    MoverData  mover_data;
    MoverState mover_state;
    glm::vec2  velocity;
    glm::vec2  hit;
  };

  struct Flybot : EnemyBase
  {
    enum State
    {
      NONE = 0,
      ATTACK,
      HIT,
      STUN,
      FOCUS,
      IDLE,
      DEAD
    };
    Flybot();
    void update(pge::f64 delta_time, glm::vec3 player_position);
    void recieve_damage(const glm::vec2 &direction, pge::i32 damage);
    State     state;
    pge::f64  state_time;
    pge::u64  hitbox;
    glm::vec2 velocity;
    glm::vec2 hit;
  };

  struct Spikejonz : EnemyBase
  {
    enum State
    {
      NONE = 0,
      ATTACK
    };
    Spikejonz();
    void update(pge::f64 delta_time, glm::vec3 player_position);
    State    state;
    pge::f64 state_time;
  };

  inline void set_look_direction(EnemyBase &base, bool look_right)
  {
    if (base.look_right == look_right) return;

    glm::quat q;
    pge::unit::get_local_rotation(base.world, base.unit, 0, q);

    glm::vec3 a(glm::degrees(glm::eulerAngles(q)));
    a.y += (look_right ? -1 : 1) * 180.f;
    
    pge::unit::set_local_rotation(base.world, base.unit, 0, glm::quat(glm::radians(a)));

    base.look_right = look_right;
  }

}

namespace game
{
  inline EnemyBase::EnemyBase (pge::f64 d) : despawn_time(d){}
  inline void EnemyBase::update (pge::f64 delta_time, glm::vec3 player_position){
    (void)delta_time;
    (void)player_position;
  }
  inline void EnemyBase::recieve_damage(const glm::vec2 &direction, pge::i32 damage){
    (void)direction;
    (void)damage;
  }
}