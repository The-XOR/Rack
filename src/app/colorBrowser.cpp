#include <app/colorBrowser.hpp>
#include <widget/OpaqueWidget.hpp>
#include <widget/TransparentWidget.hpp>
#include <settings.hpp>
#include <app/Scene.hpp>
#include <app.hpp>

#define SQR_SIZE 14
namespace rack {
namespace app {

struct colorOverlay : widget::OpaqueWidget
{
	colorOverlay()
	{
		box = parent->box.zeroPos().grow(math::Vec(-70, -70));
		box.size.x = SQR_SIZE;
		box.size.y = (int)settings::cableColors.size() * SQR_SIZE;
//		box.size = box.size.ceil();
	}

	void step() override
	{
		if (visible)
			OpaqueWidget::step();
	}

	void draw(const DrawArgs &args) override
	{
		int x = box.pos.x;
		int y = box.pos.y;
		for (int n = 0; n < (int)settings::cableColors.size(); n++)
		{
			NVGcolor color = settings::cableColors[n];
			NVGcolor colorOutline = nvgLerpRGBA(color, nvgRGBf(0.0, 0.0, 0.0), 0.5);

			// Plug solid
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, x, y, 40*SQR_SIZE-2);
			nvgFillColor(args.vg, color);
			nvgFill(args.vg);

			// Border
			nvgStrokeWidth(args.vg, 1.0);
			nvgStrokeColor(args.vg, colorOutline);
			nvgStroke(args.vg);

			// Hole
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, x, y, SQR_SIZE - 7);
			nvgFillColor(args.vg, nvgRGBf(0.0, 0.0, 0.0));
			nvgFill(args.vg);
//
			nvgBeginPath(args.vg);
			nvgRect(args.vg, box.pos.x, box.pos.y, box.size.x, box.size.y);
			nvgFillColor(args.vg, nvgRGBf(1.0, 0.0, 0.0));
			nvgFill(args.vg);
//
			y += 2 * SQR_SIZE;
		}
	}
	
	void click(int n)
	{
		if (n < (int)settings::cableColors.size())
		{
			APP->scene->rack->nextCableColorId = n;
			INFO("color %i", n);
			APP->scene->colorBrowser->hide();
		}
	}

	void onButton(const event::Button &e) override
	{
		INFO("@@ x=%i y=%i", e.pos.x, e.pos.y);
		OpaqueWidget::onButton(e);
		if (e.getTarget() != this)
			return;
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == 0)
		{
			INFO("x=%i y=%i", e.pos.x, e.pos.y);
			int n = floor(e.pos.y / SQR_SIZE);
			click(n);
			e.consume(this);
		}
	}
};

widget::Widget *colorBrowserCreate()
{
	return new colorOverlay;
}


} // namespace app
} // namespace rack

