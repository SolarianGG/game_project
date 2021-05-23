#pragma once
#include <cstdint>
#include <string_view>
#include <vector>
namespace uchiha {

struct vertex
{
  float x = 0.f;
  float y = 0.f;
  float z = 0.f;
  float r = 0.f;
  float g = 0.f;
  float b = 0.f;
  float a = 0.f;
  float tx = 0.f;
  float ty = 0.f;
  vertex()
    : x(0)
    , y(0)
    , z(0)
    , r(0)
    , g(0)
    , b(0)
    , a(0)
    , tx(0)
    , ty(0)
  {}
  vertex(float x0,
         float y0,
         float z0,
         float r0,
         float g0,
         float b0,
         float a0,
         float tx0,
         float ty0)
    : x(x0)
    , y(y0)
    , z(z0)
    , r(r0)
    , g(g0)
    , b(b0)
    , a(a0)
    , tx(tx0)
    , ty(ty0)
  {}
};

struct triangle
{
  vertex v[3];
  triangle(vertex v1, vertex v2, vertex v3)
  {
    v[0] = v1;
    v[1] = v2;
    v[2] = v3;
  }
};

class texture
{
public:
  virtual ~texture();
  virtual uint16_t get_width() const = 0;
  virtual uint16_t get_height() const = 0;
  virtual uint32_t get_handle() const = 0;
};

struct event
{
  enum
  {
    pressed,
    reliased,
    quit,
    mouse_motion,
    mouse_click
  } type;
  enum
  {
    left,
    right,
    top,
    bottom,
    enter,
    escape,
    space,
    undefined_key
  } key;
  int mouse_x = 0;
  int mouse_y = 0;
};

class engine
{
public:
  virtual ~engine();
  virtual bool init(uint16_t ww, uint16_t wh, bool fullscren = false) = 0;
  virtual bool read_input(event& e) = 0;
  virtual texture* create_texture(std::string_view path) = 0;
  virtual void render(const std::vector<triangle>& vertex_buffer) = 0;
  virtual void render(const std::vector<triangle>& vertex_buffer,
                      const texture& t) = 0;
  virtual void swap_buffers() = 0;
  virtual void destroy() = 0;
};

engine*
create_engine();

void
destroy_engine(engine* e);

}
