#pragma once

#include <game_types.h>
#include <enemy_controller.h>

namespace game
{

#define NUM_GHOSTS 8

  struct Clocky {
    pge::u64 _world;
    pge::u64 _unit;
    pge::u64 _mover;
    pge::u64 _hitbox;
    pge::u64 _box;
    pge::u64 _atp_sphere;
    pge::u64 _impact_text;
    pge::u64 _atp_ghosts[NUM_GHOSTS];
    pge::f32 _atp_ghost_cooldown;
    pge::f32 _dash_cooldown;
    pge::u64 _ground_rcast;
    pge::Array<pge::u64> *fx;
    EnemyController *enemies;

    // states

    bool cmd[NUM_COMMANDS];

    bool ascending;
    pge::f32  fly_from;

    bool      orientation;
    DTapOrientation dtap_orientation;
    DTapOrientation last_oriented_dtap;
    bool      should_move;
    bool      pinned;
    bool      about_to_attack;
    bool      can_air_dash;
    bool      hitted;
    glm::vec2 last_hit_dir;
    State     state;
    pge::f64  state_time;
    pge::f64  attack_cooldown;
    pge::f64  dtap_cooldown;
    glm::vec2 ground_dir;
    pge::f32  ground_dist;
    glm::vec2 attack_dir;
    glm::vec2 aiming_dir;
    glm::vec2 velocity;
    pge::u32  pad_index;
    pge::f32  lifetime;
    pge::i32  combo_hits;
    pge::f64  combo_countdown;
    pge::i32  combo_multiplier;
    pge::i32  num_overlapping_platforms;

    pge::Hash<Target> *close_targets;
    pge::u64           best_target;
  };

}