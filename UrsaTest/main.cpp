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
		std::string text;
		glm::vec4 color;
		int fontIndex;
	};
	struct texttoken {
		std::vector<textspan> spans;
	};
	struct textline {
		std::vector<texttoken> tokens;
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
		// the regex token iterator will create empty token if input begins with whitespace, but not if it ends with whitespace
		std::copy(
			std::sregex_token_iterator(text.begin(), text.end(), ws_re, { -1, 0 }),
			std::sregex_token_iterator(),
			std::back_inserter(tokens));

		auto iter = tokens.begin();
		// merge the first new token into the last old token, if applicable
		auto &linetokens = lines.back().tokens;
		if (linetokens.size() > 0) {
			bool lastIsWS = std::regex_match(linetokens.back().spans.back().text, ws_re);
			// if the line begins with whitespace, the first token is empty
			if (iter->size() == 0) {
				// skip the dummy zero-length token
				++iter;
				// check the last token on the line, to test for two adjancent whitespace tokens
				// TODO maybe texttoken itself should know if it's a whitespace token
				if (iter != tokens.end() && lastIsWS) {
					// combine whitespaces
					// TODO ideally, if the styles are equal, new span isn't needed
					linetokens.back().spans.push_back({ *iter++, color, fontIndex });
				}
			} else {
				// first new token isn't whitespace
				if (!lastIsWS) {
					// ... and neither is the last of the old ones, merge
					linetokens.back().spans.push_back({ *iter++, color, fontIndex });
				}
			}
		}
		
		// process the rest normally
		for (; iter != tokens.end(); ++iter) {
			// skip zero-length tokens
			if (iter->size() == 0)
				continue;
			linetokens.emplace_back();
			linetokens.back().spans.push_back({*iter, color, fontIndex});
		}
	}

	void newline() {
		lines.emplace_back();
	}
	RectList buildRects(const ursa::FontAtlas &fonts, ursa::Rect bounds) {
		RectList rects;
		float x{ bounds.pos.x }, y{ bounds.pos.y };
		for (const auto &line : lines) {
			// perform word wrapping into virtual lines, count tokens per vline to be used for geometry pass
			// TODO should whitespace tokens by omitted from rendering when they are adjacent to the wrap position?
			struct virtual_line {
				float gap{ 0 }, baseline{ 0 }, descent{ 0 };
				int tokens{ 0 };
			};
			std::vector<virtual_line> vlines{ {} };
			float vx = 0;
			for (const auto &token : line.tokens) {
				float tokenWidth = 0;
				for (const auto &span : token.spans) {
					const auto &fontInfo = fonts.fontInfo(span.fontIndex);
					// FIXME using xadvance isn't entirely accurate
					for (const auto &ch : span.text) {
						auto info = fonts.glyphInfo(span.fontIndex, ch);
						tokenWidth += info.xadvance;
					}
				}
				if (vx > 0 && vx + tokenWidth > bounds.size.x) {
					// wrapped, start new vline and place token there
					// TODO support splitting full line tokens?
					vx = 0;
					vlines.emplace_back();
				}
				vx += tokenWidth;

				// update current vline metrics based on token's sub-span metrics
				auto &vline = vlines.back();
				for (const auto &span : token.spans) {
					const auto &fontInfo = fonts.fontInfo(span.fontIndex);
					vline.gap = std::max(vline.gap, fontInfo.linegap);
					vline.baseline = std::max(vline.baseline, fontInfo.ascent);
					vline.descent = std::min(vline.descent, fontInfo.descent);
				}
				vline.tokens++;
			}
			// start from the first vline's baseline
			y += vlines.front().baseline;

			// gather the geometry for the line
			int tokencounter = 0;
			int vlinecounter = 0;
			for (const auto &token : line.tokens) {
				// test wrapping
				if (tokencounter >= vlines[vlinecounter].tokens) {
					x = bounds.pos.x;
					// move to next vline's baseline
					y += vlines[vlinecounter].gap + vlines[vlinecounter + 1].baseline - vlines[vlinecounter].descent;
					vlinecounter++;
					tokencounter = 0;
				}
				// generate token's geometry
				for (const auto &span : token.spans) {
					for (const auto &ch : span.text) {
						auto info = fonts.glyphInfo(span.fontIndex, ch);
						rects.quads.push_back(info.bounds.offset(x, y));
						rects.crops.push_back(info.crop);
						rects.colors.push_back(span.color);
						x += info.xadvance;
					}
				}
				tokencounter++;
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

class EditLine {
public:
	void input(std::string text) {
		line.insert(cursor, text);
		cursor += text.length();
	}
	std::string contents() {
		return line;
	}
	void clear() {
		line.clear();
		cursor = 0;
	}
	void offsetCursor(int offset) {
		cursor = std::max(0, std::min(cursor+offset, (int)line.length()));
	}
	void deleteOffset(int offset) {
		// when deleting backwards, move cursor first and delete forwards up to current position
		if (offset < 0) {
			int tmp = cursor;
			offsetCursor(offset);
			offset = tmp - cursor;
		}
		line.erase(cursor, offset);
	}
	// TODO horizontal scroll state, so it's possible to edit a line longer than window width
	// TODO perhaps the line should be vertically centered inside the editing bounds
	// TODO there should be an API for getting suggested height for the editline
	RectList buildRects(const ursa::FontAtlas &fonts, int fontIndex, glm::vec4 color, ursa::Rect bounds, ursa::Rect *cursorRect) {
		RectList rects;
		float x{ bounds.pos.x }, y{ bounds.pos.y };
		float cursorx{ x };

		const auto &fontInfo = fonts.fontInfo(fontIndex);
		y += fontInfo.ascent;

		for (size_t i = 0; i < line.length(); i++) {
			const auto &ch = line[i];
			auto info = fonts.glyphInfo(fontIndex, ch);
			rects.quads.push_back(info.bounds.offset(x, y));
			rects.crops.push_back(info.crop);
			rects.colors.push_back(color);
			x += info.xadvance;
			// rather than handling end-of-line case separately,
			// just check if cursor is after the current glyph and leave the cursor=0 fall to default value
			if (i+1 == cursor) {
				cursorx = x;
			}
		}

		// TODO hardcoded cursor width of 2 pixels is not necessarily a good thing
		*cursorRect = {cursorx, bounds.pos.y, 2, fontInfo.ascent};

		return rects;
	}
private:
	int cursor{0};
	std::string line;
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

	TextBlock tb;
	tb.append("The quick brown fox jumps over the lazy dog");
	tb.newline();
	tb.append("Testing line gap code");
	tb.newline();
	tb.append("... just ", { 1.0f,1.0f,1.0f,1.0f }, 2);
	tb.append("Testing", { 1.0f, 0.3f, 0.8f, 1.0f }, 1);
	tb.newline();
	tb.append("Yatta!");
	tb.newline();
	tb.append("Isn't it amazing?", { 1.0f, 1.0f, 1.0f, 0.6f }, 3);
	tb.newline();

	EditLine editline;

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

		auto textrect = ursa::Rect{ 32,32,100,400 };
		auto inputrect = ursa::Rect{ ursa::screenrect().size.x, 32 }.alignBottom(ursa::screenrect().bottom());

		ursa::blend_enable();
		ursa::draw_quad(textrect, { 0.0f,0.0f,0.0f,0.4f });
		ursa::draw_quad(fonts.tex(), ursa::Rect(512, 512).alignRight(ursa::screenrect().right()), glm::vec4{0.5f, 0.5f, 1.0f, 0.5f});
		RectList rects = tb.buildRects(fonts, textrect);
		ursa::draw_quads(fonts.tex(), rects.quads.data(), rects.crops.data(), rects.colors.data(), rects.quads.size());

		ursa::Rect cursor;
		ursa::draw_quad(inputrect, { 0.0f,0.0f,0.0f,0.4f });
		rects = editline.buildRects(fonts, 0, glm::vec4{ 1.0f,1.0f,1.0f,1.0f }, inputrect, &cursor);
		ursa::draw_quads(fonts.tex(), rects.quads.data(), rects.crops.data(), rects.colors.data(), rects.quads.size());
		// TODO make the cursor blink
		ursa::draw_quad(cursor, {0.8f,0.8f,0.8f,0.8f});

		ursa::blend_disable();

	});

	auto events = std::make_shared<ursa::EventHandler>();
	ursa::set_eventhandler(events);

	SDL_StartTextInput();
	events->hook(SDL_TEXTINPUT, [&](void *e) {
		auto *event = static_cast<SDL_TextInputEvent*>(e);
		editline.input(event->text);
	});

	events->hook(SDL_KEYDOWN, [&](void *e) {
		auto *event = static_cast<SDL_KeyboardEvent*>(e);
		if (event->keysym.scancode == SDL_SCANCODE_ESCAPE) {
			ursa::terminate();
		}

		switch (event->keysym.scancode) {
		case SDL_SCANCODE_RETURN:
			tb.append(editline.contents());
			tb.newline();
			editline.clear();
			break;
		case SDL_SCANCODE_LEFT:
			editline.offsetCursor(-1);
			break;
		case SDL_SCANCODE_RIGHT:
			editline.offsetCursor(+1);
			break;
		case SDL_SCANCODE_BACKSPACE:
			editline.deleteOffset(-1);
			break;
		case SDL_SCANCODE_DELETE:
			editline.deleteOffset(+1);
			break;
		}
	});

	ursa::run();
	
	return 0;
}
