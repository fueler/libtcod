
#include <libtcod.h>
#include <libtcod/pathfinding/astar.h>
#include <libtcod/pathfinding/breadth-first.h>
#include <libtcod/pathfinding/dijkstra.h>
#include <libtcod/pathfinding/hill-climb.h>
#include <libtcod/tileset_fallback.h>
#include <libtcod/utility/matrix.h>

#include <climits>
#include <cstddef>
#include <iostream>
#include <libtcod.hpp>
#include <random>
#include <string>
#include <utility>

#include "catch.hpp"

namespace std {
ostream& operator<<(ostream& out, const array<ptrdiff_t, 2>& data) {
  return out << '{' << data.at(0) << ',' << ' ' << data.at(1) << '}';
}
}  // namespace std

const int WIDTH = 20;
const int HEIGHT = 10;
const char* TITLE = "Unittest";

std::string get_file(const std::string& path) {
  static const char* DEFAULT_DIR = "data";
  const char* data_dir = std::getenv("DATA_DIR");
  if (!data_dir) {
    data_dir = DEFAULT_DIR;
  }
  return std::string(data_dir) + "/" + path;
}

std::string to_string(const TCOD_Console& console) {
  std::string result;
  for (int y = 0; y < console.h; ++y) {
    if (y != 0) {
      result += '\n';
    }
    for (int x = 0; x < console.w; ++x) {
      result += static_cast<char>(console.at(y, x).ch);
    }
  }
  return result;
}

void test_renderer(TCOD_renderer_t renderer) {
  TCOD_console_set_custom_font(
      get_file("fonts/terminal8x8_gs_ro.png").c_str(), TCOD_FONT_LAYOUT_CP437 | TCOD_FONT_TYPE_GREYSCALE, 0, 0);
  tcod::console::init_root(WIDTH, HEIGHT, TITLE, 0, renderer);
  REQUIRE(TCOD_console_get_width(NULL) == WIDTH);
  REQUIRE(TCOD_console_get_height(NULL) == HEIGHT);
  TCODConsole::flush();
  TCOD_console_delete(NULL);
}

TEST_CASE("SDL2 Renderer") { test_renderer(TCOD_RENDERER_SDL2); }
TEST_CASE("SDL Renderer", "[!nonportable]") { test_renderer(TCOD_RENDERER_SDL); }
TEST_CASE("OPENGL Renderer", "[!nonportable]") { test_renderer(TCOD_RENDERER_OPENGL); }
TEST_CASE("GLSL Renderer", "[!nonportable]") { test_renderer(TCOD_RENDERER_SDL); }
TEST_CASE("OPENGL2 Renderer", "[!nonportable]") { test_renderer(TCOD_RENDERER_OPENGL2); }

TEST_CASE("Console ascii") {
  TCODConsole console = TCODConsole(5, 1);
  console.print(0, 0, "Test");
  CHECK(to_string(*console.get_data()) == "Test ");
}

TEST_CASE("Console print") {
  TCODConsole console = TCODConsole(5, 1);
  console.print(0, 0, std::string("plop"));
  CHECK(to_string(*console.get_data()) == "plop ");
}
TEST_CASE("Console print empty") {
  TCODConsole console = TCODConsole(5, 1);
  console.print(0, 0, std::string(""));
}

TEST_CASE("Console eascii") {
  TCODConsole console = TCODConsole(2, 1);
  const char test_str[] = {static_cast<char>(0xff), static_cast<char>(0x00)};
  console.print(0, 0, test_str);
  CHECK(console.getChar(0, 0) == 0xff);
  CHECK(console.getChar(1, 0) == 0x20);
}

TEST_CASE("Console UTF-8 BMP") {
  TCODConsole console = TCODConsole(2, 1);
  console.printf(0, 0, "☃");
  CHECK(console.getChar(0, 0) == 0x2603);
  CHECK(console.getChar(1, 0) == 0x20);
}

