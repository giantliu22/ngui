/* ***** BEGIN LICENSE BLOCK *****
 * Distributed under the BSD license:
 *
 * Copyright (c) 2015, xuewen.chu
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of xuewen.chu nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL xuewen.chu BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ***** END LICENSE BLOCK ***** */

#include "../app-1.h"
#include "android-ogl-1.h"
#include "android/android.h"
#include "../display-port.h"
#include "../ogl/ogl-es2-1.h"
#include "glsl-shader.h"

#ifndef xx_use_depth_test
#define xx_use_depth_test 0
#endif
#define GL_ETC1_RGB8_OES  0x8D64

XX_NS(ngui)

static EGLDisplay egl_display() {
  EGLDisplay display = EGL_NO_DISPLAY;
  if ( display == EGL_NO_DISPLAY ) { // get display and init it
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    XX_ASSERT(display != EGL_NO_DISPLAY);
    EGLBoolean displayState = eglInitialize(display, nullptr, nullptr);
  }
  return display;
}

static EGLConfig egl_config(EGLDisplay display, const Map<String, int> &options, 
                            bool& multisample_ok) {
  static EGLConfig config = nullptr;

  multisample_ok = false;
  EGLint multisample = 0;

  if (options.has("multisample")) {
    multisample = XX_MAX(options.get("multisample"), 0);
  }

  // choose configuration
  const EGLint attribs[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_RED_SIZE,   8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE,  8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 0,
    EGL_STENCIL_SIZE, 8,
    EGL_SAMPLE_BUFFERS, multisample > 1 ? 1 : 0,
    EGL_SAMPLES, multisample > 1 ? multisample : 0,
    EGL_NONE
  };

  EGLint numConfigs = 0; // number of frame buffer configurations
  EGLBoolean chooseConfigState;

  // first we get size of all configurations
  chooseConfigState = eglChooseConfig(display, attribs, NULL, 0, &numConfigs);
  XX_ASSERT(chooseConfigState);

  if ( numConfigs == 0 ) {
    XX_FATAL("We can't have EGLConfig array with zero size!");
  }

  // then we create array large enough to store all configs
  ArrayBuffer<EGLConfig> supportedConfigs(numConfigs);

  // and load them
  chooseConfigState = eglChooseConfig(display, attribs, 
                                      *supportedConfigs, numConfigs, &numConfigs);
  XX_ASSERT(chooseConfigState);

  if ( numConfigs == 0 ) {
    XX_FATAL("Value of `numConfigs` must be positive");
  }

  EGLint configIndex = 0;
  for ( ; configIndex < numConfigs; configIndex++ ) {

    EGLConfig& cfg = supportedConfigs[configIndex];

    EGLint r, g ,b, a, s, sa;

    bool hasMatch =  eglGetConfigAttrib(display, cfg, EGL_RED_SIZE,   &r) && r == 8
                     && eglGetConfigAttrib(display, cfg, EGL_GREEN_SIZE, &g) && g == 8
                     && eglGetConfigAttrib(display, cfg, EGL_BLUE_SIZE,  &b) && b == 8
                     && eglGetConfigAttrib(display, cfg, EGL_ALPHA_SIZE, &a) && a == 8
                     && eglGetConfigAttrib(display, cfg, EGL_STENCIL_SIZE, &s) && s == 8
                     && eglGetConfigAttrib(display, cfg, EGL_SAMPLES, &sa) && 
                     (multisample <= 1 || sa >= multisample)
    ;
    if ( hasMatch ) {
      config = supportedConfigs[configIndex];
      break;
    }
  }

  // if we don't find anything choose first one
  if ( configIndex == numConfigs ) {
    config = supportedConfigs[0];
  }

  eglGetConfigAttrib(display, config, EGL_SAMPLES, &multisample);

  multisample_ok = multisample > 1;

  return config;
}

/**
 * @class AndroidGLDraw
 */
template<class Basic> class AndroidGLDraw: public Basic {
public:
  inline AndroidGLDraw(GUIApplication* host, EGLDisplay display,
                       EGLConfig config,
                       EGLContext ctx,
                       bool multisample_ok,
                       DrawLibrary library, const Map<String, int>& options)
          : Basic(host, options)
          , core_(this, display, config, ctx), multisample_ok_(multisample_ok) {
    this->m_library = library;
  }
  virtual void refresh_status_for_buffer() { core_.refresh_status_for_buffer(); }
  virtual void begin_render() { core_.begin_render(); }
  virtual void commit_render() { core_.commit_render(); }
  virtual GLint get_ogl_texture_pixel_format(PixelData::Format pixel_format) {
    return core_.get_ogl_texture_pixel_format(pixel_format);
  }
  virtual bool is_support_multisampled() {
    return multisample_ok_ || this->Basic::is_support_multisampled();
  }
  virtual void initializ_ogl_buffers() { core_.initializ_ogl_buffers(); }
  inline AndroidGLDrawCore* core() { return &core_; }

private:
  AndroidGLDrawCore core_;
  bool multisample_ok_;
};

