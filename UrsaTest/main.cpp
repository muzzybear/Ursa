// SDL2 header required for the SDL_main macro
#include <SDL2/SDL.h>
#include <glad/glad.h>

#include "URSA.h"

#include <vector>
#include <algorithm>
#include <regex>
#include <glm/gtc/random.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct RectList {
	std::vector<ursa::Rect> quads;
	std::vector<ursa::Rect> crops;
	std::vector<glm::vec4> colors;
};

class TextBlock {
	struct textspan {
		std::vector<std::string> tokens;
		glm::vec4 color;
		int fontIndex;
	};
	struct textline {
		std::vector<textspan> spans;
	};

	const std::regex ws_re{ "\\s+" };

public:
	void clear() {
		lines.clear();
	}

	void append(std::string text, glm::vec4 color = { 1.0f,1.0f,1.0f,1.0f }, int fontIndex = 0) {
		if (lines.empty()) {
			newline();
		}

		std::vector<std::string> tokens;
		std::copy(
			std::sregex_token_iterator(text.begin(), text.end(), ws_re, { -1, 0 }),
			std::sregex_token_iterator(),
			std::back_inserter(tokens));

		lines.back().spans.push_back({ tokens, color, fontIndex });
	}

	void newline() {
		lines.emplace_back();
	}
	RectList buildRects(const ursa::FontAtlas &fonts, ursa::Rect bounds) {
		RectList rects;
		float x{ bounds.pos.x }, y{ bounds.pos.y };
		for (const auto &line : lines) {
			// perform word wrapping into virtual lines, count tokens per vline to be used for geometry pass
			// TODO whitespace token should probably be skipped upon wrapping
			struct virtual_line {
				float gap{ 0 }, baseline{ 0 }, descent{ 0 };
				int tokens{ 0 };
			};
			std::vector<virtual_line> vlines{ {} };
			float vx = 0;
			for (const auto &span : line.spans) {
				const auto &fontInfo = fonts.fontInfo(span.fontIndex);

				for (const auto &token : span.tokens) {
					float tokenWidth = 0;
					// FIXME using xadvance isn't entirely accurate
					for (const auto &ch : token) {
						auto info = fonts.glyphInfo(span.fontIndex, ch);
						tokenWidth += info.xadvance;
					}
					if (vx > 0 && vx + tokenWidth > bounds.size.x) {
						// wrapped, start new vline and place token there
						// TODO support splitting full line tokens?
						vx = 0;
						vlines.emplace_back();
					}
					vx += tokenWidth;
					// slightly unnecessary to repeat this for every token, but it
					// will reliably work even when the first token of the span wraps
					auto &vline = vlines.back();
					vline.tokens++;
					vline.gap = std::max(vline.gap, fontInfo.linegap);
					vline.baseline = std::max(vline.baseline, fontInfo.ascent);
					vline.descent = std::min(vline.descent, fontInfo.descent);
				}
			}
			// start from the first vline's baseline
			y += vlines.front().baseline;

			// gather the geometry for the line
			int tokencounter = 0;
			int vlinecounter = 0;
			for (const auto &span : line.spans) {
				for (const auto &token : span.tokens) {
					// test wrapping
					if (tokencounter >= vlines[vlinecounter].tokens) {
						x = bounds.pos.x;
						// move to next vline's baseline
						y += vlines[vlinecounter].gap + vlines[vlinecounter+1].baseline - vlines[vlinecounter].descent;
						vlinecounter++;
						tokencounter = 0;
					}
					// generate token's geometry
					for (const auto &ch : token) {
						auto info = fonts.glyphInfo(span.fontIndex, ch);
						rects.quads.push_back(info.bounds.offset(x, y));
						rects.crops.push_back(info.crop);
						rects.colors.push_back(span.color);
						x += info.xadvance;
					}
					tokencounter++;
				}
			}

			// next line
			x = bounds.pos.x;
			y += vlines.back().gap-vlines.back().descent;
		}
		return rects;
	}
private:
	std::vector<textline> lines;
};

