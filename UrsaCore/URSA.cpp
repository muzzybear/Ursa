#ifdef _WIN32
#ifdef _DEBUG
#pragma comment(lib, "SDL2maind.lib")
#else
#pragma comment(lib, "SDL2main.lib")
#endif
#endif

#include "URSA.h"

#include <SDL2/SDL.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_rect_pack.h>
#include <stb_truetype.h>

#include <memory>
#include <map>
#include <vector>
#include <fstream>

namespace ursa {
	namespace internal {
		static bool initialized = false;

		static SDL_Window* g_window = nullptr;
		static SDL_GLContext g_glContext;

		static unsigned int g_VAO = 0;
		static unsigned int g_VBO = 0;
		static unsigned int g_shader = 0;

		// TODO glBindAttribLocation instead of layout qualifiers, for legacy support?
		const char *vsh_src =
R"(#version 330 core
layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;

uniform mat4 transform;

out vec2 uv;
out vec4 color;

void main()
{
	gl_Position = transform * vec4(in_pos, 1.0);
	uv = in_uv;
	color = in_color;
})";

		const char *fsh_src =
R"(#version 330 core
in vec2 uv;
in vec4 color;

out vec4 FragColor;

uniform sampler2D tex;
uniform bool use_tex;
uniform bool alpha_tex;

void main()
{
	if (use_tex) {
		vec4 texcolor = texture(tex, uv);
		if (alpha_tex) {
			FragColor = vec4(1.0f, 1.0f, 1.0f, texcolor.r) * color;
		} else {
			FragColor = texcolor * color;
		}
	} else {
	    FragColor = color;
	}
})";
		GLuint make_shader(GLenum shaderType, const char *source) {
			GLuint handle = glCreateShader(shaderType);

			glShaderSource(handle, 1, &source, nullptr);
			glCompileShader(handle);

			int  success;
			char infoLog[512];
			glGetShaderiv(handle, GL_COMPILE_STATUS, &success);

			if (!success)
			{
				glGetShaderInfoLog(handle, 512, NULL, infoLog);
				// TODO handle error
				//std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
			}

			return handle;
		}

		void create_internal_objects() {
			if (initialized) return;
			initialized = true;

			glGenVertexArrays(1, &g_VAO);
			glBindVertexArray(g_VAO);

			glGenBuffers(1, &g_VBO);

			glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
			glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);

			unsigned int vertexShader = make_shader(GL_VERTEX_SHADER, vsh_src);
			unsigned int fragmentShader = make_shader(GL_FRAGMENT_SHADER, fsh_src);

			g_shader = glCreateProgram();
			glAttachShader(g_shader, vertexShader);
			glAttachShader(g_shader, fragmentShader);
			glLinkProgram(g_shader);
			// individual shader objects are no longer needed after linking a shader program
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			int  success;
			char infoLog[512];

			glGetProgramiv(g_shader, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(g_shader, 512, NULL, infoLog);
				//std::cout << "asdf: " << infoLog << std::endl;
				abort();
			}

			glUseProgram(g_shader);
			// default to 2d mode
			transform_2d();

		}

		void create_window(int width, int height) {
			if (SDL_Init(SDL_INIT_VIDEO) < 0)
			{
				//std::cout << "SDL cannot init with error " << SDL_GetError() << std::endl;
				return;
			}

			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

			g_window = SDL_CreateWindow("foo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
			if (g_window == nullptr)
			{
				//std::cout << "Cannot create window with error " << SDL_GetError() << std::endl;
				return;
			}

			g_glContext = SDL_GL_CreateContext(g_window);
			if (g_glContext == nullptr)
			{
				//std::cout << "Cannot create OpenGL context with error " << SDL_GetError() << std::endl;
				return;
			}

			gladLoadGLLoader(SDL_GL_GetProcAddress);

			// enable vsync
			SDL_GL_SetSwapInterval(1);

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClearDepth(1.0f);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);

			glViewport(0, 0, width, height);
		}

		void requires_window() {
			// default to windowed mode in 800x600
			if (!internal::g_window)
				internal::create_window(800, 600);
		}

		void swap_window() {
			SDL_GL_SwapWindow(g_window);
		}

		void quit() {
			SDL_DestroyWindow(g_window);
			g_window = NULL;

			SDL_Quit();
		}
	}

	// ...

	std::unique_ptr<char[]> file_contents(const char *filename) {
		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		int filesize = (int)file.tellg();
		file.seekg(0, std::ios::beg);
		auto buf = std::make_unique<char[]>(filesize);
		file.read(buf.get(), filesize);
		if (!file) {
			// TODO handle loading error
			assert(0);
		}
		return buf;
	}

	// ...

	struct chardatas {
		FontAtlas::FontInfo fontinfo;
		stbtt_packedchar data[128];
	};

	class FontAtlasImpl {
	public:
		void add_truetype(const char *filename, float font_size) {
			m_sourcelist.emplace_back(filename, std::vector<float>{font_size});
		}
		void add_truetype(const char *filename, std::initializer_list<float> font_sizes) {
			m_sourcelist.emplace_back(filename, std::vector<float>{font_sizes});
		}

		void bake(int texwidth, int texheight) {
			std::unique_ptr<unsigned char[]> fontbitmap = std::make_unique<unsigned char[]>(texwidth*texheight);
			stbtt_pack_context spc;
			stbtt_PackBegin(&spc, fontbitmap.get(), texwidth, texheight, 0, 1, nullptr);

			for (const auto &t : m_sourcelist) {
				const auto &filename = std::get<0>(t);
				const auto &sizes = std::get<1>(t);

				// TODO avoid conversion, use std::string everywhere?
				auto fontbuf = file_contents(filename.c_str());
				stbtt_fontinfo fontinfo;
				stbtt_InitFont(&fontinfo, (unsigned char*)fontbuf.get(), 0);
				int ascent{ 0 }, descent{ 0 }, gap{ 0 }; // in FUnits
				stbtt_GetFontVMetrics(&fontinfo, &ascent, &descent, &gap);

				for (float size : sizes) {
					float scale = stbtt_ScaleForPixelHeight(&fontinfo, size);
					m_chardatas.push_back({ { ascent*scale, descent*scale, gap*scale }, {0} });
					stbtt_PackSetOversampling(&spc, 1, 1);
					stbtt_PackFontRange(&spc, (unsigned char *)fontbuf.get(), 0, size, 32, 96, m_chardatas.back().data + 32);
				}
			}

			stbtt_PackEnd(&spc);

			m_tex = ursa::texture8bpp(texwidth, texheight, fontbitmap.get());
		}

		ursa::TextureHandle tex() const { return m_tex; }

		FontAtlas::GlyphInfo glyphInfo(int fontIndex, int codepoint) const {
			const auto &c = m_chardatas[fontIndex].data[codepoint];
			return {
				{{c.x0, c.y0}, {c.x1 - c.x0, c.y1 - c.y0}},
				{{c.xoff, c.yoff}, {c.xoff2 - c.xoff, c.yoff2 - c.yoff}},
				c.xadvance
			};
		}

		FontAtlas::FontInfo fontInfo(int fontIndex) const {
			assert(fontIndex >= 0 && (unsigned)fontIndex < m_chardatas.size());
			return m_chardatas[fontIndex].fontinfo;
		}
	private:
		ursa::TextureHandle m_tex;
		std::vector<chardatas> m_chardatas;
		std::vector<std::tuple<std::string, std::vector<float>>> m_sourcelist;

	};

	FontAtlas::FontAtlas() : impl(new FontAtlasImpl) {}
	FontAtlas::FontAtlas(const FontAtlas & other) : impl{ new FontAtlasImpl{*(other.impl)} } {}
	FontAtlas::FontAtlas(FontAtlas && other) : impl{ nullptr } { impl.swap(other.impl); }
	FontAtlas & FontAtlas::operator=(const FontAtlas & other) {
		if (&other != this) {
			impl.reset(new FontAtlasImpl{ *(other.impl) });
		}
		return *this;
	}
	FontAtlas & FontAtlas::operator=(FontAtlas && other) {
		if (&other != this) {
			impl.swap(other.impl);
		}
		return *this;
	}
	FontAtlas::~FontAtlas() = default;
	void FontAtlas::add_truetype(const char *filename, float font_size) { impl->add_truetype(filename, font_size); }
	void FontAtlas::add_truetype(const char *filename, std::initializer_list<float> font_sizes) { impl->add_truetype(filename, font_sizes); }
	void FontAtlas::bake(int texwidth, int texheight) { impl->bake(texwidth, texheight); }
	ursa::TextureHandle FontAtlas::tex() const { return impl->tex(); }
	FontAtlas::GlyphInfo FontAtlas::glyphInfo(int fontIndex, int codepoint) const { return impl->glyphInfo(fontIndex, codepoint); }
	FontAtlas::FontInfo FontAtlas::fontInfo(int fontIndex) const { return impl->fontInfo(fontIndex);  }

	ObjectRef<FontAtlas> font_atlas() { return FontAtlas::create_instance(); }

	std::vector<FontAtlas> FontAtlas::s_instances;

	// ...

	TextureHandle internal_texture(int width, int height, const void *data, GLenum format) {
		assert(data != nullptr);

		internal::requires_window();

		GLuint handle = 0;
		glGenTextures(1, &handle);
		// TODO set min/mag filter or some other tex parameters?

		glBindTexture(GL_TEXTURE_2D, handle);
		glTexImage2D(GL_TEXTURE_2D, 0 /*miplevel*/, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		return { handle, width, height };
	}

	TextureHandle texture(int width, int height, const void *data) {
		return internal_texture(width, height, data, GL_RGBA);
	}

	TextureHandle texture8bpp(int width, int height, const void *data) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		auto tex = internal_texture(width, height, data, GL_RED);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		tex.kind = TextureHandle::TextureKind::Alpha;
		return tex;
	}

	TextureHandle texture(const char *filename) {
		// load file
		// TODO SDL_GetBasePath() + name? need a resource management scheme
		int width = 0, height = 0, channels = 0;
		uint8_t *pixels = stbi_load(filename, &width, &height, &channels, 0 /*STBI_rgb_alpha*/);
		if (!pixels) {
			// TODO error handling
			abort();
			return { 0 };
		}

		assert((channels == 3) || (channels==4));
		GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
		TextureHandle handle = internal_texture(width, height, pixels, format);
		stbi_image_free(pixels);

		return handle;
	}

	// ...

	Rect screenrect() {
		int width = 0, height = 0;
		SDL_GetWindowSize(internal::g_window, &width, &height);
		return { {0,0}, {width,height} };
	}

	/// Set up ortho transformation with pixel coordinates
	void transform_2d() {
		int width = 0, height = 0;
		SDL_GetWindowSize(internal::g_window, &width, &height);
		glm::mat4 transform = glm::ortho(0.0f, (float)width, (float)height, 0.0f);

		GLint loc = glGetUniformLocation(internal::g_shader, "transform");
		glUniformMatrix4fv(loc, 1, GL_FALSE, &transform[0][0]);
	}

	/// Set up 3d transformation with [-1..1] coordinate range
	void transform_3d(glm::mat4 transform) {
		GLint loc = glGetUniformLocation(internal::g_shader, "transform");
		glUniformMatrix4fv(loc, 1, GL_FALSE, &transform[0][0]);
	}

	void blend_enable()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// TODO different blending modes such as additive
		//glBlendFunc(GL_ONE, GL_ONE);
	}

	void blend_disable()
	{
		glDisable(GL_BLEND);
	}

	// ...

	void clear(glm::vec4 color) {
		glClearColor(color.r, color.g, color.b, color.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void draw_triangles(Vertex vertices[], int count) {
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*count, vertices, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, count);
	}

	void draw_points(Vertex vertices[], int count) {
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*count, vertices, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_POINTS, 0, count);
	}

	void draw_lines(Vertex vertices[], int count) {
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*count, vertices, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINES, 0, count);
	}

	void internal_draw_rect(Rect r, Rect uv, glm::vec4 color) {
		Vertex vertices[6] = {
			{{r.left(),  r.top(),    0.0f}, {uv.left(),  uv.top()},    color},
			{{r.right(), r.top(),    0.0f}, {uv.right(), uv.top()},    color},
			{{r.left(),  r.bottom(), 0.0f}, {uv.left(),  uv.bottom()}, color},
			{{r.right(), r.top(),    0.0f}, {uv.right(), uv.top()},    color},
			{{r.right(), r.bottom(), 0.0f}, {uv.right(), uv.bottom()}, color},
			{{r.left(),  r.bottom(), 0.0f}, {uv.left(),  uv.bottom()}, color},
		};
		draw_triangles(vertices, 6);
	}

	void draw_rect(TextureHandle tex, Rect rect, Rect crop, glm::vec4 color)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex.handle);
		GLint texloc = glGetUniformLocation(internal::g_shader, "tex");
		GLint usetexloc = glGetUniformLocation(internal::g_shader, "use_tex");
		GLint alphatexloc = glGetUniformLocation(internal::g_shader, "alpha_tex");
		glUniform1i(texloc, 0 /*texture unit*/);
		glUniform1i(usetexloc, GL_TRUE);
		glUniform1i(alphatexloc, (tex.kind == TextureHandle::TextureKind::Alpha) ? GL_TRUE : GL_FALSE);
		Rect uv = { crop.pos / tex.size(), crop.size / tex.size() };
		internal_draw_rect(rect, uv, color);
		glUniform1i(usetexloc, GL_FALSE);
		glUniform1i(alphatexloc, GL_FALSE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void draw_rect(Rect r, glm::vec4 color) {
		internal_draw_rect(r, Rect(1, 1), color);
	}

	void draw_rect(TextureHandle tex, Rect rect, glm::vec4 color) {
		draw_rect(tex, rect, tex.bounds(), color);
	}

	void draw_rects(TextureHandle tex, Rect rects[], Rect crops[], int count)
	{
		// TODO batch it
		for (int i = 0; i < count; i++) {
			draw_rect(tex, rects[i], crops[i]);
		}
	}

	void draw_rects(TextureHandle tex, Rect rects[], Rect crops[], glm::vec4 colors[], int count)
	{
		// TODO batch it
		for (int i = 0; i < count; i++) {
			draw_rect(tex, rects[i], crops[i], colors[i]);
		}
	}

	void draw_9patch(TextureHandle tex, Rect rect, int margin, glm::vec4 color) {
		glm::vec2 border((float)margin);
		Rect crop = tex.bounds();

		// ninepatch widths ands offsets for output rect
		glm::vec2 nsize[3] = { border, rect.size - border * 2.0f, border };
		glm::vec2 npos[3] = { glm::vec2(0.0f), nsize[0], nsize[0] + nsize[1] };

		// ninepatch widths ands offsets for texture
		glm::vec2 tnsize[3] = { border, crop.size - border * 2.0f, border };
		glm::vec2 tnpos[3] = { glm::vec2(0.0f), tnsize[0], tnsize[0] + tnsize[1] };

		for (int y = 0; y < 3; y++) {
			for (int x = 0; x < 3; x++) {
				Rect drawpatch(rect.pos.x + npos[x].x,  rect.pos.y + npos[y].y,  nsize[x].x,  nsize[y].y);
				Rect croppatch(crop.pos.x + tnpos[x].x, crop.pos.y + tnpos[y].y, tnsize[x].x, tnsize[y].y);
				draw_rect(tex, drawpatch, croppatch, color);
			}
		}
	}

	void draw_text(FontAtlas::object_ref fonts, int fontIndex, float x, float y, const char *text, glm::vec4 color)
	{
		const auto &fontInfo = fonts->fontInfo(fontIndex);
		y += fontInfo.ascent;

		std::vector<Rect> rects;
		std::vector<Rect> crops;
		std::vector<glm::vec4> colors;

		for (const char *p = text; *p; p++) {
			auto info = fonts->glyphInfo(fontIndex, *p);
			rects.push_back(info.bounds.offset(x, y));
			crops.push_back(info.crop);
			colors.push_back(color);
			x += info.xadvance;
		}

		draw_rects(fonts->tex(), rects.data(), crops.data(), colors.data(), rects.size());
	}

	// ...

	EventHandler::EventHandler() : pImpl(std::make_unique<impl>()) {}
	EventHandler::~EventHandler() = default;

	struct EventHandler::impl {
		std::map<uint32_t, HandlerFunc> handlers;
	};

	void EventHandler::hook(uint32_t type, HandlerFunc func) {
		pImpl->handlers.insert_or_assign(type, func);
	}

	void EventHandler::handle(void *e) {
		auto f = pImpl->handlers.find(static_cast<SDL_Event*>(e)->type);
		if (f != pImpl->handlers.end()) {
			f->second(e);
		}
	}

	// ...

	std::function<void(float)> g_framefunc;
	std::shared_ptr<EventHandler> g_eventhandler;
	bool g_quit = false;

	void window(int width, int height) {
		internal::create_window(width, height);
	}

	void set_framefunc(std::function<void(float)> framefunc) {
		g_framefunc = framefunc;
	}

	void set_eventhandler(const std::shared_ptr<EventHandler> &handler) {
		g_eventhandler = handler;
	}

	void run() {
		internal::requires_window();

		internal::create_internal_objects();

		const int fpsLimit = 60;
		const uint32_t minimumFrameTicks = 1000 / fpsLimit;

		g_quit = false;
		SDL_Event sdlEvent;
		uint32_t lastTicks = SDL_GetTicks();
		while (!g_quit) {
			uint32_t currentTicks = SDL_GetTicks();
			uint32_t deltaTicks = currentTicks - lastTicks;
			lastTicks = currentTicks;

			while (SDL_PollEvent(&sdlEvent) != 0) {
				if (sdlEvent.type == SDL_QUIT) {
					g_quit = true;
				}
				if (g_eventhandler)
					g_eventhandler->handle(&sdlEvent);
			}

			float deltaTime = deltaTicks * 0.001f;

			if (g_framefunc)
				g_framefunc(deltaTime);

			ursa::internal::swap_window();
			
			// limit fps because swapwindow doesn't necessarily wait (e.g. if the window is completely hidden)
			uint32_t frameTicks = SDL_GetTicks() - currentTicks;
			if (frameTicks < minimumFrameTicks) {
				SDL_Delay(minimumFrameTicks - frameTicks);
			}
		}
		ursa::internal::quit();
	}

	void terminate() {
		g_quit = true;
	}

}

