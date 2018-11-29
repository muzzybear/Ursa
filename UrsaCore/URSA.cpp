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

/*

void main() {
	auto tex = ursa::texture("foo.png");

	ursa::set_framefunc([&](){
		ursa::draw_quad(tex, 200,200, 100,100);
	});

	ursa::run();
}

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

void main()
{
    FragColor = color;
})";
		GLuint make_shader(GLenum shaderType, const char *source) {
			create_internal_objects();

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

			initialized = true;
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

	/// Set up ortho transformation with pixel coordinates
	void transform_2d() {
		int width = 0, height = 0;
		SDL_GetWindowSize(internal::g_window, &width, &height);
		glm::mat4 transform = glm::ortho(0.0f, (float)width, (float)height, 0.0f);

		GLint loc = glGetUniformLocation(internal::g_shader, "transform");
		glUniformMatrix4fv(loc, 1, GL_FALSE, &transform[0][0]);
	}

	// ...

	void draw_triangles(Vertex vertices[], int count) {
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*count, vertices, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, count);
	}

	// ...

	std::function<void(float)> g_framefunc;

	void window(int width, int height) {
		internal::create_window(width, height);
	}

	void set_framefunc(std::function<void(float)> framefunc) {
		g_framefunc = framefunc;
	}	

	void run() {
		// default to windowed mode in 800x600
		if (!internal::g_window)
			internal::create_window(800, 600);

		internal::create_internal_objects();

		bool quit = false;
		SDL_Event sdlEvent;
		while (!quit)
		{
			while (SDL_PollEvent(&sdlEvent) != 0)
			{
				if (sdlEvent.type == SDL_QUIT)
				{
					quit = true;
				}
			}

			// FIXME implement
			float deltaTime = 0.0f;

			if (g_framefunc)
				g_framefunc(deltaTime);

			ursa::internal::swap_window();
		}
		ursa::internal::quit();
	}

}

