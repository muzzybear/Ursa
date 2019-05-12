// SDL2 header required for the SDL_main macro
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "URSA.h"

#include <vector>
#include <map>
#include <memory>

struct StrokeEdge;

struct StrokeVertex {
	glm::vec2 pos;

	StrokeVertex(glm::vec2 pos) : pos(pos) {}
};

struct StrokeEdge {
	StrokeVertex *p1, *p2;
	glm::vec2 slant;
	float thickness;
	float adjust; // adjust stroke position along slant direction, relative to stroke radius

	StrokeEdge(StrokeVertex *p1, StrokeVertex *p2, float thickness=1.0f, glm::vec2 slant = {1,0}, float adjust=0.0f)
		: p1(p1), p2(p2), thickness(thickness), slant(glm::normalize(slant)), adjust(adjust)
	{}

};

struct Glyph {
	std::vector<std::unique_ptr<StrokeVertex>> vertices;
	std::vector<std::unique_ptr<StrokeEdge>> strokes;

	void vertex(glm::vec2 pos) {
		vertices.push_back(std::make_unique<StrokeVertex>(pos));
	}
	void stroke(int i1, int i2, float thickness = 1.0f, glm::vec2 slant = {1,0}, float adjust=0.0f) {
		strokes.push_back(std::make_unique<StrokeEdge>(vertices[i1].get(), vertices[i2].get(), thickness, slant, adjust));
	}
};

