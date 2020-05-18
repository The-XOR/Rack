#include <app/colorBrowser.hpp>
#include <widget/OpenGlWidget.hpp>
#include <widget/OpaqueWidget.hpp>
#include <widget/TransparentWidget.hpp>
#include <settings.hpp>
#include <app/Scene.hpp>

#include <app.hpp>

#define NUM_test_ROW 8
#define NUM_test_COL 2
#define SQR_SIZE 23
namespace rack {
namespace app {

struct colorBrowser;
struct colorBrowserDisplay : widget::OpenGlWidget
{
	colorBrowserDisplay()
	{
		pmodule = NULL;
	}
	void drawFramebuffer() override;
	void onButton(const event::Button& e) override;

	void SetModule(colorBrowser *module)
	{
		pmodule = module;
	}

private:
	colorBrowser *pmodule;
	const int margin = 1;
};

struct colorBrowser : widget::OpaqueWidget
{
	colorBrowser()
	{
		box.size.x = NUM_test_COL * SQR_SIZE;
		box.size.y = NUM_test_ROW * SQR_SIZE;
		box.size = box.size.ceil();
		colorBrowserDisplay *display = new colorBrowserDisplay;
		display->box.size = box.size;
		display->SetModule(this);
		addChild(display);
	}
	
	void step() override {
		box = parent->box.zeroPos();
		// Only step if visible, since there are potentially thousands of descendants that don't need to be stepped.
		if (visible)
			OpaqueWidget::step();
	}

	void onButton(const event::Button& e) override {
		OpaqueWidget::onButton(e);
		if (e.getTarget() != this)
			return;

		/*if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			hide();
			e.consume(this);
		}*/
	}

	const NVGcolor getCellColor(int r, int c)
	{
		int n = r + c * NUM_test_ROW;
		if(n < (int)settings::cableColors.size())
			return settings::cableColors[n];
		return rack::color::BLACK_TRANSPARENT;
	}

	void click(int r, int c)
	{
		int n = r + c * NUM_test_ROW;
		if(n < (int)settings::cableColors.size())
		{
			APP->scene->rack->nextCableColorId = n;
			INFO("color %i",n);
			APP->scene->colorBrowser->hide();
		}
	}

};

struct  colorOverlay : widget::OpaqueWidget {
	void step() override {
		box = parent->box.zeroPos();
		// Only step if visible, since there are potentially thousands of descendants that don't need to be stepped.
		if (visible)
			OpaqueWidget::step();
	}

	void onButton(const event::Button& e) override {
		OpaqueWidget::onButton(e);
		if (e.getTarget() != this)
			return;

		/*if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			hide();
			e.consume(this);
		}*/
	}
};

widget::Widget* colorBrowserCreate()
{
	colorOverlay* overlay = new colorOverlay;
	colorBrowser* browser = new colorBrowser;
	overlay->addChild(browser);
	return overlay;
}

void colorBrowserDisplay::drawFramebuffer() 
	{
		if(pmodule == NULL)
			return;
		float square_dim = fbSize.x / NUM_test_COL - margin;

		glViewport(0.0, 0.0, fbSize.x, fbSize.y);
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, fbSize.x, 0.0, fbSize.y, -1.0, 1.0);

		float posY = fbSize.y - square_dim - margin;
		for(int r = 0; r < NUM_test_ROW; r++)
		{
			float posX = 0;
			for(int c = 0; c < NUM_test_COL; c++)
			{
				glBegin(GL_QUADS);
				NVGcolor dg = pmodule->getCellColor(r,c);
				glColor3f(dg.r, dg.g, dg.b); //colore lancetta

				glVertex3f(posX, posY, 0); // top left
				glVertex3f(posX + square_dim, posY, 0); // top right 
				glVertex3f(posX + square_dim, posY + square_dim, 0); // bottom right
				glVertex3f(posX, posY + square_dim, 0); // bottom left
				glEnd();
				posX += square_dim + margin;
			}

			posY -= square_dim + margin;
		}
	}

	void colorBrowserDisplay::onButton(const event::Button& e)
	{
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == 0) 
		{
			int c = floor(e.pos.x / SQR_SIZE);
			int r = floor(e.pos.y /  SQR_SIZE);
			pmodule->click(r,c);	
			e.consume(this);
		}
	}
	} // namespace app
} // namespace rack
