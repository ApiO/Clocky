#include <game_camera.h>
#include <engine/pge.h>
#include <glm/glm.hpp>
#include <math.h>
#include <runtime/array.h>
#include <runtime/memory.h>
#include <glm/gtx/projection.hpp>
#include <glm/gtx/random.hpp>
#include <game_types.h>

namespace
{
  using namespace pge;

  void get_closest_point(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &p, glm::vec3 &r, f32 &d2)
  {
    f32 d2b;
    const glm::vec3 tmp = b - a;
    const float dotProduct = glm::dot(tmp, p - a);

    const float percAlongLine = dotProduct / glm::length2(tmp);
    if (percAlongLine  < 0.0f || percAlongLine  > 1.0f) {
      // Point isn't within the line segment
      d2  = glm::distance2(p, a);
      d2b = glm::distance2(p, b);
      if (d2 < d2b) {
        r = a;
      } else {
        r  = b;
        d2 = d2b;
      }
      return;
    }
    r  = (a + (percAlongLine * (b - a)));
    d2 = glm::distance2(r, p);
  }
}

namespace game
{
  using namespace pge;

  namespace game_camera
  {
    void init(GameCamera &c, u64 world, const glm::vec2 &resolution, Allocator &a)
    {
      c.rfov = DEFAULT_FOV * (f32)(atan(1.0)*4.0) / 180.0f; // precise fov in radians
      f32 tz = resolution.y / (2 * tan(c.rfov / 2)); // z for 1:1 render scale

      c.world = world;
      c.id    = world::spawn_camera(world, resolution.x / resolution.y, glm::vec3(0, 0, tz), glm::quat(1, 0, 0, 0));
      c.ratio = resolution.x / resolution.y;
      c.mode  = GameCamera::FREE;
      c.path_data = MAKE_NEW(a, Array<f32>, a);

      camera::set_projection_type(world, c.id, PROJECTION_PERSPECTIVE);
      camera::set_vertical_fov(world, c.id, DEFAULT_FOV);
      camera::set_near_range(world, c.id, DEFAULT_NEAR);
      camera::set_far_range(world, c.id, DEFAULT_FAR);
    }

    void set_target(GameCamera &c, const glm::vec2 &t)
    {
      c.target = t;
    }

    void clamp_to_box(GameCamera &c, const glm::vec2 &min, const glm::vec2 &max)
    {
      c.mode      = GameCamera::CLAMPED_TO_BOX;
      c.clamp_min = min;
      c.clamp_max = max;
    }

    void follow_path(GameCamera &c, const glm::vec3 *path, u32 path_size, f32 duration)
    {
      c.mode        = GameCamera::FOLLOW_PATH_CONST;
      c.path        = path;
      c.path_size   = path_size;
      c.effect_time = 0;
      c.path_head   = 1;
      array::resize(*c.path_data, path_size);

      Array<f32> &time = *c.path_data;

      time[0] = 0.0f;
      f32 distance = 0;

      for (u32 i = 1; i < path_size; i++) {
        // store the distance into the time to avoid recalculating it just after
        time[i] = glm::distance(path[i], path[i - 1]);
        distance += time[i];
      }

      for (u32 i = 1; i < path_size; i++) {
        // for now, time[i] is the distance from the privious point
        time[i] = time[i - 1] + (duration * time[i] / distance);
        // now it's the time for the path point
      }
    }

    void follow_path(GameCamera &c, const glm::vec3 *path, u32 path_size, const f32 *key_time, const f32 *key_value, u32 num_keys)
    {
      c.mode        = GameCamera::FOLLOW_PATH_CURVE;
      c.path        = path;
      c.path_size   = path_size;
      c.key_time    = key_time;
      c.key_value   = key_value;
      c.num_keys    = num_keys;
      c.effect_time = 0;
      c.path_head   = 1;
      c.key_head    = 1;
      array::resize(*c.path_data, path_size);

      Array<f32> &weigth = *c.path_data;

      weigth[0] = 0.0f;
      f32 distance = 0;

      for (u32 i = 1; i < path_size; i++) {
        // store the distance into the weight to avoid recalculating it just after
        weigth[i] = glm::distance(path[i], path[i - 1]);
        distance += weigth[i];
      }

      for (u32 i = 1; i < path_size; i++) {
        // for now, time[i] is the distance from the privious point
        weigth[i] = weigth[i-1] + weigth[i]/distance;
        // now it's the weight of the path point
      }
    }

    void clamp_to_path(GameCamera &c, const glm::vec3 *path, u32 path_size)
    {
      c.mode        = GameCamera::CLAMPED_TO_PATH;
      c.path        = path;
      c.path_size   = path_size;
      array::resize(*c.path_data, path_size);
    }