AndroidGLDrawCore* AndroidGLDrawCore::create(GUIApplication* host, 
                                             const Map<String, int> &options) {
  AndroidGLDrawCore* rv = nullptr;
  bool multisample_ok;
  EGLDisplay display = egl_display();
  EGLConfig config = egl_config(display, options, multisample_ok);

  EGLint ctx_attrs[] = {
          EGL_CONTEXT_CLIENT_VERSION, 3,  // opengl es 3
          EGL_NONE
  };

  // TODO: 3.0 现在很多设备都抛出错误,并有一些设备不能绘制边框,暂时只使用2.0
  // TODO: `validate_vertex_attrib_state: No vertex attrib is enabled in a draw call!`
  EGLContext ctx = nullptr;//eglCreateContext(display, config, nullptr, ctx_attrs);
  if ( ctx ) {
    rv = (new AndroidGLDraw<GLDraw>(host, display, config, ctx,
                                    multisample_ok,
                                    DRAW_LIBRARY_GLES3, options))->core();
  } else {
    ctx_attrs[1] = 2; // opengl es 2
    ctx = eglCreateContext(display, config, nullptr, ctx_attrs);
    XX_ASSERT(ctx);

    rv = (new AndroidGLDraw<GLES2Draw>(host, display, config, ctx,
                                       multisample_ok,
                                       DRAW_LIBRARY_GLES2, options))->core();
  }

  return rv;
}

AndroidGLDrawCore::AndroidGLDrawCore(GLDraw* host,
                                     EGLDisplay display, EGLConfig cfg, EGLContext ctx)
: m_display(display)
, m_config(cfg)
, m_context(ctx)
, m_surface(EGL_NO_SURFACE)
, m_window(nullptr)
, m_host(host) {

}

AndroidGLDrawCore::~AndroidGLDrawCore() {
  if ( m_display != EGL_NO_DISPLAY ) {
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if ( m_context != EGL_NO_CONTEXT ) {
      eglDestroyContext(m_display, m_context);
    }
    if ( m_surface != EGL_NO_SURFACE ) {
      eglDestroySurface(m_display, m_surface);
    }
    eglTerminate(m_display);
  }
  m_display = EGL_NO_DISPLAY;
  m_context = EGL_NO_CONTEXT;
  m_surface = EGL_NO_SURFACE;
  m_window = nullptr;
}

GLint AndroidGLDrawCore::get_ogl_texture_pixel_format(PixelData::Format pixel_format) {
  if (m_host->library() == DRAW_LIBRARY_GLES2) {
    switch (pixel_format) {
      case PixelData::RGBA4444:
      case PixelData::RGBX4444:
      case PixelData::RGBA5551:
      case PixelData::RGBA8888:
      case PixelData::RGBX8888: return GL_RGBA;
      case PixelData::RGB565:
      case PixelData::RGB888: return GL_RGB;
      case PixelData::ALPHA8: return GL_ALPHA;
      case PixelData::LUMINANCE8: return GL_LUMINANCE;
      case PixelData::LUMINANCE_ALPHA88: return GL_LUMINANCE_ALPHA;
      // compressd
      case PixelData::ETC1: 
        return m_host->is_support_compressed_ETC1() ? GL_ETC1_RGB8_OES : 0;
        // return is_support_compressed_ETC1() ? GL_COMPRESSED_RGB8_ETC2 : 0;
      default: return 0;
    }
  } else { // 
    switch (pixel_format) {
      case PixelData::RGBA4444:
      case PixelData::RGBX4444:
      case PixelData::RGBA5551:
      case PixelData::RGBA8888:
      case PixelData::RGBX8888: return GL_RGBA;
      case PixelData::RGB565:
      case PixelData::RGB888: return GL_RGB;
      case PixelData::ALPHA8: return GL_ALPHA;
      case PixelData::LUMINANCE8: return GL_LUMINANCE;
      case PixelData::LUMINANCE_ALPHA88: return GL_LUMINANCE_ALPHA;
      // compressd
      case PixelData::ETC1:
      case PixelData::ETC2_RGB: return GL_COMPRESSED_RGB8_ETC2;
      case PixelData::ETC2_RGB_A1:
      case PixelData::ETC2_RGBA: return GL_COMPRESSED_RGBA8_ETC2_EAC;
      default: return 0;
    }
    // #define GL_COMPRESSED_RGB8_ETC2           0x9274
    // #define GL_COMPRESSED_SRGB8_ETC2          0x9275
    // #define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
    // #define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
    // #define GL_COMPRESSED_RGBA8_ETC2_EAC      0x9278
    // #define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279
  }
}

void AndroidGLDrawCore::initialize() {
  m_host->set_best_display_scale(Android::get_display_scale());
  m_host->initialize();
  refresh_surface_size(nullptr);
}

