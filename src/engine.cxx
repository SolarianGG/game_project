#include "engine.hxx"
#include "glad/glad.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <tuple>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

uchiha::engine::~engine() {}

namespace uchiha {
class shader
{
  GLuint vertex_shader = 0;
  GLuint fragment_shader = 0;
  GLuint program = 0;

  bool init_shader(GLuint shader, const char* shader_src)
  {
    glShaderSource(shader, 1, &shader_src, nullptr);
    glCompileShader(shader);

    GLint compile_status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status == 0) {
      GLint info_len = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
      std::vector<char> info(static_cast<size_t>(info_len));
      glGetShaderInfoLog(shader, info_len, nullptr, info.data());
      glDeleteShader(shader);
      return false;
    }
    return true;
  }

public:
  shader(const char* vertex_shader_src,
         const char* fragment_shader_src,
         const std::vector<std::tuple<GLuint, const GLchar*>>& attributes)
  {
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    if (!init_shader(vertex_shader, vertex_shader_src)) {
      std::cerr << "error: Failed to create vertex shader ( engine.cxx:  )"
                << std::endl;
    }

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!init_shader(fragment_shader, fragment_shader_src)) {
      std::cerr << "error: Failed to create fragment shader ( engine.cxx:  )"
                << std::endl;
    }
    program = glCreateProgram();
    if (program == 0) {
      std::cerr << "error: Failed to create program ( engine.cxx:  )"
                << std::endl;
    }

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    for (const auto& attr : attributes) {
      GLuint loc = std::get<0>(attr);
      const GLchar* attr_name = std::get<1>(attr);
      glBindAttribLocation(program, loc, attr_name);
    }
    glLinkProgram(program);

    GLint linked_status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked_status);

    if (linked_status == 0) {
      GLint infoLen = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
      std::vector<char> infoLog(static_cast<size_t>(infoLen));
      glGetProgramInfoLog(program, infoLen, nullptr, infoLog.data());
      glDeleteProgram(program);
      std::cerr << "error: Failed to link program ( engine.cxx:  )" << infoLen
                << std::endl;
    }
  }

  void use() { glUseProgram(program); }
  void set_uniform(std::string_view attr, const texture& t)
  {
    int location = glGetUniformLocation(program, attr.data());
    glActiveTexture(GL_TEXTURE0 + t.get_handle());
    glBindTexture(GL_TEXTURE_2D, t.get_handle());
    glUniform1i(location, 0 + t.get_handle());
  }
};
}

uchiha::texture::~texture() {}

class texture_impl : public uchiha::texture
{
  uint16_t texture_width = 0;
  uint16_t texture_height = 0;
  GLuint textrure_handle = 0;

public:
  texture_impl(uint16_t width, uint16_t height, GLuint handle)
    : texture_width(width)
    , texture_height(height)
    , textrure_handle(handle)
  {}
  uint16_t get_width() const override { return texture_width; }
  uint16_t get_height() const override { return texture_height; }
  uint32_t get_handle() const override { return textrure_handle; }
};

class engine_impl : public uchiha::engine
{
  SDL_Window* window = nullptr;
  uint16_t window_width = 0;
  uint16_t window_height = 0;
  SDL_GLContext context = nullptr;
  uint32_t gl_default_vbo = 0;
  std::vector<uchiha::shader*> shaders;
  GLuint vertex_attribute_object = 0;

public:
  bool init(uint16_t ww, uint16_t wh, bool fullscreen = false) override;
  bool read_input(uchiha::event& e) override;
  uchiha::texture* create_texture(std::string_view path) override;
  void render(const std::vector<uchiha::triangle>& vertex_buffer) override;
  void render(const std::vector<uchiha::triangle>& vertex_buffer,
              const uchiha::texture& t) override;
  void swap_buffers() override;
  void destroy() override;
};