TEST_CASE("Console UTF-8 SMP") {
  TCODConsole console = TCODConsole(2, 1);
  console.printf(0, 0, "🌍");
  CHECK(console.getChar(0, 0) == 0x1F30D);
  CHECK(console.getChar(1, 0) == 0x20);
}

TEST_CASE("Console wchar BMP", "[!nonportable]") {
  TCODConsole console = TCODConsole(2, 1);
  console.print(0, 0, L"\u2603");
  CHECK(console.getChar(0, 0) == 0x2603);
  CHECK(console.getChar(1, 0) == 0x20);
}

/* Fails when sizeof(wchar_t) == 2 */
TEST_CASE("Console wchar SMP", "[!nonportable][!mayfail][!hide]") {
  TCODConsole console = TCODConsole(2, 1);
  console.print(0, 0, L"\U0001F30D");
  CHECK(console.getChar(0, 0) == 0x1F30D);
  CHECK(console.getChar(1, 0) == 0x20);
}

TEST_CASE("New Dijkstra") {
  using Catch::Matchers::Equals;
  using index_type = std::array<ptrdiff_t, 2>;
  auto dist = tcod::Vector2<int>(3, 2, INT_MAX);
  auto path_map = tcod::Vector2<index_type>(3, 2);
  tcod::pathfinding::path_clear(path_map);
  dist.atf(0, 0) = 0;
  tcod::Vector2<int> cost{
      {1, 1, 1},
      {0, 1, 2},
  };
  tcod::pathfinding::dijkstra_compute(dist, cost, 2, 3, &path_map);
  const tcod::Vector2<int> EXPECTED_DIST{
      {0, 2, 4},
      {INT_MAX, 3, 7},
  };
  CHECK(dist == EXPECTED_DIST);
  const tcod::Vector2<index_type> EXPECTED_PATH_MAP{
      {{0, 0}, {0, 0}, {0, 1}},
      {{1, 0}, {0, 0}, {1, 1}},
  };
  CHECK(path_map == EXPECTED_PATH_MAP);
  const std::vector<index_type> EXPECTED_TRAVEL_PATH{{1, 1}, {0, 0}};
  CHECK_THAT(tcod::pathfinding::get_path(path_map, {1, 2}), Equals(EXPECTED_TRAVEL_PATH));
}
TEST_CASE("Pathfinder Benchmarks", "[benchmark]") {
  const int SIZE = 50;
  tcod::Matrix<uint8_t, 2> plain_cost({SIZE, SIZE}, 1);
  tcod::pathfinding::SimpleGraph2D<decltype(plain_cost)> plain_graph(plain_cost);
  BENCHMARK("Old Dijkstra 50x50") {
    TCOD_Map* map = TCOD_map_new(SIZE, SIZE);
    TCOD_map_clear(map, 1, 1);
    TCOD_Dijkstra* dijkstra = TCOD_dijkstra_new(map, 1.5f);
    TCOD_dijkstra_compute(dijkstra, 0, 0);
    TCOD_dijkstra_delete(dijkstra);
    TCOD_map_delete(map);
  }
  BENCHMARK("Breadth-first 50x50") {
    tcod::Matrix<int, 2> map({SIZE, SIZE}, std::numeric_limits<int>::max());
    tcod::Matrix<int8_t, 2> cost({SIZE, SIZE}, 1);
    tcod::pathfinding::breadth_first2d(map, cost, {{0, 0}});
    CHECK(map.at({SIZE - 1, SIZE - 1}) == SIZE - 1);
  }
  BENCHMARK("Breadth-first (graph) 50x50") {
    tcod::Matrix<int, 2> map({SIZE, SIZE}, std::numeric_limits<int>::max());
    tcod::pathfinding::breadth_first(map, plain_graph, {{0, 0}});
    CHECK(map.at({SIZE - 1, SIZE - 1}) == SIZE - 1);
    CHECK(tcod::pathfinding::simple_hillclimb(map, plain_graph, {SIZE - 1, SIZE - 1}).size() == SIZE);
  }
  BENCHMARK("New Dijkstra 50x50") {
    tcod::Matrix<int, 2> map({SIZE, SIZE}, std::numeric_limits<int>::max());
    tcod::Matrix<int8_t, 2> cost({SIZE, SIZE}, 1);
    map.at({0, 0}) = 0;
    tcod::pathfinding::dijkstra2d(map, cost);
    CHECK(map.at({SIZE - 1, SIZE - 1}) == SIZE - 1);
  }
  BENCHMARK("New Dijkstra (graph) 50x50") {
    tcod::Matrix<int, 2> map({SIZE, SIZE}, std::numeric_limits<int>::max());
    map.at({0, 0}) = 0;
    tcod::pathfinding::dijkstra(map, plain_graph);
    CHECK(map.at({SIZE - 1, SIZE - 1}) == SIZE - 1);
    CHECK(tcod::pathfinding::simple_hillclimb(map, plain_graph, {SIZE - 1, SIZE - 1}).size() == SIZE);
  }
  BENCHMARK("Advanced Dijkstra 50x50") {
    using index_type = std::array<ptrdiff_t, 2>;
    auto dist = tcod::Vector2<int>(SIZE, SIZE, INT_MAX);
    auto path_map = tcod::Vector2<index_type>(SIZE, SIZE);
    tcod::pathfinding::path_clear(path_map);
    dist.at({0, 0}) = 0;
    auto cost = tcod::Vector2<int>(SIZE, SIZE, 1);
    tcod::pathfinding::dijkstra_compute(dist, cost, 2, 3, &path_map);
  }
  BENCHMARK("Old A* 50x50") {
    TCOD_Map* map = TCOD_map_new(SIZE, SIZE);
    TCOD_map_clear(map, 1, 1);
    TCOD_Path* astar = TCOD_path_new_using_map(map, 1.0f);
    TCOD_path_compute(astar, 0, 0, SIZE - 1, SIZE - 1);
    TCOD_path_delete(astar);
    TCOD_map_delete(map);
  }
  BENCHMARK("New A* 50x50") {
    tcod::Matrix<int, 2> map({SIZE, SIZE}, std::numeric_limits<int>::max());
    tcod::Matrix<int8_t, 2> cost({SIZE, SIZE}, 1);
    map.at({0, 0}) = 0;
    tcod::pathfinding::astar2d(map, cost, {SIZE - 1, SIZE - 1});
    CHECK(map.at({SIZE - 1, SIZE - 1}) == SIZE - 1);
  }
  BENCHMARK("New A* (graph) 50x50") {
    tcod::Matrix<int, 2> map({SIZE, SIZE}, std::numeric_limits<int>::max());
    map.at({0, 0}) = 0;
    tcod::pathfinding::astar(map, plain_graph, {SIZE - 1, SIZE - 1});
    CHECK(map.at({SIZE - 1, SIZE - 1}) == (SIZE - 1));
    CHECK(tcod::pathfinding::simple_hillclimb(map, plain_graph, {SIZE - 1, SIZE - 1}).size() == SIZE);
  }
}