bool AndroidGLDrawCore::create_surface(ANativeWindow* window) {
  XX_ASSERT(!m_window);
  XX_ASSERT(!m_surface);
  EGLSurface surface = eglCreateWindowSurface(m_display, m_config, window, nullptr);

  if ( !surface ) {
    XX_ERR("Unable to create a drawing surface");
    return false;
  }
  if ( !eglMakeCurrent(m_display, surface, surface, m_context) ) {
    eglDestroySurface(m_display, surface);
    return false;
  }

  m_window = window;
  m_surface = surface;
  m_surface_size = Vec2(ANativeWindow_getWidth(window), ANativeWindow_getHeight(window));

  return true;
}

void AndroidGLDrawCore::destroyed_surface(ANativeWindow* window) {
  if ( m_window ) {
    XX_ASSERT(window == m_window);
    if (m_surface) {
      eglDestroySurface(m_display, m_surface);
    }
    m_window = nullptr;
    m_surface = nullptr;
  }
}

void AndroidGLDrawCore::refresh_surface_size(CGRect* rect) {

  if ( m_window ) {
    m_surface_size = Vec2(ANativeWindow_getWidth(m_window), ANativeWindow_getHeight(m_window));
  }

  if ( m_surface_size[0] == 0 || m_surface_size[1] == 0 ) return;

  if ( rect == nullptr ) {
    CGRect region = m_host->selected_region(); // 使用上次的区域，如果这是有效的

    if (region.size.width() == 0) { // 区域无效
      m_host->set_surface_size(m_surface_size);
    } else {
      m_host->set_surface_size(m_surface_size, &region);
    }
  } else {
    m_host->set_surface_size(m_surface_size, rect);
  }
  
  // set virtual keys rect

  m_virtual_keys_rect = CGRect();

  Vec2 scale = m_host->host()->display_port()->scale_value();
  CGRect region = m_host->selected_region();

  int width = int(m_host->surface_size().width() - region.size.width());
  int height = int(m_host->surface_size().height() - region.size.height());

  if ( width > 0 ) { // left / right
    if ( region.origin.x() == 0 ) { // right
      m_virtual_keys_rect = {
        Vec2(region.size.width() / scale[0], 0),
        Vec2(width / scale[0], region.size.height() / scale[1])
      };
    } else { // left
      m_virtual_keys_rect = {
        Vec2(-region.origin.x() / scale[0], 0),
        Vec2(region.origin.x() / scale[0], region.size.height() / scale[1])
      };
    }
  } else if ( height > 0 ) { // bottom
    m_virtual_keys_rect = {
      Vec2(0, region.size.height() / scale[0]),
      Vec2(region.size.width() / scale[0], height / scale[1])
    };
  }

}

void AndroidGLDrawCore::refresh_status_for_buffer() {

  if ( m_host->surface_size() == Vec2() )
    return;

  Vec2 size = m_host->surface_size();
  int width = size.width();
  int height = size.height();

  glViewport(0, 0, width, height);

  // Test the framebuffer for completeness.
  if ( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ) {
    XX_ERR("failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER) );
  }

  // Retrieve the height and width of the color renderbuffer.
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
  XX_DEBUG("GL_RENDERBUFFER_WIDTH: %d, GL_RENDERBUFFER_HEIGHT: %d", width, height);
}

void AndroidGLDrawCore::begin_render() {
  m_host->m_stencil_ref_value = 0;
  m_host->m_root_stencil_ref_value = 0;
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_STENCIL_TEST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void AndroidGLDrawCore::commit_render() {
  if ( m_host->is_support_vao() ) {
    glBindVertexArray(0);
  }

#define gl_  glshaders(m_host)

  if ( m_virtual_keys_rect.size.width() != 0 ) {
    // Draw Virtual Keys background color
    gl_->box_color.use();

    float data[] = {
      1,0,0, // matrix
      0,1,0,
      1, // opacity
      m_virtual_keys_rect.origin[0], // vertex_ac
      m_virtual_keys_rect.origin[1],
      m_virtual_keys_rect.origin[0] + m_virtual_keys_rect.size[0],
      m_virtual_keys_rect.origin[1] + m_virtual_keys_rect.size[1],
      0,0,0,1 // background_color
    };

    glUniform1fv(gl_->box_color_uniform_draw_data, sizeof(data), data);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  }

  eglSwapBuffers(m_display, m_surface);
}

void AndroidGLDrawCore::initializ_ogl_buffers() {
  if ( ! m_host->m_frame_buffer ) {
    // Create the framebuffer and bind it so that future OpenGL ES framebuffer commands are directed to it.
    glGenFramebuffers(1, &m_host->m_frame_buffer);
    if ( m_host->is_support_query() ) { // 屏幕遮挡查询对像
      glGenQueries(1, &m_host->m_SCREEN_RANGE_OCCLUSION_QUERY_HANDLE);
    }
  }
}

/**
 * @func default_atom_pixel
 */
float DisplayPort::default_atom_pixel() {
  float v = Android::get_display_scale();
  return 1.0f / v;
}

XX_END