bool
engine_impl::init(uint16_t ww, uint16_t wh, bool fullscreen)
{
  SDL_version compiled;
  SDL_version linked;
  SDL_VERSION(&compiled);
  SDL_GetVersion(&linked);
  std::cout << static_cast<int>(compiled.major) << "."
            << static_cast<int>(compiled.minor) << "."
            << static_cast<int>(compiled.patch) << std::endl;

  std::cout << static_cast<int>(linked.major) << "."
            << static_cast<int>(linked.minor) << "."
            << static_cast<int>(linked.patch) << std::endl;

  if (SDL_COMPILEDVERSION !=
      SDL_VERSIONNUM(linked.major, linked.minor, linked.patch)) {
    std::cerr << "error: There are different versions of compiled and linked "
                 "SDL ( engine.cxx:  )"
              << std::endl;
    return false;
  }

  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    std::cerr << "error: Failed to init SDL ( engine.cxx:  )" << SDL_GetError()
              << std::endl;
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
  uint32_t window_flags = fullscreen
                            ? SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP
                            : SDL_WINDOW_OPENGL;
  window = SDL_CreateWindow("Uchiha Engine",
                            SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED,
                            ww,
                            wh,
                            window_flags);
  window_width = ww;
  window_height = wh;
  if (window == nullptr) {
    std::cerr << "error: Failed to create window ( engine.cxx:  )"
              << SDL_GetError() << std::endl;
    SDL_Quit();
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  context = SDL_GL_CreateContext(window);
  if (context == nullptr) {
    std::cerr << "error: Failed to create context ( engine.cxx:  )"
              << SDL_GetError() << std::endl;

    SDL_DestroyWindow(window);
    SDL_Quit();
    return false;
  }

  if (gladLoadGLLoader(SDL_GL_GetProcAddress) == 0) {
    std::cerr << "error: Failed to load GL functions ( engine.cxx:  )"
              << std::endl;
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return false;
  }

  glGenBuffers(1, &gl_default_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, gl_default_vbo);
  glGenVertexArrays(1, &vertex_attribute_object);
  glBindVertexArray(vertex_attribute_object);
  uint32_t data_size_in_bytes = 0;
  glBufferData(GL_ARRAY_BUFFER, data_size_in_bytes, nullptr, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, data_size_in_bytes, nullptr);

  const char* vertex_shader_01_src = R"(
                                     #version 330 core
                                     layout (location = 0) in vec3 a_position;
                                     layout (location = 1) in vec4 a_color;


                                     out vec4 v_color;
                                     void main()
                                     {
                                        v_color = a_color;
                                        gl_Position = vec4(a_position, 1.0);
                                     }
                                     )";

  const char* fragment_shader_01_src = R"(
                                       #version 330 core

                                       in vec4 v_color;

                                       out vec4 frag_color;

                                       void main()
                                       {
                                           frag_color = v_color;
                                       }
                                       )";
  shaders.push_back(
    new uchiha::shader(vertex_shader_01_src,
                       fragment_shader_01_src,
                       { { 0, "a_position" }, { 1, "a_color" } }));
  shaders.at(0)->use();

  const char* vertex_shader_02_src = R"(
                                     #version 330 core
                                     layout (location = 0) in vec3 a_position;
                                     layout (location = 1) in vec4 a_color;
                                     layout (location = 2) in vec2 a_tex_coord;


                                     out vec4 v_color;
                                     out vec2 v_tex_coord;
                                     void main()
                                     {
                                        v_color = a_color;
                                        v_tex_coord = a_tex_coord;
                                        gl_Position = vec4(a_position, 1.0);
                                     }
                                     )";

  const char* fragment_shader_02_src = R"(
                                       #version 330 core

                                       in vec4 v_color;
                                       in vec2 v_tex_coord;

                                       uniform sampler2D s_texture;
                                       out vec4 frag_color;

                                       void main()
                                       {
                                           frag_color = texture(s_texture, v_tex_coord) * v_color;
                                       }
                                       )";
  shaders.push_back(new uchiha::shader(
    vertex_shader_02_src,
    fragment_shader_02_src,
    { { 0, "a_position" }, { 1, "a_color" }, { 2, "a_tex_coord" } }));
  shaders.at(1)->use();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClearColor(0.f, 0.0, 0.f, 0.0f);
  glViewport(0, 0, window_width, window_height);
  return true;
}