TEST_CASE("Fallback font.", "[!mayfail]") { REQUIRE(tcod::tileset::new_fallback_tileset()); }

void test_alignment(TCOD_alignment_t alignment) {
  // Compare alignment between the new and old functions.
  int x = GENERATE(0, 1, 2, 3, 4, 5, 6);
  int width = GENERATE(11, 12);
  INFO("x=" << x << ", width=" << width);
  TCOD_Console* console1 = TCOD_console_new(width, 1);
  TCOD_Console* console2 = TCOD_console_new(width, 1);
  SECTION("Print text.") {
    for (auto& tile : *console1) {
      tile.ch = static_cast<int>('.');
    }
    for (auto& tile : *console2) {
      tile.ch = static_cast<int>('.');
    }
    TCOD_console_print_ex(console1, x, 0, TCOD_BKGND_NONE, alignment, "123");
    TCOD_console_printf_ex(console2, x, 0, TCOD_BKGND_NONE, alignment, "123");
    CHECK(to_string(*console1) == to_string(*console2));
  }
  SECTION("Print rect.") {}
  TCOD_console_delete(console1);
  TCOD_console_delete(console2);
}
TEST_CASE("Left alignment regression.") { test_alignment(TCOD_LEFT); }
TEST_CASE("Center alignment regression.") { test_alignment(TCOD_CENTER); }
TEST_CASE("Right alignment regression.") { test_alignment(TCOD_RIGHT); }
TEST_CASE("Rectangle text alignment.") {
  auto console = tcod::new_console(12, 1);
  // TCOD_Console* console = TCOD_console_new(12, 1);
  for (auto& tile : *console) {
    tile.ch = static_cast<int>('.');
  }
  tcod::print_rect(*console, 0, 0, 0, 0, "123", nullptr, nullptr, TCOD_BKGND_NONE, TCOD_LEFT);
  tcod::print_rect(*console, 0, 0, 0, 0, "123", nullptr, nullptr, TCOD_BKGND_NONE, TCOD_CENTER);
  tcod::print_rect(*console, 0, 0, 0, 0, "123", nullptr, nullptr, TCOD_BKGND_NONE, TCOD_RIGHT);
  CHECK(to_string(*console) == "123.123..123");
}
TEST_CASE("Load BDF.") { REQUIRE(tcod::load_bdf(get_file("fonts/ucs-fonts/4x6.bdf"))); }
TEST_CASE("Print color codes.") {
  using namespace std::string_literals;
  auto console = tcod::new_console(8, 1);
  std::string text = "1\u0006\u0001\u0002\u00032\u00083"s;
  tcod::print(*console, 0, 0, text, &TCOD_white, &TCOD_black, TCOD_BKGND_SET, TCOD_LEFT);
  REQUIRE(to_string(*console) == "123     ");
  CHECK(console->at(0, 0).fg.r == 255);
  CHECK(console->at(0, 0).fg.g == 255);
  CHECK(console->at(0, 0).fg.b == 255);
  CHECK(console->at(0, 0).fg.a == 255);
  CHECK(console->at(0, 1).fg.r == 1);
  CHECK(console->at(0, 1).fg.g == 2);
  CHECK(console->at(0, 1).fg.b == 3);
  CHECK(console->at(0, 1).fg.a == 255);
  CHECK(console->at(0, 2).fg.r == 255);
  CHECK(console->at(0, 2).fg.g == 255);
  CHECK(console->at(0, 2).fg.b == 255);
  CHECK(console->at(0, 2).fg.a == 255);
}
TEST_CASE("Color code formatting.") {
  using namespace std::string_literals;
  auto console = tcod::new_console(3, 3);
  std::string text = "1\u0006\u0001\u0002\u0003\n2 \u0008\n 3"s;
  for (auto& tile : *console) {
    tile.ch = static_cast<int>('.');
  }
  tcod::print(*console, 0, 0, text, &TCOD_white, &TCOD_black, TCOD_BKGND_SET, TCOD_LEFT);
  REQUIRE(to_string(*console) == ("1..\n2 .\n 3."));
}
TEST_CASE("Malformed UTF-8.", "[!throws]") {
  auto console = tcod::new_console(8, 1);
  std::string text = "\x80";
  REQUIRE_THROWS(tcod::print(*console, 0, 0, text, &TCOD_white, &TCOD_black, TCOD_BKGND_SET, TCOD_LEFT));
}
TEST_CASE("Heap test.") {
  struct TCOD_Heap heap;
  TCOD_heap_init(&heap, sizeof(int));
  std::vector<int> INPUT{0, 1, 2, 3, 4, 5, 6, 7};
  for (int& it : INPUT) {
    TCOD_minheap_push(&heap, it, &it);
  }
  std::vector<int> output;
  while (heap.size) {
    int out;
    TCOD_minheap_pop(&heap, &out);
    output.emplace_back(out);
  }
  REQUIRE(INPUT == output);
  TCOD_heap_uninit(&heap);
}