int main(int argc, char *argv[])
{
	ursa::window(800, 600);

	ursa::Vertex vertices[] = {
		{{50.f, 150.f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{100.f, 50.f, 0.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{150.0f, 150.f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}
	};

	uint32_t c1{ 0xffe0e0e0 }, c2{ 0xff808080 }, c3{ 0xff404040 }, c_{ 0xff604020 };
	uint32_t panel_pixels[] = {
		c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c2,
		c1,c2,c2,c2,c2,c2,c2,c2,c2,c2,c2,c2,c2,c2,c2,c3,
		c1,c2,c_,c_,c_,c2,c_,c_,c_,c_,c2,c_,c_,c_,c2,c3,
		c1,c2,c_,c2,c_,c2,c_,c2,c2,c_,c2,c_,c2,c_,c2,c3,
		c1,c2,c_,c_,c_,c_,c_,c2,c2,c_,c_,c_,c_,c_,c2,c3,
		c1,c2,c2,c2,c_,c2,c2,c2,c2,c2,c2,c_,c2,c2,c2,c3,
		c1,c2,c_,c_,c_,c2,c2,c2,c2,c2,c2,c_,c_,c_,c2,c3,
		c1,c2,c_,c2,c2,c2,c2,c2,c2,c2,c2,c2,c2,c_,c2,c3,
		c1,c2,c_,c2,c2,c2,c2,c2,c2,c2,c2,c2,c2,c_,c2,c3,
		c1,c2,c_,c_,c_,c2,c2,c2,c2,c2,c2,c_,c_,c_,c2,c3,
		c1,c2,c2,c2,c_,c2,c2,c2,c2,c2,c2,c_,c2,c2,c2,c3,
		c1,c2,c_,c_,c_,c_,c_,c2,c2,c_,c_,c_,c_,c_,c2,c3,
		c1,c2,c_,c2,c_,c2,c_,c2,c2,c_,c2,c_,c2,c_,c2,c3,
		c1,c2,c_,c_,c_,c2,c_,c_,c_,c_,c2,c_,c_,c_,c2,c3,
		c1,c2,c2,c2,c2,c2,c2,c2,c2,c2,c2,c2,c2,c2,c2,c3,
		c2,c3,c3,c3,c3,c3,c3,c3,c3,c3,c3,c3,c3,c3,c3,c3,
	};

	auto paneltex = ursa::texture(16, 16, panel_pixels);

	auto tex = ursa::texture(R"(C:\Windows\Web\Wallpaper\Theme1\img1.jpg)");

	std::vector<ursa::Vertex> ball;
	for (int i = 0; i < 1000; i++) {
		ursa::Vertex v { glm::sphericalRand(1.0f), {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} };
		ball.push_back(v);
	}

	float angle = 0.0f;

	auto fonts = ursa::font_atlas();
	fonts.add_truetype(R"(c:\windows\fonts\arialbd.ttf)", { 18.0f, 36.0f });
	fonts.add_truetype(R"(c:\windows\fonts\arial.ttf)", 18.0f);
	fonts.add_truetype(R"(c:\windows\fonts\comic.ttf)", 24.0f);
	fonts.bake(512, 512);

	// ...

	ursa::set_framefunc([&](float deltaTime) {
		ursa::clear({0.20f, 0.32f, 0.35f, 1.0f});
		
		ursa::transform_2d();
		ursa::draw_triangles(vertices, 3);

		ursa::Rect r = ursa::screenrect();
		ursa::Rect r2 = ursa::Rect(400,200).centerAt(r.center());
		ursa::Rect r3 = r2.expand(8);
		ursa::draw_9patch(paneltex, r3, 8);
		ursa::draw_quad(tex, r2);

		angle += deltaTime * 0.05f;
		auto transform = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

		ursa::transform_3d(transform);
		ursa::draw_points(ball.data(), ball.size());

		ursa::transform_2d();

		// TODO ursa::draw_text
		TextBlock tb;
		tb.append("The quick brown fox jumps over the lazy dog");
		tb.newline();
		tb.append("Testing line gap code");
		tb.newline();
		tb.append("... just ", {1.0f,1.0f,1.0f,1.0f}, 2);
		tb.append("Testing", {1.0f, 0.3f, 0.8f, 1.0f}, 1);
		tb.newline();
		tb.append("Yatta!");
		tb.newline();
		tb.append("Isn't it amazing?", {1.0f, 1.0f, 1.0f, 0.6f}, 3);

		auto textrect = ursa::Rect{ 32,32,100,400 };
		RectList rects = tb.buildRects(fonts, textrect);

		ursa::blend_enable();
		ursa::draw_quad(textrect, { 0.0f,0.0f,0.0f,0.4f });
		ursa::draw_quad(fonts.tex(), ursa::Rect(512, 512).alignRight(ursa::screenrect().right()), glm::vec4{0.5f, 0.5f, 1.0f, 0.5f});
		ursa::draw_quads(fonts.tex(), rects.quads.data(), rects.crops.data(), rects.colors.data(), rects.quads.size());
		ursa::blend_disable();

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