bool
engine_impl::read_input(uchiha::event& event)
{
  SDL_Event e;
  if (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      event.type = event.quit;
      return true;
    } else if (e.type == SDL_MOUSEMOTION) {
      event.type = event.mouse_motion;
      event.mouse_x = e.motion.x;
      event.mouse_y = e.motion.y;
      return true;

    } else if (e.type == SDL_MOUSEBUTTONUP) {
      event.type = event.mouse_click;
      event.mouse_x = e.motion.x;
      event.mouse_y = e.motion.y;
      return true;
    } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
      event.type = (e.type == SDL_KEYDOWN) ? event.pressed : event.reliased;
      if (e.key.keysym.sym == SDLK_w || e.key.keysym.sym == SDLK_UP) {
        event.key = event.top;
      } else if (e.key.keysym.sym == SDLK_s || e.key.keysym.sym == SDLK_DOWN) {
        event.key = event.bottom;
      } else if (e.key.keysym.sym == SDLK_a || e.key.keysym.sym == SDLK_LEFT) {
        event.key = event.left;
      } else if (e.key.keysym.sym == SDLK_d || e.key.keysym.sym == SDLK_RIGHT) {
        event.key = event.right;
      } else if (e.key.keysym.sym == SDLK_ESCAPE) {
        event.key = event.escape;
      } else if (e.key.keysym.sym == SDLK_SPACE) {
        event.key = event.space;
      } else if (e.key.keysym.sym == SDLK_KP_ENTER ||
                 e.key.keysym.sym == SDLK_RETURN) {
        event.key = event.enter;
      } else {
        event.key = event.undefined_key;
      }
      return true;
    }
  }
  return false;
}

uchiha::texture*
engine_impl::create_texture(std::string_view path)
{
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int width, height, nrChannels;
  unsigned char* data = stbi_load(path.data(), &width, &height, &nrChannels, 0);
  if (data) {
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 width,
                 height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cerr << "error: Failed to load texture ( engine.cxx: )" << std::endl;
  }
  stbi_image_free(data);
  texture_impl* texture_object = new texture_impl(width, height, texture);
  return texture_object;
}

void
engine_impl::render(const std::vector<uchiha::triangle>& vertex_buffer)
{
  shaders.at(0)->use();
  const uchiha::vertex* t = &vertex_buffer.data()->v[0];
  uint32_t data_size_in_bytes =
    static_cast<uint32_t>((vertex_buffer.size() * 3) * sizeof(uchiha::vertex));
  glBufferData(GL_ARRAY_BUFFER, data_size_in_bytes, t, GL_DYNAMIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, data_size_in_bytes, t);

  glEnableVertexAttribArray(0);
  GLintptr position_attr_offset = 0;
  glVertexAttribPointer(0,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(uchiha::vertex),
                        reinterpret_cast<void*>(position_attr_offset));

  glEnableVertexAttribArray(1);
  GLintptr color_attr_offset = sizeof(float) * 3;
  glVertexAttribPointer(1,
                        4,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(uchiha::vertex),
                        reinterpret_cast<void*>(color_attr_offset));
  GLsizei num_of_vertexes = static_cast<GLsizei>(vertex_buffer.size() * 3);
  glDrawArrays(GL_TRIANGLES, 0, num_of_vertexes);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
}