    void shake(GameCamera &c, const glm::vec2 &dir, f32 frequency, f32 magnitude, f32 time, 
               f32 radial_frequency, f32 radial_magnitude)
    {
      c.mode = GameCamera::SHAKE;
      c.effect_time     = 0;
      c.effect_duration = time;
      c.shake_freq      = frequency;
      c.shake_mag       = magnitude;
      c.shake_dir       = glm::normalize(dir);
      c.shake_rad_freq  = radial_frequency;
      c.shake_rad_mag   = radial_magnitude;
    }

    void free(GameCamera &c)
    {
      c.mode = GameCamera::FREE;
    }

    void update(GameCamera &c, f64 delta_time)
    {
      glm::vec3 cp;
      glm::vec3 tp, tps, tpp;

      Array<f32> &path_data = *c.path_data;

      f32  tmp, min;

      camera::get_local_translation(c.world, c.id, cp);

      tp = glm::vec3(c.target.x, c.target.y, cp.z);
      c.effect_time += delta_time;

      switch (c.mode) {
        case GameCamera::CLAMPED_TO_BOX: {
          glm::vec2 margin;
          glm::vec2 correction;

          margin.y = tan(c.rfov / 2) * cp.z;
          margin.x = c.ratio * margin.y;

          correction = c.clamp_min - ((glm::vec2)tp - margin);
          if (correction.x > 0)
            tp.x += correction.x;
          if (correction.y > 0)
            tp.y += correction.y;

          correction = (glm::vec2)tp + margin - c.clamp_max;
          if (correction.x > 0)
            tp.x -= correction.x;
          if (correction.y > 0)
            tp.y -= correction.y;
        }
        case GameCamera::FOLLOW_PATH_CONST:
          while (c.effect_time >= path_data[c.path_head] && c.path_head <= c.path_size)
            c.path_head++;
          if (c.path_head >= c.path_size) {
            c.mode = GameCamera::FREE;
            break;
          }
          tp = glm::mix(c.path[c.path_head - 1], c.path[c.path_head], 
                        (f32)(c.effect_time - path_data[c.path_head - 1]) / (path_data[c.path_head] - path_data[c.path_head - 1]));
          break;
        case GameCamera::FOLLOW_PATH_CURVE:
          while (c.effect_time >= c.key_time[c.key_head] && c.key_head <= c.num_keys)
            c.key_head++;
          if (c.key_head >= c.num_keys) {
            c.mode = GameCamera::FREE;
            break;
          }

          tmp = c.key_time [c.key_head] - c.key_time [c.key_head- 1];

          // compute the wanted path weight
          tmp = glm::mix(c.key_value[c.key_head - 1], c.key_value[c.key_head], (f32)(c.effect_time - c.key_time[c.key_head - 1]) / tmp);

          if (c.key_value[c.key_head] - c.key_value[c.key_head- 1] > 0) {
            // forward
            while (tmp > path_data[c.path_head])
              ++c.path_head;
          } else {
            // backward
            while (tmp < path_data[c.path_head - 1])
              --c.path_head;
          }

          tp = glm::mix(c.path[c.path_head - 1], c.path[c.path_head], 
                        (tmp - path_data[c.path_head - 1]) / (path_data[c.path_head] - path_data[c.path_head - 1]));
          break;
        case GameCamera::CLAMPED_TO_PATH:
          tps   = tp;
          tps.z = 0;
          min   = FLT_MAX;
          for (u32 i = 1; i < c.path_size; i++) {
            get_closest_point(c.path[i - 1], c.path[i], tps, tpp, tmp);
            if (tmp < min) {
              tp  = tpp;
              min = tmp;
            }
          }
          break;
        case GameCamera::SHAKE:
          if (c.effect_time >= c.effect_duration) {
            c.mode = GameCamera::FREE;
            break;
          }
          tp  = glm::vec3(c.target, 0);
          tpp = glm::vec3(c.shake_dir, 0);
          tps = tpp * (f32)((c.shake_mag * (c.effect_duration - c.effect_time) / c.effect_duration) * sin(2 * PI * c.effect_time * c.shake_freq));

          if (c.shake_rad_mag > 0 && c.shake_rad_freq > 0) {
            tpp.x = tpp.y;
            tpp.y = c.shake_dir.x;
            tps += tpp * (f32)((c.shake_rad_mag * (c.effect_duration - c.effect_time) / c.effect_duration) * cos(2 * PI * c.effect_time * c.shake_rad_freq));
          }

          break;
        default:
          break;
      }
      tp.z = cp.z;
      cp += (tp - cp) * DEFAULT_DAMPING * (f32)delta_time;

      if (c.mode == GameCamera::SHAKE)
        cp += tps;

      camera::set_local_translation(c.world, c.id, cp);
    }

    void destroy(GameCamera &c)
    {
      MAKE_DELETE(*c.path_data->_allocator, Array<f32>, c.path_data);
      world::despawn_camera(c.world, c.id);
    }
  }
}