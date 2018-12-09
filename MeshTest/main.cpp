// UNFINISHED, PERHAPS ABANDONED

// SDL2 header required for the SDL_main macro
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "URSA.h"

#include <vector>
#include <map>
#include <memory>
#include <glm/gtc/random.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// TODO turn into a class, write accessors for everything
struct Camera {
	float m_aspectRatio = 4.0f / 3.0f;
	float m_near = 0.1f, m_far = 1000.0f;
	glm::vec3 m_position = {0,0,0};
	glm::quat m_rotation = glm::quat();

	glm::mat4 projection() const {
		float fovy = 45.0f;
		return glm::perspective(fovy, m_aspectRatio, m_near, m_far);
	}

	glm::mat4 view() const {
		return glm::mat4_cast(m_rotation) * glm::translate(glm::mat4(1.0f), -m_position);
	}

	glm::mat4 transform() const {
		return projection() * view();
	}

	glm::vec3 forward() const { return glm::vec3(0, 0, -1) * m_rotation; }
	glm::vec3 right() const { return glm::vec3(1, 0, 0) * m_rotation; }
	glm::vec3 up() const { return glm::vec3(0, 1, 0) * m_rotation; }
};

// ...

struct DM_Vertex {
	float x, y, z;
	struct DM_Edge *e;
};
struct DM_Edge {
	DM_Vertex *start;
	DM_Edge *opposite;
	DM_Edge *next;
	struct DM_Face *f;
};
struct DM_Face {
	DM_Edge *e;
};

// DynamicMesh is for procedurally generating a mesh
struct DynamicMesh {
	// TODO vertice, edges, faces
	// for now, only keeping track of an arbitrary face, everything else will be connected to it
	DM_Face *anyface;
};

// SimpleMesh is for rendering a mesh
struct SimpleMesh {
	std::vector<ursa::Vertex> vertices;
	// TODO triangle indices
};

int main(int argc, char *argv[])
{
	ursa::window(800, 600);

	std::vector<ursa::Vertex> ball;
	for (int i = 0; i < 1000; i++) {
		ursa::Vertex v{ glm::sphericalRand(1.0f), {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} };
		ball.push_back(v);
	}

	float angle = 0.0f;

	Camera camera;
	camera.m_position = {0,0,5};

	ursa::set_framefunc([&](float deltaTime) {
		ursa::clear({ 0.20f, 0.32f, 0.35f, 1.0f });

		angle += deltaTime * 0.25f;
		glm::mat4 modeltransform = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

		auto MVP = camera.transform() * modeltransform;

		ursa::transform_3d(MVP);
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
