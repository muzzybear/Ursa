#include "URSA.h"
#include "gui.h"

#include <stack>
#include <map>

namespace ursa { namespace gui {

	struct State {
		// sliders, dropdown lists, etc require the widget to be active
		// TODO should activewidget be a stack, to support silly things like sliders inside dropdown menus?
		int activeWidget{ -1 };
		std::stack<Rect> viewportStack;
		std::stack<std::string> styleStack;
		std::map<std::string, Style> styles;
		glm::vec2 mousePosition{ 0,0 };
		bool LMB{ false };
		bool clicked{ false };
		FontAtlas::object_ref atlas;
		int defaultFontIndex = -1;

		State() {
			styles["default"] = {
				Style::SolidColor{ {0,0,0, 0.2f} },
				{ {1,1,1,1} },
			};
		}
	};

	static State state;
	static int nextId = 0;

	void set_default_font(FontAtlas::object_ref atlas, int fontIndex) {
		state.atlas = atlas;
		state.defaultFontIndex = fontIndex;
	}

	void set_style(std::string name, const Style &style) {
		state.styles[name] = style;
	}

	// ...

	void handle_mousedown(glm::vec2 pos) {
		// TODO right mouse button, doubleclick
		if (!state.LMB)
			state.clicked = true;
		state.LMB = true;
		state.mousePosition = pos;
	}
	void handle_mouseup(glm::vec2 pos) {
		state.LMB = false;
	}

	void handle_mousemove(glm::vec2 pos) {
		// TODO support sliders and drag&drop
	}

	// ...

	void draw_background() {
		// TODO get best matching style
		// for now, will only match against top of the stack, later support dialog.button and maybe dialog>button etc...
		const Style &style = state.styles["default"];

		if (const auto bg = std::get_if<Style::SolidColor>(&style.background)) {
			background(bg->color);
		} else if (const auto bg = std::get_if<Style::Textured>(&style.background)) {
			// TODO draw texture
			background(bg->color);
		}

	}

	void draw_string(std::string str) {
		// TODO color
		// TODO alignment, i.e. center button texts
		Rect r = state.viewportStack.top();
		draw_text(state.atlas, state.defaultFontIndex, r.pos.x, r.pos.y, str.c_str());
	}

	// ...

	void frame_begin()
	{
		nextId = 0;
		assert(state.viewportStack.empty());
		assert(state.styleStack.empty());
		state.viewportStack.push(screenrect());
	}

	void frame_end() {
		state.viewportStack.pop();
		assert(state.styleStack.empty());
		assert(state.viewportStack.empty());

		// clicked state only persist until end of the frame
		state.clicked = false;
	}

	// TODO ??? x&y size (in % or px), and x&y position (in % or px)
	void dialog_begin() {}
	void dialog_end() {}

	// TODO RAII guard for panel_begin + panel_end ?

	// slice away a portion of the current viewport for the child widget
	void panel_begin(PanelEdge edge, float size_px, std::string styleName) {
		Rect current = state.viewportStack.top();
		state.viewportStack.pop();

		Rect parent, child;

		switch (edge) {
			case PanelEdge::left:
				std::tie(child,parent) = current.splitX(size_px);
				break;
			case PanelEdge::right:
				std::tie(parent, child) = current.splitX(current.size.x - size_px);
				break;
			case PanelEdge::top:
				std::tie(child, parent) = current.splitY(size_px);
				break;
			case PanelEdge::bottom:
				std::tie(parent, child) = current.splitY(current.size.y - size_px);
				break;
		}

		state.viewportStack.push(parent);
		state.viewportStack.push(child);
		state.styleStack.push(styleName);
	}

	void panel_begin_by_percent(PanelEdge edge, float size_percent, std::string styleName) {
		switch (edge) {
			case PanelEdge::left:
			case PanelEdge::right:
				panel_begin(edge, state.viewportStack.top().size.x * size_percent, styleName);
				break;
			case PanelEdge::top:
			case PanelEdge::bottom:
				panel_begin(edge,state.viewportStack.top().size.y * size_percent, styleName);
				break;
		}
	}

	void panel_end() {
		state.styleStack.pop();
		state.viewportStack.pop();
	}

	void padding(float px) {
		Rect inner = state.viewportStack.top().expand(-px);
		state.viewportStack.pop();
		state.viewportStack.push(inner);
	}

	void background(const glm::vec4 &color) {
		draw_quad(state.viewportStack.top(), color);
	}

	bool mouseover() { return state.viewportStack.top().contains(state.mousePosition); }

	bool clicked() { return state.clicked && mouseover(); }

	bool button(std::string str) {
		int id = nextId++;
		panel_begin(PanelEdge::top, 32, "button");

		bool pressed = false;
		if (clicked())
		{
			// TODO anything else? set focus or something?
			pressed = true;
		}

		draw_background();
		draw_string(str);
		panel_end();

		return pressed;
	}

	void text(std::string str) {
		panel_begin(PanelEdge::top, 32, "text");
		draw_string(str);
		panel_end();
	}

	void input() {}
	void checkbox(std::string label, bool *var) {
		int id = nextId++;
		panel_begin(PanelEdge::top, 32, "checkbox");

		if (clicked())
			*var = !*var;

		draw_background();
		panel_begin(PanelEdge::left, 32, *var ? "checkbox_true" : "checkbox_false");
		draw_background();
		panel_end();
		draw_string(label);
		panel_end();
	}
	void radiobutton(std::string label, bool *var) {}
	void progress() {}
	void slider() {}
	void listbox() {}
	void combobox() {}

}
}