TEST_CASE("Noise Benchmarks", "[benchmark]") {
  TCOD_Random* rng = TCOD_random_new_from_seed(TCOD_RNG_MT, 0);
  TCOD_Noise* noise1d = TCOD_noise_new(1, TCOD_NOISE_DEFAULT_HURST, TCOD_NOISE_DEFAULT_LACUNARITY, rng);
  TCOD_Noise* noise2d = TCOD_noise_new(2, TCOD_NOISE_DEFAULT_HURST, TCOD_NOISE_DEFAULT_LACUNARITY, rng);
  TCOD_Noise* noise3d = TCOD_noise_new(3, TCOD_NOISE_DEFAULT_HURST, TCOD_NOISE_DEFAULT_LACUNARITY, rng);
  TCOD_Noise* noise4d = TCOD_noise_new(4, TCOD_NOISE_DEFAULT_HURST, TCOD_NOISE_DEFAULT_LACUNARITY, rng);
  const float POINT[4] = {0.5f, 0.5f, 0.5f, 0.5f};
  TCOD_noise_get_ex(noise1d, POINT, TCOD_NOISE_WAVELET);  // Pre-generate wavelet data.
  TCOD_noise_get_ex(noise2d, POINT, TCOD_NOISE_WAVELET);
  TCOD_noise_get_ex(noise3d, POINT, TCOD_NOISE_WAVELET);
  BENCHMARK("Perlin 1D") { TCOD_noise_get_ex(noise1d, POINT, TCOD_NOISE_PERLIN); }
  BENCHMARK("Perlin 2D") { TCOD_noise_get_ex(noise2d, POINT, TCOD_NOISE_PERLIN); }
  BENCHMARK("Perlin 3D") { TCOD_noise_get_ex(noise3d, POINT, TCOD_NOISE_PERLIN); }
  BENCHMARK("Perlin 4D") { TCOD_noise_get_ex(noise4d, POINT, TCOD_NOISE_PERLIN); }
  BENCHMARK("Simplex 1D") { TCOD_noise_get_ex(noise1d, POINT, TCOD_NOISE_SIMPLEX); }
  BENCHMARK("Simplex 2D") { TCOD_noise_get_ex(noise2d, POINT, TCOD_NOISE_SIMPLEX); }
  BENCHMARK("Simplex 3D") { TCOD_noise_get_ex(noise3d, POINT, TCOD_NOISE_SIMPLEX); }
  BENCHMARK("Simplex 4D") { TCOD_noise_get_ex(noise4d, POINT, TCOD_NOISE_SIMPLEX); }
  BENCHMARK("Wavelet 1D") { TCOD_noise_get_ex(noise1d, POINT, TCOD_NOISE_WAVELET); }
  BENCHMARK("Wavelet 2D") { TCOD_noise_get_ex(noise2d, POINT, TCOD_NOISE_WAVELET); }
  BENCHMARK("Wavelet 3D") { TCOD_noise_get_ex(noise3d, POINT, TCOD_NOISE_WAVELET); }

  BENCHMARK("Simplex fbM 1D octaves=4") { TCOD_noise_get_fbm_ex(noise1d, POINT, 4, TCOD_NOISE_SIMPLEX); }
  BENCHMARK("Simplex fbM 2D octaves=4") { TCOD_noise_get_fbm_ex(noise2d, POINT, 4, TCOD_NOISE_SIMPLEX); }
  BENCHMARK("Simplex fbM 3D octaves=4") { TCOD_noise_get_fbm_ex(noise3d, POINT, 4, TCOD_NOISE_SIMPLEX); }
  BENCHMARK("Simplex fbM 4D octaves=4") { TCOD_noise_get_fbm_ex(noise4d, POINT, 4, TCOD_NOISE_SIMPLEX); }
  BENCHMARK("Simplex turbulence 1D octaves=4") { TCOD_noise_get_turbulence_ex(noise1d, POINT, 4, TCOD_NOISE_SIMPLEX); }
  BENCHMARK("Simplex turbulence 2D octaves=4") { TCOD_noise_get_turbulence_ex(noise2d, POINT, 4, TCOD_NOISE_SIMPLEX); }
  BENCHMARK("Simplex turbulence 3D octaves=4") { TCOD_noise_get_turbulence_ex(noise3d, POINT, 4, TCOD_NOISE_SIMPLEX); }
  BENCHMARK("Simplex turbulence 4D octaves=4") { TCOD_noise_get_turbulence_ex(noise4d, POINT, 4, TCOD_NOISE_SIMPLEX); }

  TCOD_noise_delete(noise4d);
  TCOD_noise_delete(noise3d);
  TCOD_noise_delete(noise2d);
  TCOD_noise_delete(noise1d);
  TCOD_random_delete(rng);
}
TEST_CASE("Noise Vectorized Benchmarks", "[benchmark]") {
  TCOD_Random* rng = TCOD_random_new_from_seed(TCOD_RNG_MT, 0);
  TCOD_Noise* noise1d = TCOD_noise_new(1, TCOD_NOISE_DEFAULT_HURST, TCOD_NOISE_DEFAULT_LACUNARITY, rng);
  TCOD_Noise* noise2d = TCOD_noise_new(2, TCOD_NOISE_DEFAULT_HURST, TCOD_NOISE_DEFAULT_LACUNARITY, rng);
  TCOD_Noise* noise3d = TCOD_noise_new(3, TCOD_NOISE_DEFAULT_HURST, TCOD_NOISE_DEFAULT_LACUNARITY, rng);
  TCOD_Noise* noise4d = TCOD_noise_new(4, TCOD_NOISE_DEFAULT_HURST, TCOD_NOISE_DEFAULT_LACUNARITY, rng);
  constexpr int ARRAY_SIZE = 1024;
  {
    const float POINT[4] = {0.5f, 0.5f, 0.5f, 0.5f};
    TCOD_noise_get_ex(noise1d, POINT, TCOD_NOISE_WAVELET);  // Pre-generate wavelet data.
    TCOD_noise_get_ex(noise2d, POINT, TCOD_NOISE_WAVELET);
    TCOD_noise_get_ex(noise3d, POINT, TCOD_NOISE_WAVELET);
  }
  float x[ARRAY_SIZE];
  float y[ARRAY_SIZE];
  float z[ARRAY_SIZE];
  float w[ARRAY_SIZE];
  float out[ARRAY_SIZE];
  BENCHMARK("Perlin 2Dx256") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      const float point[2] = {i * 0.1f, i * 0.1f};
      TCOD_noise_get_ex(noise2d, point, TCOD_NOISE_PERLIN);
    }
  }
  BENCHMARK("Perlin 2Dx256 vectorized") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      x[i] = y[i] = i * 0.1f;
    }
    TCOD_noise_get_vectorized(noise2d, TCOD_NOISE_PERLIN, ARRAY_SIZE, x, y, NULL, NULL, out);
  }
  BENCHMARK("Perlin 3Dx256") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      const float point[3] = {i * 0.1f, i * 0.1f, i * 0.1f};
      TCOD_noise_get_ex(noise3d, point, TCOD_NOISE_PERLIN);
    }
  }
  BENCHMARK("Perlin 3Dx256 vectorized") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      x[i] = y[i] = z[i] = i * 0.1f;
    }
    TCOD_noise_get_vectorized(noise3d, TCOD_NOISE_PERLIN, ARRAY_SIZE, x, y, z, NULL, out);
  }
  BENCHMARK("Perlin 4Dx256") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      const float point[4] = {i * 0.1f, i * 0.1f, i * 0.1f, i * 0.1f};
      TCOD_noise_get_ex(noise4d, point, TCOD_NOISE_PERLIN);
    }
  }
  BENCHMARK("Perlin 4Dx256 vectorized") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      x[i] = y[i] = z[i] = w[i] = i * 0.1f;
    }
    TCOD_noise_get_vectorized(noise4d, TCOD_NOISE_PERLIN, ARRAY_SIZE, x, y, z, w, out);
  }
  BENCHMARK("Simplex 2Dx256") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      const float point[2] = {i * 0.1f, i * 0.1f};
      TCOD_noise_get_ex(noise2d, point, TCOD_NOISE_SIMPLEX);
    }
  }
  BENCHMARK("Simplex 2Dx256 vectorized") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      x[i] = y[i] = i * 0.1f;
    }
    TCOD_noise_get_vectorized(noise2d, TCOD_NOISE_SIMPLEX, ARRAY_SIZE, x, y, NULL, NULL, out);
  }
  BENCHMARK("Simplex 3Dx256") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      const float point[3] = {i * 0.1f, i * 0.1f, i * 0.1f};
      TCOD_noise_get_ex(noise3d, point, TCOD_NOISE_SIMPLEX);
    }
  }
  BENCHMARK("Simplex 3Dx256 vectorized") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      x[i] = y[i] = z[i] = i * 0.1f;
    }
    TCOD_noise_get_vectorized(noise3d, TCOD_NOISE_SIMPLEX, ARRAY_SIZE, x, y, z, NULL, out);
  }
  BENCHMARK("Simplex 4Dx256") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      const float point[4] = {i * 0.1f, i * 0.1f, i * 0.1f, i * 0.1f};
      TCOD_noise_get_ex(noise4d, point, TCOD_NOISE_SIMPLEX);
    }
  }
  BENCHMARK("Simplex 4Dx256 vectorized") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      x[i] = y[i] = z[i] = w[i] = i * 0.1f;
    }
    TCOD_noise_get_vectorized(noise4d, TCOD_NOISE_SIMPLEX, ARRAY_SIZE, x, y, z, w, out);
  }
  BENCHMARK("Wavelet 2Dx256") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      const float point[2] = {i * 0.1f, i * 0.1f};
      TCOD_noise_get_ex(noise2d, point, TCOD_NOISE_WAVELET);
    }
  }
  BENCHMARK("Wavelet 2Dx256 vectorized") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      x[i] = y[i] = i * 0.1f;
    }
    TCOD_noise_get_vectorized(noise2d, TCOD_NOISE_WAVELET, ARRAY_SIZE, x, y, NULL, NULL, out);
  }
  BENCHMARK("Wavelet 3Dx256") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      const float point[3] = {i * 0.1f, i * 0.1f, i * 0.1f};
      TCOD_noise_get_ex(noise3d, point, TCOD_NOISE_WAVELET);
    }
  }
  BENCHMARK("Wavelet 3Dx256 vectorized") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      x[i] = y[i] = z[i] = i * 0.1f;
    }
    TCOD_noise_get_vectorized(noise3d, TCOD_NOISE_WAVELET, ARRAY_SIZE, x, y, z, NULL, out);
  }
  BENCHMARK("Perlin fBm 2Dx256 octaves=4") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      const float point[2] = {i * 0.1f, i * 0.1f};
      TCOD_noise_get_fbm_ex(noise2d, point, 4, TCOD_NOISE_PERLIN);
    }
  }
  BENCHMARK("Perlin fBm 2Dx256 octaves=4 vectorized") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      x[i] = y[i] = i * 0.1f;
    }
    TCOD_noise_get_fbm_vectorized(noise2d, TCOD_NOISE_PERLIN, 4, ARRAY_SIZE, x, y, NULL, NULL, out);
  }
  BENCHMARK("Perlin turbulence 2Dx256 octaves=4") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      const float point[2] = {i * 0.1f, i * 0.1f};
      TCOD_noise_get_turbulence_ex(noise2d, point, 4, TCOD_NOISE_PERLIN);
    }
  }
  BENCHMARK("Perlin turbulence 2Dx256 octaves=4 vectorized") {
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      x[i] = y[i] = i * 0.1f;
    }
    TCOD_noise_get_turbulence_vectorized(noise2d, TCOD_NOISE_PERLIN, 4, ARRAY_SIZE, x, y, NULL, NULL, out);
  }
  TCOD_noise_delete(noise4d);
  TCOD_noise_delete(noise3d);
  TCOD_noise_delete(noise2d);
  TCOD_noise_delete(noise1d);
  TCOD_random_delete(rng);
}

