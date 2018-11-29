#pragma once

#include <functional>

#include <glm/glm.hpp>

namespace ursa {

	// TODO normals
	// TODO should vertex color just multiply texture, or some fancier operation (e.g. photoshop "overlay") to allow colorization with white colors intact?
	struct Vertex {
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec4 color;
	};

	struct Rect {
		glm::vec2 pos;
		glm::vec2 size;

		Rect centerAt(glm::vec2 center) {
			return {center - size*0.5f, size};
		}
	};

	namespace internal {
		void create_internal_objects();
		void swap_window();
		void quit();
	}

	Rect screenrect();

	void transform_2d();

	void draw_triangles(Vertex vertices[], int count);

	void draw_quad(Rect rect, glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f});

	void window(int width, int height);
	void set_framefunc(std::function<void(float)> framefunc);

	void run();
}