void render_glyph(Glyph *glyph, ursa::Rect outrect, float baseThickness, bool lines, bool fill) {
	if (lines)
		ursa::draw_rect(outrect, { 0.45f,0.45f,0.45f,1.0f });

	std::vector<ursa::Vertex> verts;
	std::vector<ursa::Vertex> fillverts;
	for (auto &gv : glyph->strokes) {
		auto v1 = outrect.pos + gv->p1->pos * outrect.size;
		auto v2 = outrect.pos + gv->p2->pos * outrect.size;
		// strokeline
		verts.push_back(ursa::Vertex{ {v1.x, v1.y, 0.0f}, {0.0f, 0.0f}, {1.0f,0.0f,0.0f,1.0f} });
		verts.push_back(ursa::Vertex{ {v2.x, v2.y, 0.0f}, {0.0f, 0.0f}, {1.0f,0.0f,0.0f,1.0f} });
		// slant
		glm::vec2 slantvec = gv->thickness * baseThickness * gv->slant;
		verts.push_back(ursa::Vertex{ glm::vec3(v1 + slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, {0.0f,1.0f,1.0f,1.0f} });
		verts.push_back(ursa::Vertex{ glm::vec3(v1 - slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, {0.0f,1.0f,1.0f,1.0f} });
		verts.push_back(ursa::Vertex{ glm::vec3(v2 + slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, {0.0f,1.0f,1.0f,1.0f} });
		verts.push_back(ursa::Vertex{ glm::vec3(v2 - slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, {0.0f,1.0f,1.0f,1.0f} });
		// stroke edges
		verts.push_back(ursa::Vertex{ glm::vec3(v1 + slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, {0.0f,0.0f,0.0f,1.0f} });
		verts.push_back(ursa::Vertex{ glm::vec3(v2 + slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, {0.0f,0.0f,0.0f,1.0f} });
		verts.push_back(ursa::Vertex{ glm::vec3(v1 - slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, {0.0f,0.0f,0.0f,1.0f} });
		verts.push_back(ursa::Vertex{ glm::vec3(v2 - slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, {0.0f,0.0f,0.0f,1.0f} });
		// fill
		glm::vec4 fillcolor{ 0.3f, 0.3f, 0.4f, 1.0f };
		fillverts.push_back(ursa::Vertex{ glm::vec3(v1 - slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, fillcolor });
		fillverts.push_back(ursa::Vertex{ glm::vec3(v1 + slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, fillcolor });
		fillverts.push_back(ursa::Vertex{ glm::vec3(v2 - slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, fillcolor });

		fillverts.push_back(ursa::Vertex{ glm::vec3(v1 + slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, fillcolor });
		fillverts.push_back(ursa::Vertex{ glm::vec3(v2 + slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, fillcolor });
		fillverts.push_back(ursa::Vertex{ glm::vec3(v2 - slantvec + gv->adjust * slantvec, 0), {0.0f, 0.0f}, fillcolor });
	}

	if (fill)
		ursa::draw_triangles(fillverts.data(), fillverts.size());
	if (lines)
		ursa::draw_lines(verts.data(), verts.size());
}

int main(int argc, char *argv[])
{
	ursa::window(1024, 768);

	std::map<char, Glyph> glyphs;

	glyphs['A'].vertex({ 0.0f, 1.0f });
	glyphs['A'].vertex({ 0.5f, 0.0f });
	glyphs['A'].vertex({ 1.0f, 1.0f });
	glyphs['A'].vertex({ 0.5f/3, 2.0f/3 });
	glyphs['A'].vertex({ 1.0f-0.5f/3, 2.0f/3 });
	glyphs['A'].stroke(0,1);
	glyphs['A'].stroke(1,2);
	glyphs['A'].stroke(3, 4, 0.6f, {0,1});

	glyphs['B'].vertex({ 0.0f, 0.0f });
	glyphs['B'].vertex({ 0.0f, 0.5f });
	glyphs['B'].vertex({ 0.0f, 1.0f });
	glyphs['B'].vertex({ 0.5f, 0.0f });
	glyphs['B'].vertex({ 0.5f, 0.5f });
	glyphs['B'].vertex({ 0.5f, 1.0f });
	glyphs['B'].vertex({ 1.0f, 0.25f });
	glyphs['B'].vertex({ 1.0f, 0.75f });
	glyphs['B'].stroke(0, 1);
	glyphs['B'].stroke(1, 2);
	glyphs['B'].stroke(0, 3, 0.6f, {1,2}, 1.0f);
	glyphs['B'].stroke(1, 4, 0.6f, {0,1});
	glyphs['B'].stroke(2, 5, 0.6f, {-1,2}, -1.0f);
	glyphs['B'].stroke(3, 6);
	glyphs['B'].stroke(6, 4);
	glyphs['B'].stroke(4, 7);
	glyphs['B'].stroke(7, 5);

	glyphs['C'].vertex({ 1.0f, 0.25f });
	glyphs['C'].vertex({ 0.5f, 0.0f });
	glyphs['C'].vertex({ 0.0f, 0.25f });
	glyphs['C'].vertex({ 0.0f, 0.75f });
	glyphs['C'].vertex({ 0.5f, 1.0f });
	glyphs['C'].vertex({ 1.0f, 0.75f });
	glyphs['C'].stroke(0, 1);
	glyphs['C'].stroke(1, 2);
	glyphs['C'].stroke(2, 3);
	glyphs['C'].stroke(3, 4);
	glyphs['C'].stroke(4, 5);

	glyphs['D'].vertex({ 0.0f, 0.0f });
	glyphs['D'].vertex({ 0.0f, 1.0f });
	glyphs['D'].vertex({ 0.5f, 0.0f });
	glyphs['D'].vertex({ 0.5f, 1.0f });
	glyphs['D'].vertex({ 1.0f, 0.25f });
	glyphs['D'].vertex({ 1.0f, 0.75f });
	glyphs['D'].stroke(0, 1);
	glyphs['D'].stroke(0, 2, 0.6f, { 1,2 }, 1.0f);
	glyphs['D'].stroke(1, 3, 0.6f, { -1,2 }, -1.0f);
	glyphs['D'].stroke(2, 4);
	glyphs['D'].stroke(3, 5);
	glyphs['D'].stroke(4, 5);

	glyphs['E'].vertex({ 0.0f, 0.0f });
	glyphs['E'].vertex({ 0.0f, 0.5f });
	glyphs['E'].vertex({ 0.0f, 1.0f });
	glyphs['E'].vertex({ 1.0f, 0.0f });
	glyphs['E'].vertex({ 0.7f, 0.5f });
	glyphs['E'].vertex({ 1.0f, 1.0f });
	glyphs['E'].stroke(0, 1);
	glyphs['E'].stroke(1, 2);
	glyphs['E'].stroke(0, 3, 1.0f, { 0,1 }, 1.0f);
	glyphs['E'].stroke(1, 4, 1.0f, { 0,1 });
	glyphs['E'].stroke(2, 5, 1.0f, { 0,1 }, -1.0f);

	glyphs['F'].vertex({ 0.0f, 0.0f });
	glyphs['F'].vertex({ 0.0f, 0.5f });
	glyphs['F'].vertex({ 0.0f, 1.0f });
	glyphs['F'].vertex({ 1.0f, 0.0f });
	glyphs['F'].vertex({ 0.7f, 0.5f });
	glyphs['F'].stroke(0,1);
	glyphs['F'].stroke(1,2);
	glyphs['F'].stroke(0,3, 1.0f, {0,1}, 1.0f);
	glyphs['F'].stroke(1,4, 1.0f, {0,1});

	glyphs['G'].vertex({ 1.0f, 0.25f });
	glyphs['G'].vertex({ 0.5f, 0.0f });
	glyphs['G'].vertex({ 0.0f, 0.25f });
	glyphs['G'].vertex({ 0.0f, 0.75f });
	glyphs['G'].vertex({ 0.5f, 1.0f });
	glyphs['G'].vertex({ 1.0f, 0.75f });
	glyphs['G'].vertex({ 1.0f, 0.5f });
	glyphs['G'].vertex({ 0.5f, 0.5f });
	glyphs['G'].stroke(0, 1);
	glyphs['G'].stroke(1, 2);
	glyphs['G'].stroke(2, 3);
	glyphs['G'].stroke(3, 4);
	glyphs['G'].stroke(4, 5);
	glyphs['G'].stroke(5, 6);
	// adjustment is used to hide corner, putting the bar a little low
	glyphs['G'].stroke(6, 7, 1.0f, {0,1}, 1.0f);

	glyphs['H'].vertex({ 0.0f, 0.0f });
	glyphs['H'].vertex({ 0.0f, 1.0f });
	glyphs['H'].vertex({ 1.0f, 0.0f });
	glyphs['H'].vertex({ 1.0f, 1.0f });
	glyphs['H'].vertex({ 0.0f, 0.5f });
	glyphs['H'].vertex({ 1.0f, 0.5f });
	glyphs['H'].stroke(0, 1);
	glyphs['H'].stroke(2, 3);
	glyphs['H'].stroke(4, 5, 1.0f, {0,1});

	glyphs['I'].vertex({ 0.5f, 0.0f });
	glyphs['I'].vertex({ 0.5f, 1.0f });
	glyphs['I'].stroke(0, 1);

	glyphs['J'].vertex({ 1.0f, 0.0f });
	glyphs['J'].vertex({ 1.0f, 0.75f });
	glyphs['J'].vertex({ 0.5f, 1.0f });
	glyphs['J'].vertex({ 0.0f, 0.75f });
	glyphs['J'].stroke(0, 1);
	glyphs['J'].stroke(1, 2);
	glyphs['J'].stroke(2, 3);
	
	glyphs['K'].vertex({ 0.0f, 0.0f });
	glyphs['K'].vertex({ 0.0f, 1.0f });
	glyphs['K'].vertex({ 1.0f, 0.0f });
	glyphs['K'].vertex({ 0.0f, 0.5f });
	glyphs['K'].vertex({ 1.0f, 1.0f });
	glyphs['K'].stroke(0, 1);
	// TODO both ends would ideally have different adjustment
	glyphs['K'].stroke(2, 3, 1.0f, {1,0}, 1.0f);
	glyphs['K'].stroke(3, 4, 1.0f, {1,0}, 1.0f);

	glyphs['L'].vertex({ 0.0f, 0.0f });
	glyphs['L'].vertex({ 0.0f, 1.0f });
	glyphs['L'].vertex({ 1.0f, 1.0f });
	glyphs['L'].stroke(0, 1);
	glyphs['L'].stroke(1, 2, 1.0f, {0,1}, -1.0f);

	glyphs['M'].vertex({ 0.0f, 1.0f });
	glyphs['M'].vertex({ 0.0f, 0.0f });
	glyphs['M'].vertex({ 0.5f, 0.5f });
	glyphs['M'].vertex({ 1.0f, 0.0f });
	glyphs['M'].vertex({ 1.0f, 1.0f });
	glyphs['M'].stroke(0, 1);
	glyphs['M'].stroke(1, 2);
	glyphs['M'].stroke(2, 3);
	glyphs['M'].stroke(3, 4);

	glyphs['N'].vertex({ 0.0f, 0.0f });
	glyphs['N'].vertex({ 0.0f, 1.0f });
	glyphs['N'].vertex({ 1.0f, 0.0f });
	glyphs['N'].vertex({ 1.0f, 1.0f });
	glyphs['N'].stroke(0, 1);
	glyphs['N'].stroke(2, 3);
	glyphs['N'].stroke(0, 3);

	glyphs['O'].vertex({ 1.0f, 0.25f });
	glyphs['O'].vertex({ 0.5f, 0.0f });
	glyphs['O'].vertex({ 0.0f, 0.25f });
	glyphs['O'].vertex({ 0.0f, 0.75f });
	glyphs['O'].vertex({ 0.5f, 1.0f });
	glyphs['O'].vertex({ 1.0f, 0.75f });
	glyphs['O'].stroke(0, 1);
	glyphs['O'].stroke(1, 2);
	glyphs['O'].stroke(2, 3);
	glyphs['O'].stroke(3, 4);
	glyphs['O'].stroke(4, 5);
	glyphs['O'].stroke(5, 0);

	glyphs['P'].vertex({ 0.0f, 1.0f });
	glyphs['P'].vertex({ 0.0f, 0.0f });
	glyphs['P'].vertex({ 0.5f, 0.0f });
	glyphs['P'].vertex({ 1.0f, 0.25f });
	glyphs['P'].vertex({ 0.5f, 0.5f });
	glyphs['P'].vertex({ 0.0f, 0.5f });
	glyphs['P'].stroke(0, 1);
	glyphs['P'].stroke(1, 2, 0.6f, {1,2}, 1.0f);
	glyphs['P'].stroke(2, 3);
	glyphs['P'].stroke(3, 4);
	glyphs['P'].stroke(4, 5, 0.6f, {-1,2}, -1.0f);

	glyphs['Q'].vertex({ 1.0f, 0.25f });
	glyphs['Q'].vertex({ 0.5f, 0.0f });
	glyphs['Q'].vertex({ 0.0f, 0.25f });
	glyphs['Q'].vertex({ 0.0f, 0.75f });
	glyphs['Q'].vertex({ 0.5f, 1.0f });
	glyphs['Q'].vertex({ 1.0f, 0.75f });
	glyphs['Q'].vertex({ 0.5f, 0.75f });
	glyphs['Q'].vertex({ 1.0f, 1.0f });
	glyphs['Q'].stroke(0, 1);
	glyphs['Q'].stroke(1, 2);
	glyphs['Q'].stroke(2, 3);
	glyphs['Q'].stroke(3, 4);
	glyphs['Q'].stroke(4, 5);
	glyphs['Q'].stroke(5, 0);
	glyphs['Q'].stroke(6, 7);

	glyphs['R'].vertex({ 0.0f, 1.0f });
	glyphs['R'].vertex({ 0.0f, 0.0f });
	glyphs['R'].vertex({ 0.5f, 0.0f });
	glyphs['R'].vertex({ 1.0f, 0.25f });
	glyphs['R'].vertex({ 0.5f, 0.5f });
	glyphs['R'].vertex({ 0.0f, 0.5f });
	glyphs['R'].vertex({ 1.0f, 1.0f });
	glyphs['R'].stroke(0, 1);
	glyphs['R'].stroke(1, 2, 0.6f, { 1,2 }, 1.0f);
	glyphs['R'].stroke(2, 3);
	glyphs['R'].stroke(3, 4);
	glyphs['R'].stroke(4, 5, 0.6f, { -1,2 }, -1.0f);
	glyphs['R'].stroke(4, 6);

	glyphs['S'].vertex({ 1.0f, 0.25f });
	glyphs['S'].vertex({ 0.5f, 0.0f });
	glyphs['S'].vertex({ 0.0f, 0.25f });
	glyphs['S'].vertex({ 1.0f, 0.75f });
	glyphs['S'].vertex({ 0.5f, 1.0f });
	glyphs['S'].vertex({ 0.0f, 0.75f });
	glyphs['S'].stroke(0, 1);
	glyphs['S'].stroke(1, 2);
	glyphs['S'].stroke(2, 3);
	glyphs['S'].stroke(3, 4);
	glyphs['S'].stroke(4, 5);

	glyphs['T'].vertex({ 0.5f, 0.0f });
	glyphs['T'].vertex({ 0.5f, 1.0f });
	glyphs['T'].vertex({ 0.0f, 0.0f });
	glyphs['T'].vertex({ 1.0f, 0.0f });
	glyphs['T'].stroke(0, 1);
	glyphs['T'].stroke(2, 3, 1.0f, {0,1}, 1.0f);

	glyphs['U'].vertex({ 0.0f, 0.0f });
	glyphs['U'].vertex({ 0.0f, 0.75f });
	glyphs['U'].vertex({ 0.5f, 1.0f });
	glyphs['U'].vertex({ 1.0f, 0.75f });
	glyphs['U'].vertex({ 1.0f, 0.0f });
	glyphs['U'].stroke(0, 1);
	glyphs['U'].stroke(1, 2);
	glyphs['U'].stroke(2, 3);
	glyphs['U'].stroke(3, 4);

	glyphs['V'].vertex({ 0.0f, 0.0f });
	glyphs['V'].vertex({ 0.5f, 1.0f });
	glyphs['V'].vertex({ 1.0f, 0.0f });
	glyphs['V'].stroke(0, 1);
	glyphs['V'].stroke(1, 2);

	glyphs['W'].vertex({ 0.0f, 0.0f });
	glyphs['W'].vertex({ 0.0f, 1.0f });
	glyphs['W'].vertex({ 0.5f, 0.5f });
	glyphs['W'].vertex({ 1.0f, 1.0f });
	glyphs['W'].vertex({ 1.0f, 0.0f });
	glyphs['W'].stroke(0, 1);
	glyphs['W'].stroke(1, 2);
	glyphs['W'].stroke(2, 3);
	glyphs['W'].stroke(3, 4);

	glyphs['X'].vertex({ 0.0f, 0.0f });
	glyphs['X'].vertex({ 0.0f, 1.0f });
	glyphs['X'].vertex({ 1.0f, 0.0f });
	glyphs['X'].vertex({ 1.0f, 1.0f });
	glyphs['X'].stroke(0, 3);
	glyphs['X'].stroke(1, 2);

	glyphs['Y'].vertex({ 0.5f, 0.5f });
	glyphs['Y'].vertex({ 0.5f, 1.0f });
	glyphs['Y'].vertex({ 0.0f, 0.0f });
	glyphs['Y'].vertex({ 1.0f, 0.0f });
	glyphs['Y'].stroke(0, 1);
	glyphs['Y'].stroke(0, 2);
	glyphs['Y'].stroke(0, 3);

	glyphs['Z'].vertex({ 0.0f, 0.0f });
	glyphs['Z'].vertex({ 1.0f, 0.0f });
	glyphs['Z'].vertex({ 0.0f, 1.0f });
	glyphs['Z'].vertex({ 1.0f, 1.0f });
	glyphs['Z'].stroke(0, 1, 1.0f, {0,1}, 1.0f);
	glyphs['Z'].stroke(1, 2);
	glyphs['Z'].stroke(2, 3, 1.0f, {0,1}, -1.0f);

	ursa::set_framefunc([&](float deltaTime) {
		ursa::clear({ 0.50f, 0.5f, 0.5f, 1.0f });

		float thickness = 5.0f;
		const char *foo = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		float x = 50;
		float y = 50;
		for (int i = 0; i < strlen(foo); i++) {
			char ch = foo[i];
			if (glyphs.find(ch) != glyphs.end()) {
				render_glyph(&glyphs[ch], { x,y, 30,100 }, thickness, true, false);
				render_glyph(&glyphs[ch], { x,y+300, 30,100 }, thickness, false, true);
			}
			x += 50;
			if (x > 650) {
				x = 50;
				y += 150;
			}
		}
		x = 50; y = 640;
		for (int i = 0; i < strlen(foo); i++) {
			char ch = foo[i];
			if (glyphs.find(ch) != glyphs.end()) {
				render_glyph(&glyphs[ch], { x, y, 12,50 }, thickness / 2.5f, false, true);
			}
			x += 20;
		}
		x = 50; y = 720;
		for (int i = 0; i < strlen(foo); i++) {
			char ch = foo[i];
			if (glyphs.find(ch) != glyphs.end()) {
				render_glyph(&glyphs[ch], { x, y, 10,18 }, thickness / 3.5f, false, true);
			}
			x += 14;
		}
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
