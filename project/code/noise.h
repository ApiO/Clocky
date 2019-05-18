#pragma once

#include <engine/pge.h>
#include <runtime/memory.h>
#include <runtime/array.h>
#include <math.h>

namespace game
{
  using namespace pge;

  struct Noise
  {
    Array<u64> *sprites;
    u64 world;
    f64 time;
    f32 freq;
    i32 toggle;
  };

  namespace noise
  {
    void init   (Noise &noise, u64 world, const glm::vec2 &size, f32 frequency, Allocator &a);
    void update (Noise &noise, f64 delta_time);
    void free   (Noise &noise);
  }

  namespace noise
  {
    inline void init(Noise &noise, u64 world, const glm::vec2 &size, f32 frequency, Allocator &a)
    {
      const i32 TEXTURE_SIZE = 256;
      i32 i, rows, cols;
      const glm::vec2 offset(TEXTURE_SIZE/2 - size.x/2, TEXTURE_SIZE/2 - (size.y / 2));

      noise.freq = frequency;
      noise.toggle = 0;
      noise.time = 0;
      noise.world = world;
      noise.sprites = MAKE_NEW(a, Array<u64>, a);

      cols = ceil(size.x / TEXTURE_SIZE);
      rows = ceil(size.y / TEXTURE_SIZE);

      array::resize(*noise.sprites, rows * cols);

      i = 0;
      for (i32 c = 0; c < cols; c++) {
        for (i32 r = 0; r < rows; r++) {
          (*noise.sprites)[i] = world::spawn_sprite(world, "fx/noise", glm::vec3(c * TEXTURE_SIZE + offset.x, r * TEXTURE_SIZE + offset.y, -0.5) , IDENTITY_ROTATION);
          ++i;
        } 
      }
    }

    inline void update(Noise &noise, f64 delta_time)
    {
      Array<u64> &sprites = *noise.sprites;

      noise.time += delta_time;
      if (noise.time <= noise.freq)
        return;

      noise.time = 0;
      noise.toggle = !noise.toggle;

      for (u32 i = 0; i < array::size(sprites); i++)
        sprite::set_local_scale(noise.world, sprites[i], glm::vec3(noise.toggle ? 1 : -1, noise.toggle ? 1 : -1, 1));
    }

    inline void free(Noise &noise)
    {
      Array<u64> &sprites = *noise.sprites;

      for (u32 i = 0; i < array::size(sprites); i++)
        world::despawn_sprite(noise.world, sprites[i]);

      MAKE_DELETE(*noise.sprites->_allocator, Array<u64>, noise.sprites);
    }
  }
}