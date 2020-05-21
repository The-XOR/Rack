#include <app/colorBrowser.hpp>
#include <widget/OpaqueWidget.hpp>
#include <widget/TransparentWidget.hpp>
#include <asset.hpp>
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
		font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
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
		float HSQR = SQR_SIZE * 0.5f;
		static const char *shortocutti="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		char txt[2] = {0,0};
		backgr(args.vg);
		int x = box.pos.x+1;
		int y = box.pos.y;
		for(int n = 0; n < (int)settings::cableColors.size(); n++)
		{
			NVGcolor color = settings::cableColors[n];
	
			// Border
			nvgStrokeWidth(args.vg, 1.0);
			nvgStrokeColor(args.vg, nvgLerpRGBA(color, nvgRGBf(0.1, 0.1, 0.1), 0.5));
			nvgStroke(args.vg);
			
			// Plug solid
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, x + HSQR, y + HSQR, HSQR);
			nvgFillColor(args.vg, color);
			nvgFill(args.vg);

			// Hole
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, x + HSQR, y + HSQR, HSQR * 0.5);
			nvgFillColor(args.vg, nvgRGBf(0.0, 0.0, 0.0));
			nvgFill(args.vg);

			if (font->handle >= 0 && n < 36) 
			{
				txt[0] = shortocutti[n];
				nvgFillColor(args.vg, nvgRGBf(1.0, 1.0, 1.0));
				nvgFontFaceId(args.vg, font->handle);
				nvgTextLetterSpacing(args.vg, 0.0);
				nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE|NVG_ALIGN_CENTER);

				nvgFontSize(args.vg, 11);
				nvgText(args.vg,  x + HSQR, y + HSQR +0.5f, txt, NULL);
			}

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

	void onKeyboard(const event::HoverKey& e)
	{
		if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) 
		{
			int n=-1;
			switch (e.key) 
			{
				case GLFW_KEY_ESCAPE:
					APP->scene->colorBrowser->hide();
					e.consume(this);
					break;

				case GLFW_KEY_KP_0:	
				case GLFW_KEY_KP_1:	
				case GLFW_KEY_KP_2:	
				case GLFW_KEY_KP_3:	
				case GLFW_KEY_KP_4:	
				case GLFW_KEY_KP_5:	
				case GLFW_KEY_KP_6:	
				case GLFW_KEY_KP_7:	
				case GLFW_KEY_KP_8:	
				case GLFW_KEY_KP_9:	
					n = e.key - GLFW_KEY_KP_0;
					break;

				case GLFW_KEY_0:
				case GLFW_KEY_1:
				case GLFW_KEY_2:
				case GLFW_KEY_3:
				case GLFW_KEY_4:
				case GLFW_KEY_5:
				case GLFW_KEY_6:
				case GLFW_KEY_7:
				case GLFW_KEY_8:
				case GLFW_KEY_9:
				  	n = e.key - GLFW_KEY_0;
					break;


				case GLFW_KEY_A:
				case GLFW_KEY_B:
				case GLFW_KEY_C:
				case GLFW_KEY_D:
				case GLFW_KEY_E:
				case GLFW_KEY_F:
				case GLFW_KEY_G:
				case GLFW_KEY_H:
				case GLFW_KEY_I:
				case GLFW_KEY_J:
				case GLFW_KEY_K:
				case GLFW_KEY_L:
				case GLFW_KEY_M:
				case GLFW_KEY_N:
				case GLFW_KEY_O:
				case GLFW_KEY_P:
				case GLFW_KEY_Q:
				case GLFW_KEY_R:
				case GLFW_KEY_S:
				case GLFW_KEY_T:
				case GLFW_KEY_U:
				case GLFW_KEY_V:
				case GLFW_KEY_W:
				case GLFW_KEY_X:
				case GLFW_KEY_Y:
				case GLFW_KEY_Z:
					n = 10 + e.key - GLFW_KEY_A;
					break;
			}
		
			if(n >=0 && n < (int)settings::cableColors.size())
			{
				click(n);
				e.consume(this);
			}
		}
	}


	void onHoverKey(const event::HoverKey& e) override
	{
		onKeyboard(e);
	}

private:
	std::shared_ptr<Font> font;

};

struct colorOverlayCtr : widget::OpaqueWidget 
{
	colorOverlay *browser;
	colorOverlayCtr()
	{
		browser = new colorOverlay;
		addChild(browser);
	}

	void onHoverKey(const event::HoverKey& e) override
	{
		browser->onKeyboard(e);
	}

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
	return new colorOverlayCtr;
}

} // namespace app
} // namespace rack

