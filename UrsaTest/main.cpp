// SDL2 header required for the SDL_main macro
#include <SDL2/SDL.h>
#include <glad/glad.h>

#include "URSA.h"

#include <vector>
#include <glm/gtc/random.hpp>
#include <glm/gtc/matrix_transform.hpp>

int main(int argc, char *argv[])
{
	ursa::window(800, 600);

	ursa::Vertex vertices[] = {
		{{50.f, 150.f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{100.f, 50.f, 0.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{150.0f, 150.f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}
	};

	auto tex = ursa::texture(R"(C:\Windows\Web\Wallpaper\Theme1\img1.jpg)");

	std::vector<ursa::Vertex> ball;
	for (int i = 0; i < 1000; i++) {
		ursa::Vertex v { glm::sphericalRand(1.0f), {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} };
		ball.push_back(v);
	}

	float angle = 0.0f;

	ursa::set_framefunc([&](float deltaTime) {
		ursa::clear({0.20f, 0.32f, 0.35f, 1.0f});
		
		ursa::transform_2d();
		ursa::draw_triangles(vertices, 3);

		ursa::Rect r = ursa::screenrect();
		ursa::Rect r2 = ursa::Rect(400,200).centerAt(r.center());
		ursa::draw_quad(tex, r2);		

		angle += deltaTime * 0.05f;
		auto transform = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

		// 3d mode is [-1..1] coordinates
		ursa::transform_3d(transform);
		ursa::draw_points(ball.data(), ball.size());

	});

	auto events = std::make_shared<ursa::EventHandler>();
	ursa::set_eventhandler(events);

	events->hook(SDL_KEYDOWN, [&](void *e) {
		auto *event = static_cast<SDL_KeyboardEvent*>(e);
		if (event->keysym.scancode == SDL_SCANCODE_ESCAPE) {
			ursa::terminate();
		}
	});

	ursa::run();

	return 0;
}
