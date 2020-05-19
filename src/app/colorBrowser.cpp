#include <app/colorBrowser.hpp>
#include <widget/OpaqueWidget.hpp>
#include <widget/TransparentWidget.hpp>
#include <settings.hpp>
#include <app/Scene.hpp>
#include <app.hpp>

#define SQR_SIZE 30.f
namespace rack {
namespace app {

struct colorOverlay : widget::OpaqueWidget
{
	colorOverlay()
	{
	}
			
	void step() override
	{
		if(visible)
		{
			box = parent->box.zeroPos();
			OpaqueWidget::step();
		}
	}
			
	void backgr(NVGcontext *vg)
	{
		nvgBeginPath(vg);
		nvgRect(vg, box.pos.x, box.pos.y, box.size.x, box.size.y);
		nvgStrokeColor(vg, nvgRGBf(1.f, 1.f, 1.f));
		nvgFillColor(vg, nvgRGBf(0.2f, 0.2f, 0.2f));
		nvgStrokeWidth(vg, 1);
		nvgFill(vg);
		nvgStroke(vg);
	}

	void draw(const DrawArgs &args) override
	{
		float HSQR = SQR_SIZE / 2.f;

		backgr(args.vg);
		int x = box.pos.x+1;
		int y = box.pos.y;
		for(int n = 0; n < (int)settings::cableColors.size(); n++)
		{
			NVGcolor color = settings::cableColors[n];
			NVGcolor colorOutline = nvgLerpRGBA(color, nvgRGBf(0.0, 0.0, 0.0), 0.5);

			// Plug solid
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, x + HSQR, y + HSQR, HSQR);
			nvgFillColor(args.vg, color);
			nvgFill(args.vg);

			// Border
			nvgStrokeWidth(args.vg, 1.0);
			nvgStrokeColor(args.vg, colorOutline);
			nvgStroke(args.vg);

			y += 1 + SQR_SIZE;
		}
	}

	void click(int n)
	{
		if(n < (int)settings::cableColors.size())
		{
			APP->scene->rack->nextCableColorId = n;
			APP->scene->colorBrowser->hide();
		}
	}

	void onButton(const event::Button &e) override
	{
		OpaqueWidget::onButton(e);
		if(e.getTarget() != this)
			return;
		if(e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == 0)
		{
			int n = floor(e.pos.y / SQR_SIZE);
			click(n);
			e.consume(this);
		}
	}
};

struct colorOverlayCtr : widget::OpaqueWidget 
{
	void step() override 
	{
		// Only step if visible, since there are potentially thousands of descendants that don't need to be stepped.
		if(visible)
		{
			box = parent->box.zeroPos().grow(math::Vec(-70, -70));
			box.size.x = SQR_SIZE + 2;
			box.size.y = (int)settings::cableColors.size() * (1 + SQR_SIZE);
			box.size = box.size.ceil();

			OpaqueWidget::step();
		}
	}
};

widget::Widget *colorBrowserCreate()
{
	colorOverlayCtr *container = new colorOverlayCtr;
	colorOverlay *browser = new colorOverlay;
	container->addChild(browser);
	return container;
}

} // namespace app
} // namespace rack

