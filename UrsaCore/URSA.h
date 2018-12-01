#pragma once

#include <functional>
#include <memory>

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

		glm::vec2 center() { return pos + size * 0.5f; }
		Rect centerAt(glm::vec2 center) { return {center - size*0.5f, size}; }

		Rect() {}
		Rect(glm::vec2 size) : size(size) {}
		Rect(glm::vec2 pos, glm::vec2 size) : pos(pos), size(size) {}
		Rect(float width, float height) : size(width, height) {}
		Rect(float x, float y, float width, float height) : pos(x, y), size(width, height) {}
	};

	struct TextureHandle {
		unsigned int handle;
	};

	class EventHandler {
		using HandlerFunc = std::function<void(void *)>;
		struct impl;
		std::unique_ptr<impl> pImpl;
	public:
		EventHandler();
		~EventHandler();
		// TODO do we need multiple handlers for single event? do we need unhooking?
		void hook(uint32_t type, HandlerFunc func);
		void handle(void *e);
	};

	TextureHandle texture(const char *filename);

	Rect screenrect();

	void transform_2d();
	void transform_3d(glm::mat4 transform = glm::mat4());

	void clear(glm::vec4 color = { 0.0f, 0.0f, 0.0f, 0.0f });

	void draw_triangles(Vertex vertices[], int count);
	void draw_points(Vertex vertices[], int count);

	void draw_quad(Rect rect, glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f});
	void draw_quad(TextureHandle tex, Rect rect, glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });

	void window(int width, int height);
	void set_framefunc(std::function<void(float)> framefunc);

	void set_eventhandler(const std::shared_ptr<EventHandler> &handler);

	void run();
	void terminate();
}
