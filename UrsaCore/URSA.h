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

		float left() const { return pos.x; }
		float right() const { return pos.x + size.x; }
		float top() const { return pos.y; }
		float bottom() const { return pos.y + size.y; }

		glm::vec2 center() { return pos + size * 0.5f; }
		Rect centerAt(glm::vec2 center) { return {center - size*0.5f, size}; }
		Rect expand(float margin) { return { pos - glm::vec2(margin), size + glm::vec2(margin * 2) }; }
		Rect offset(glm::vec2 offset) { return { pos + offset, size }; }
		Rect offset(float x, float y) { return offset({ x,y }); }

		Rect pixelAlign() { return { {floor(pos.x+0.5f), floor(pos.y+0.5f)} , size }; }

		Rect alignRight(float x) { return { {x-size.x, pos.y}, size }; }
		Rect alignBottom(float y) { return { {pos.x, y-size.y}, size }; }

		Rect() : pos(), size() {}
		Rect(glm::vec2 size) : pos(), size(size) {}
		Rect(glm::vec2 pos, glm::vec2 size) : pos(pos), size(size) {}
		Rect(float width, float height) : pos(), size(width, height) {}
		Rect(float x, float y, float width, float height) : pos(x, y), size(width, height) {}

		Rect operator*(const glm::vec2 &scale) { return { pos*scale, size*scale }; }
	};

	struct TextureHandle {
		unsigned int handle;
		// Carrying metadata with the handle to avoid lookups
		// The downside is that different copies of the handle might get out of sync
		// TODO consider some shared_ptr mechanic to automatically delete the opengl resources and keep metadata in sync
		// TODO consider some Ref<type> object pools as seen in http://etodd.io/2018/02/20/poor-mans-netcode/
		int width, height;
		enum TextureKind {
			RGBA, Alpha
		} kind = RGBA;

		glm::vec2 size() { return { (float)width, (float)height }; }
		Rect bounds() { return Rect((float)width, (float)height); }
	};

	class FontAtlas {
	public:
		struct GlyphInfo {
			Rect crop;
			Rect bounds;
			float xadvance;
		};

		struct FontInfo {
			float ascent;
			float descent;
			float linegap;
		};

		FontAtlas();
		FontAtlas(const FontAtlas & other);
		FontAtlas(FontAtlas && other);
		FontAtlas& operator=(const FontAtlas & other);
		FontAtlas& operator=(FontAtlas && other);
		~FontAtlas();
		
		void add_truetype(const char *filename, float font_size);
		void add_truetype(const char *filename, std::initializer_list<float> font_sizes);

		void bake(int texwidth, int texheight);

		TextureHandle tex() const;
		GlyphInfo glyphInfo(int fontIndex, int codepoint) const;
		FontInfo fontInfo(int fontIndex) const;
	private:
		std::unique_ptr<class FontAtlasImpl> impl;
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
	TextureHandle texture(int width, int height, const void *data);
	TextureHandle texture8bpp(int width, int height, const void *data);

	FontAtlas font_atlas();

	Rect screenrect();

	void transform_2d();
	void transform_3d(glm::mat4 transform = glm::mat4());

	void blend_enable();
	void blend_disable();

	void clear(glm::vec4 color = { 0.0f, 0.0f, 0.0f, 0.0f });

	void draw_triangles(Vertex vertices[], int count);
	void draw_points(Vertex vertices[], int count);
	void draw_lines(Vertex vertices[], int count);

	void draw_quad(Rect rect, glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f});
	void draw_quad(TextureHandle tex, Rect rect, glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });
	void draw_quad(TextureHandle tex, Rect rect, Rect crop, glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });

	void draw_quads(TextureHandle tex, Rect rects[], Rect crops[], int count);
	void draw_quads(TextureHandle tex, Rect rects[], Rect crops[], glm::vec4 colors[], int count);

	void draw_9patch(TextureHandle tex, Rect rect, int margin, glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });

	void draw_text(const FontAtlas &fonts, int fontIndex, float x, float y, const char *text, glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });

	void window(int width, int height);
	void set_framefunc(std::function<void(float)> framefunc);

	void set_eventhandler(const std::shared_ptr<EventHandler> &handler);

	void run();
	void terminate();
}