void
engine_impl::render(const std::vector<uchiha::triangle>& vertex_buffer,
                    const uchiha::texture& tx)
{
  shaders.at(1)->use();
  shaders.at(1)->set_uniform("s_texture", tx);
  const uchiha::vertex* t = &vertex_buffer.data()->v[0];
  uint32_t data_size_in_bytes =
    static_cast<uint32_t>((vertex_buffer.size() * 3) * sizeof(uchiha::vertex));
  glBufferData(GL_ARRAY_BUFFER, data_size_in_bytes, t, GL_DYNAMIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, data_size_in_bytes, t);

  glEnableVertexAttribArray(0);
  GLintptr position_attr_offset = 0;
  glVertexAttribPointer(0,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(uchiha::vertex),
                        reinterpret_cast<void*>(position_attr_offset));

  glEnableVertexAttribArray(1);
  GLintptr color_attr_offset = sizeof(float) * 3;
  glVertexAttribPointer(1,
                        4,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(uchiha::vertex),
                        reinterpret_cast<void*>(color_attr_offset));
  glEnableVertexAttribArray(2);
  GLintptr texture_attr_offset = sizeof(float) * 7;
  glVertexAttribPointer(2,
                        2,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(uchiha::vertex),
                        reinterpret_cast<void*>(texture_attr_offset));

  GLsizei num_of_vertexes = static_cast<GLsizei>(vertex_buffer.size() * 3);
  glDrawArrays(GL_TRIANGLES, 0, num_of_vertexes);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
}

void
engine_impl::swap_buffers()
{
  SDL_GL_SwapWindow(window);

  glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

void
engine_impl::destroy()
{
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

uchiha::engine*
create_engine()
{
  engine_impl* engine = new engine_impl();
  return engine;
}

void
destroy_engine(uchiha::engine* e)
{
  if (e)
    delete e;
}

int
main(int /*argc*/, char** /*argv*/)
{
  engine_impl* engine = new engine_impl();
  engine->init(800, 600, false);

  uchiha::texture* tx = engine->create_texture("res/black-horse.png");

  std::vector<uchiha::triangle> vertex_buffer;

  uchiha::triangle tr0(
    { 0.3f, -0.3f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.3f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
    { -0.3f, -0.3f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f });
  uchiha::triangle tr1(
    { 0.6f, 0.3f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.9f, 0.3f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.75f, 0.6f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f });
  vertex_buffer.push_back(tr0);
  vertex_buffer.push_back(tr1);

  std::vector<uchiha::triangle> vertex_buffer_01;
  uchiha::triangle tr2(
    { -0.1f, 0.1f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
    { -0.1f, 0.4f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f },
    { -0.4f, 0.1f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f });
  uchiha::triangle tr3(
    { -0.4f, 0.4f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f },
    { -0.1f, 0.4f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f },
    { -0.4f, 0.1f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f });
  vertex_buffer_01.push_back(tr2);
  vertex_buffer_01.push_back(tr3);

  uchiha::event e;
  bool is_loop = true;
  while (is_loop) {
    while (engine->read_input(e)) {
      switch (e.type) {
        case uchiha::event::quit: {
          std::cout << "event: quit" << std::endl;
          is_loop = false;
          break;
        }
        case uchiha::event::mouse_motion: {
          std::cout << "event: mouse motion" << std::endl;
          std::cout << e.mouse_x << " " << e.mouse_y << std::endl;
          break;
        }
        case uchiha::event::mouse_click: {
          std::cout << "event: mouse click" << std::endl;
          break;
        }
        case uchiha::event::pressed: {
          std::cout << "event: key pressed" << std::endl;
          std::cout << e.key << std::endl;
          break;
        }
        case uchiha::event::reliased: {
          std::cout << "event: key reliased" << std::endl;
          std::cout << e.key << std::endl;
          break;
        }
      }
    }
    engine->render(vertex_buffer);
    engine->render(vertex_buffer_01, *tx);
    engine->swap_buffers();
  }
  engine->destroy();
  return 0;
}
