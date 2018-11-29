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

	namespace internal {
		void create_internal_objects();
		void swap_window();
		void quit();
	}

	void transform_2d();

	void draw_triangles(Vertex vertices[], int count);

	void window(int width, int height);
	void set_framefunc(std::function<void(float)> framefunc);

	void run();
}
