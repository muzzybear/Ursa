// SDL2 header required for the SDL_main macro
#include <SDL2/SDL.h>
#include <glad/glad.h>

#include "URSA.h"

int main(int argc, char *argv[])
{
	ursa::window(800, 600);

	ursa::Vertex vertices[] = {
		{{50.f, 150.f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{100.f, 50.f, 0.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{150.0f, 150.f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}
	};

	ursa::set_framefunc([&](float deltaTime) {
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ursa::transform_2d();
		ursa::draw_triangles(vertices, 3);
	});

	ursa::run();

	return 0;
}