static tcod::MapPtr_ new_map_with_radius(int radius, bool start_transparent) {
  int size = radius * 2 + 1;
  tcod::MapPtr_ map{TCOD_map_new(size, size)};
  TCOD_map_clear(map.get(), start_transparent, 0);
  return map;
}
static tcod::MapPtr_ new_empty_map(int radius) { return new_map_with_radius(radius, true); }
static tcod::MapPtr_ new_opaque_map(int radius) { return new_map_with_radius(radius, false); }
static tcod::MapPtr_ new_corridor_map(int radius) {
  tcod::MapPtr_ map{new_map_with_radius(radius, false)};
  for (int i = 0; i < radius * 2 + 1; ++i) {
    TCOD_map_set_properties(map.get(), radius, i, true, true);
    TCOD_map_set_properties(map.get(), i, radius, true, true);
  }
  return map;
}
static tcod::MapPtr_ new_forest_map(int radius) {
  // Forest map with 1 in 4 chance of a blocking tile.
  std::mt19937 rng(0);
  std::uniform_int_distribution<int> chance(0, 3);
  tcod::MapPtr_ map{new_map_with_radius(radius, true)};
  for (int i = 0; i < map->nbcells; ++i) {
    if (chance(rng) == 0) {
      map->cells[i].transparent = false;
    }
  }
  return map;
}
TEST_CASE("FOV Benchmarks", "[benchmark]") {
  std::array<std::tuple<std::string, tcod::MapPtr_>, 16> test_maps{{
      {"empty_r4", new_empty_map(4)},
      {"empty_r10", new_empty_map(10)},
      {"empty_r50", new_empty_map(50)},
      {"empty_r300", new_empty_map(300)},
      {"opaque_r4", new_opaque_map(4)},
      {"opaque_r10", new_opaque_map(10)},
      {"opaque_r50", new_opaque_map(50)},
      {"opaque_r300", new_opaque_map(300)},
      {"corridor_r4", new_corridor_map(4)},
      {"corridor_r10", new_corridor_map(10)},
      {"corridor_r50", new_corridor_map(50)},
      {"corridor_r300", new_corridor_map(300)},
      {"forest_r4", new_forest_map(4)},
      {"forest_r10", new_forest_map(10)},
      {"forest_r50", new_forest_map(50)},
      {"forest_r300", new_forest_map(300)},
  }};
  for (auto& active_test : test_maps) {
    const std::string& map_name = std::get<0>(active_test);
    TCOD_Map* map = std::get<1>(active_test).get();
    const int radius = TCOD_map_get_width(map) / 2;
    BENCHMARK(map_name + " FOV_BASIC") { TCOD_map_compute_fov(map, radius, radius, 0, true, FOV_BASIC); };
    BENCHMARK(map_name + " FOV_DIAMOND") { TCOD_map_compute_fov(map, radius, radius, 0, true, FOV_DIAMOND); };
    BENCHMARK(map_name + " FOV_SHADOW") { TCOD_map_compute_fov(map, radius, radius, 0, true, FOV_SHADOW); };
    BENCHMARK(map_name + " FOV_RESTRICTIVE") { TCOD_map_compute_fov(map, radius, radius, 0, true, FOV_RESTRICTIVE); };
    BENCHMARK(map_name + " FOV_PERMISSIVE_8") { TCOD_map_compute_fov(map, radius, radius, 0, true, FOV_PERMISSIVE_8); };
    BENCHMARK(map_name + " FOV_SYMMETRIC_SHADOWCAST") {
      TCOD_map_compute_fov(map, radius, radius, 0, true, FOV_SYMMETRIC_SHADOWCAST);
    };
  }
}
