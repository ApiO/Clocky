#include <string>
#include <engine/pge.h>
#include <runtime/memory.h>

using namespace pge;

namespace game
{
  void init();
  void update(f64 delta_time);
  void render();
  void shutdown();

  char data_path[_MAX_PATH];
}
inline void handle_args(int argc, char * argv[])
{
  if (argc == 1){
    strcpy(game::data_path, "../data");
  return;
  }

  strcpy(game::data_path, argv[1]);
}

int main(int argc, char * argv[])
{
  handle_args(argc, argv);

  memory_globals::init();

  Allocator *a = &memory_globals::default_allocator();
  {
    application::init(game::init, game::update, game::render, game::shutdown, game::data_path, *a);

    while (!application::should_quit())
      application::update();

    application::shutdown();
  }
  memory_globals::shutdown();
}
