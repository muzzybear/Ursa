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
#include <stb_image.h>

#include <memory>
#include <map>

/*

void main() {
	ursa::window(800,600);

	auto tex = ursa::texture("foo.png");

	ursa::set_framefunc([&](){
		Rect r = ursa::screenrect();
		Rect r2 = tex.bounds().centerAt(r.w/2, r.h/2);
		ursa::draw_quad(tex, r2);
	});

	ursa::run();
}

void main() {
	ursa::window(800,600);

	auto logo = ursa::texture("foo.png");

	std::vector<ursa::Point3> ball;
	for(int i=0; i<1000; i++) {
		ursa::Point3 pt = glm::sphericalRand(1.0f);
		ball.push_back(pt);
	}

	float angle = 0.0f;

	ursa::set_framefunc([&](float deltaTime){
		ursa::Rect r = ursa::screenrect();
		ursa::Rect r2 = logo.bounds().centerAt(r.w/2, r.h/2);

		angle += deltaTime * 0.05f;
		auto transform = glm::rotate(glm::mat4(), angle, glm::vec3(0.0f, 1.0f, 0.0f));

		// 3d mode is [-1..1] coordinates
		ursa::transform_3d(transform);
		ursa::draw_points(ball);

		// 2d mode is pixel perfect
		ursa::transform_2d();
		ursa::draw_quad(logo, r2);
	});

	ursa::run();
}


*/

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

void main()
{
	if (use_tex) {
		vec4 texcolor = texture(tex, uv);
		FragColor = texcolor * color;
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

	TextureHandle internal_texture(int width, int height, const void *data, GLenum format) {
		assert(data != nullptr);

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

	void internal_draw_quad(Rect r, Rect uv, glm::vec4 color) {
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

	void draw_quad(TextureHandle tex, Rect rect, Rect crop, glm::vec4 color)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex.handle);
		GLint texloc = glGetUniformLocation(internal::g_shader, "tex");
		GLint usetexloc = glGetUniformLocation(internal::g_shader, "use_tex");
		glUniform1i(texloc, 0 /*texture unit*/);
		glUniform1i(usetexloc, GL_TRUE);
		Rect uv = { crop.pos / tex.size(), crop.size / tex.size() };
		internal_draw_quad(rect, uv, color);
		glUniform1i(usetexloc, GL_FALSE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void draw_quad(Rect r, glm::vec4 color) {
		internal_draw_quad(r, Rect(1, 1), color);
	}

	void draw_quad(TextureHandle tex, Rect rect, glm::vec4 color) {
		draw_quad(tex, rect, tex.bounds(), color);
	}

	void draw_9patch(TextureHandle tex, Rect rect, int margin, glm::vec4 color) {
		float border = margin;
		// ninepatch widths for output rect
		float nw[3] = { border, rect.size.x - border * 2, border };
		float nh[3] = { border, rect.size.y - border * 2, border };
		// ninepatch offsets
		float nx[3] = { 0, nw[0], nw[0] + nw[1] };
		float ny[3] = { 0, nh[0], nh[0] + nh[1]	};

		const float tw = tex.width;
		const float th = tex.height;
		Rect crop = tex.bounds();

		// ninepatch widths for texture
		float tnw[3] = { border, tw - border * 2, border };
		float tnh[3] = { border, th - border * 2, border };
		// and the offset
		float tnx[3] = { 0, tnw[0], tnw[0] + tnw[1] };
		float tny[3] = { 0, tnh[0], tnh[0] + tnh[1] };

		for (int y = 0; y < 3; y++) {
			for (int x = 0; x < 3; x++) {
				Rect patch(rect.pos.x + nx[x], rect.pos.y + ny[y], nw[x], nh[y]);
				Rect croppatch(crop.pos.x + tnx[x], crop.pos.y + tny[y], tnw[x], tnh[y]);
				draw_quad(tex, patch, croppatch, color);

			}
		}
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
		// default to windowed mode in 800x600
		if (!internal::g_window)
			internal::create_window(800, 600);

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

