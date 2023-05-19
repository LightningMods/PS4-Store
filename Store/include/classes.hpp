#pragma once
#include <vector>
#include <memory>
#include <string>
#include "log.h"
#include "GLES2_common.h"
#include <freetype-gl.h>  // links against libfreetype-gl




// then we can print them splitted
typedef struct {
    int offset;
    int count;
} textline_t;

// ------------------------------------------------------- typedef & struct ---
typedef struct {
    float x, y, z;    // position (3f)
    float s, t;       // texture  (2f)
    float r, g, b, a; // color    (4f)
} vertex_t;


void ftgl_render_vbo(vertex_buffer_t* vbo, vec3* offset);

class VertexBuffer {
  public: std::unique_ptr < vertex_buffer_t,
  decltype( & vertex_buffer_delete) > buffer {
    nullptr,
    vertex_buffer_delete
  };

  VertexBuffer() =  default;

  VertexBuffer(const char * format) {
    if (!buffer)
      buffer.reset(vertex_buffer_new(format));
  }

  void render(GLenum mode) {
    if (buffer)
      vertex_buffer_render(buffer.get(), mode);
    else
      log_error("vertex_buffer_render failed: Buffer is NULL");
  }

  void render_setup(GLenum mode) {
    if (buffer)
      vertex_buffer_render_setup(buffer.get(), mode);
  }

  void render_item(int numb) {
    if (buffer)
      vertex_buffer_render_item(buffer.get(), numb);
  }

  void render_finish() {
    if (buffer)
      vertex_buffer_render_finish(buffer.get());
  }

  // You might need to replace texture_font_t and texture_glyph_t
  // with your actual definitions
  void add_text(struct texture_font_t * font, std::string text, vec4 & color, vec2 & pen) {
    if (!font || !buffer) {
      log_error("add_text failed: Font is %s, Buffer is %s", font ? "OK" : "NULL", buffer ? "OK" : "NULL");
      return;
    }

   // log_info("message: %s", text.c_str());
    size_t i;
    float r = color.r, g = color.g, b = color.b, a = color.a;

    for (i = 0; i < text.length(); ++i) {
      texture_glyph_t * glyph = texture_font_get_glyph(font, (const char * ) & text.at(i));
      if (glyph) {
        float kerning = 0.0f;
        if (i > 0) kerning = texture_glyph_get_kerning(glyph, (const char * ) & text.at(i - 1));
        pen.x += kerning;
        int x0 = (int)(pen.x + glyph -> offset_x);
        int y0 = (int)(pen.y + glyph -> offset_y);
        int x1 = (int)(x0 + glyph -> width);
        int y1 = (int)(y0 - glyph -> height);
        float s0 = glyph -> s0;
        float t0 = glyph -> t0;
        float s1 = glyph -> s1;
        float t1 = glyph -> t1;
        GLuint indices[6] = { 0, 1, 2, 0, 2,3 }; // (two triangles)
        /* VBO is setup as: "vertex:3f, tex_coord:2f, color:4f" */
        vertex_t vertices[4];

        vertices[0].x = vertices[1].x = x0;
        vertices[2].x = vertices[3].x = x1;

        vertices[0].y = vertices[3].y = y0;
        vertices[1].y = vertices[2].y = y1;

        vertices[0].z = vertices[1].z = vertices[2].z = vertices[3].z = 0;

        vertices[1].s = vertices[0].s = s0;
        vertices[2].s = vertices[3].s = s1;

        vertices[1].t = vertices[2].t = t1;
        vertices[0].t = vertices[3].t = t0;

        vertices[0].r = vertices[1].r = vertices[2].r = vertices[3].r = r;
        vertices[0].g = vertices[1].g = vertices[2].g = vertices[3].g = g;
        vertices[0].b = vertices[1].b = vertices[2].b = vertices[3].b = b;
        vertices[0].a = vertices[1].a = vertices[2].a = vertices[3].a = a;

        vertex_buffer_push_back(buffer.get(), vertices, 4, indices, 6);
        pen.x += glyph -> advance_x;
      }
    }
  }

  void render_vbo(vec3 * offset) {
    if (buffer)
      ftgl_render_vbo(buffer.get(), offset);
  }

  void clear() {
    if (buffer)
      buffer.reset();
  }

  void push_back(const void * vertices, size_t vcount,
    const GLuint * indices, size_t icount) {
    if (buffer) {
      vertex_buffer_push_back(buffer.get(), vertices, vcount, indices, icount);
    }
  }

  bool empty() const {
    return buffer == nullptr;
  }

  explicit operator bool() const {
    return static_cast < bool > (buffer);
  }
};


class Base64 {
 public:

  static std::string Encode(const std::string data) {
    static constexpr char sEncodingTable[] = {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
      'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
      'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
      'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
      'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
      'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
      'w', 'x', 'y', 'z', '0', '1', '2', '3',
      '4', '5', '6', '7', '8', '9', '+', '/'
    };

    size_t in_len = data.size();
    size_t out_len = 4 * ((in_len + 2) / 3);
    std::string ret(out_len, '\0');
    size_t i;
    char *p = const_cast<char*>(ret.c_str());

    for (i = 0; i < in_len - 2; i += 3) {
      *p++ = sEncodingTable[(data[i] >> 2) & 0x3F];
      *p++ = sEncodingTable[((data[i] & 0x3) << 4) | ((int) (data[i + 1] & 0xF0) >> 4)];
      *p++ = sEncodingTable[((data[i + 1] & 0xF) << 2) | ((int) (data[i + 2] & 0xC0) >> 6)];
      *p++ = sEncodingTable[data[i + 2] & 0x3F];
    }
    if (i < in_len) {
      *p++ = sEncodingTable[(data[i] >> 2) & 0x3F];
      if (i == (in_len - 1)) {
        *p++ = sEncodingTable[((data[i] & 0x3) << 4)];
        *p++ = '=';
      }
      else {
        *p++ = sEncodingTable[((data[i] & 0x3) << 4) | ((int) (data[i + 1] & 0xF0) >> 4)];
        *p++ = sEncodingTable[((data[i + 1] & 0xF) << 2)];
      }
      *p++ = '=';
    }

    return ret;
  }

  static std::string Decode(const std::string& input, std::string& out) {
    static constexpr unsigned char kDecodingTable[] = {
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
      52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
      64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
      15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
      64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
      41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
    };

    size_t in_len = input.size();
    if (in_len % 4 != 0) return "Input data size is not a multiple of 4";

    size_t out_len = in_len / 4 * 3;
    if (input[in_len - 1] == '=') out_len--;
    if (input[in_len - 2] == '=') out_len--;

    out.resize(out_len);

    for (size_t i = 0, j = 0; i < in_len;) {
      uint32_t a = input[i] == '=' ? 0 & i++ : kDecodingTable[static_cast<int>(input[i++])];
      uint32_t b = input[i] == '=' ? 0 & i++ : kDecodingTable[static_cast<int>(input[i++])];
      uint32_t c = input[i] == '=' ? 0 & i++ : kDecodingTable[static_cast<int>(input[i++])];
      uint32_t d = input[i] == '=' ? 0 & i++ : kDecodingTable[static_cast<int>(input[i++])];

      uint32_t triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + (d << 0 * 6);

      if (j < out_len) out[j++] = (triple >> 2 * 8) & 0xFF;
      if (j < out_len) out[j++] = (triple >> 1 * 8) & 0xFF;
      if (j < out_len) out[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return "";
  }

};
