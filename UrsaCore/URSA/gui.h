#pragma once

#include <string>
#include <variant>

#include <glm/glm.hpp>

// TODO in the future, c++17 or so will allow namespace ursa::gui {}
namespace ursa { namespace gui {

	struct Style {
		struct SolidColor {
			glm::vec4 color;
		};
		struct Textured {
			glm::vec4 color;
			TextureHandle texture;
			// TODO tiling/stretching
		};
		struct Ninepatch {
			glm::vec4 color;
			TextureHandle texture;
			float margin;
		};
		struct Transparent {};
		struct Custom {
			std::function<void(void)> render;
		};

		std::variant<SolidColor, Textured, Ninepatch, Transparent, Custom> background;

		struct Text {
			glm::vec4 color;
			// TODO font, text alignment, wordwrap, clipping
		} text;

		// TODO how to handle focus, pressed, hover states? distinct styles for each?
	};

	// TODO think about widget state to support animated widgets

	void set_default_font(FontAtlas::object_ref atlas, int fontIndex);
	void set_style(std::string name, const Style &style);

	void handle_mousedown(glm::vec2 pos);
	void handle_mouseup(glm::vec2 pos);
	void handle_mousemove(glm::vec2 pos);
	void handle_mousewheel(glm::vec2 pos);
	void handle_keydown();

	// ...

	bool clicked();
	bool mouseover();

	// ...

	// TODO specify rendertarget, viewport, whatever
	void frame_begin();
	void frame_end();

	// TODO name this thing overlay_begin instead?
	void dialog_begin();
	void dialog_end();

	// panels are containers attached to one of the four edges
	enum class PanelEdge { left, top, right, bottom };

	void panel_begin(PanelEdge edge, float size_px, std::string styleName="");
	void panel_begin_by_percent(PanelEdge edge, float size_percent, std::string styleName="");
	void panel_end();

	void padding(float px);
	void space(float px);

	void background(const glm::vec4 &color);
	void border(const glm::vec4 &color, float width);

	bool button(std::string str);
	void text(std::string str);
	void input();
	void checkbox(std::string label, bool *var);
	void radiobutton(std::string label, int value, int *var);
	void progress();
	void slider();
	void listbox();
	void combobox();

	void table_begin();
	void table_end();
	void tablerow_begin();
	void tablerow_end();
	void tablecell_begin();
	void tablecell_end();

	void list_begin();
	void list_end();

	void tree_begin();
	void tree_end();
}}
