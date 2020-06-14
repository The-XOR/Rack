#include <app/ModuleBrowser.hpp>
#include <widget/OpaqueWidget.hpp>
#include <widget/TransparentWidget.hpp>
#include <widget/ZoomWidget.hpp>
#include <ui/ScrollWidget.hpp>
#include <ui/SequentialLayout.hpp>
#include <ui/MarginLayout.hpp>
#include <ui/Label.hpp>
#include <ui/TextField.hpp>
#include <ui/MenuOverlay.hpp>
#include <ui/List.hpp>
#include <ui/MenuItem.hpp>
#include <ui/Button.hpp>
#include <ui/RadioButton.hpp>
#include <ui/ChoiceButton.hpp>
#include <ui/Tooltip.hpp>
#include <app/ModuleWidget.hpp>
#include <app/Scene.hpp>
#include <plugin.hpp>
#include <app.hpp>
#include <plugin/Model.hpp>
#include <string.hpp>
#include <history.hpp>
#include <settings.hpp>
#include <tag.hpp>
#include <asset.hpp>
#include <osdialog.h>

#include <set>
#include <algorithm>


namespace rack {
namespace app {


// Static functions

struct Favourites
{
	bool showFavorites;
	void Init(std::vector<plugin::Plugin*> &plugins)
	{
		showFavorites = false;
		for (plugin::Plugin* plugin : plugin::plugins) 
			for (plugin::Model* model : plugin->models) 
			{
				favorites[mkFavoriteName(model)] = false;
				permanentlyHidden[mkFavoriteName(model)] = false;
			}

		load();
		loadHiddens();
	}

	void toggle(rack::plugin::Model *model)
	{
		std::string nm = mkFavoriteName(model);
		favorites[nm] = !favorites[nm];
	}

	bool hide(rack::plugin::Model *model)
	{
		if(osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK_CANCEL, "Permanently hide this module?"))
		{
			permanentlyHidden[mkFavoriteName(model)] = true;
			saveHiddens();
			return true;
		}
		return false;
	}

	bool isHidden(rack::plugin::Model *model)
	{
		return permanentlyHidden[mkFavoriteName(model)];
	}

	bool isFavorite(rack::plugin::Model *model)
	{
		return favorites[mkFavoriteName(model)];
	}

	void load()
	{
		std::string favfile = rack::asset::userDir + "/favorites-v" + app::ABI_VERSION + ".json";
		FILE* file = fopen(favfile.c_str(), "r");
		if (!file)
			return;
		DEFER({
			fclose(file);
		});

		json_error_t error;
		json_t* rootJ = json_loadf(file, 0, &error);
		if (!rootJ)
			throw UserException(string::f("Favoutites file has invalid JSON at %d:%d %s", error.line, error.column, error.text));

		for (auto it = favorites.begin(); it != favorites.end(); it++ )
		{
			json_t* vj = json_object_get(rootJ, it->first.c_str());
			if (vj)
				favorites[it->first] = json_integer_value(vj) > 0;
		}
		json_decref(rootJ);
	}

	void loadHiddens()
	{
		std::string hidfile = rack::asset::userDir + "/hiddens-v" + app::ABI_VERSION + ".json";
		FILE* file = fopen(hidfile.c_str(), "r");
		if (!file)
			return;
		DEFER({
			fclose(file);
		});

		json_error_t error;
		json_t* rootJ = json_loadf(file, 0, &error);
		if (!rootJ)
			throw UserException(string::f("Hidden file has invalid JSON at %d:%d %s", error.line, error.column, error.text));

		for (auto it = favorites.begin(); it != favorites.end(); it++ )
		{
			json_t* vj = json_object_get(rootJ, it->first.c_str());
			if (vj)
				permanentlyHidden[it->first] = json_integer_value(vj) > 0;
		}
		json_decref(rootJ);
	}

	void saveHiddens()
	{
		std::string hidfile = rack::asset::userDir + "/hiddens-v" + app::ABI_VERSION + ".json";
		json_t* rootJ = json_object();
		for (auto it = permanentlyHidden.begin(); it != permanentlyHidden.end(); it++ )
		{
			json_object_set_new(rootJ, it->first.c_str(), json_integer(it->second ? 1 : 0));
		}

		FILE* file = fopen(hidfile.c_str(), "w");
		if (!file)
			return;
		DEFER({
			fclose(file);
		});

		json_dumpf(rootJ, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
		json_decref(rootJ);
	}

	void save()
	{
		std::string favfile = rack::asset::userDir + "/favorites-v" + app::ABI_VERSION + ".json";
		json_t* rootJ = json_object();
		for (auto it = favorites.begin(); it != favorites.end(); it++ )
		{
			json_object_set_new(rootJ, it->first.c_str(), json_integer(it->second ? 1 : 0));
		}

		FILE* file = fopen(favfile.c_str(), "w");
		if (!file)
			return;
		DEFER({
			fclose(file);
		});

		json_dumpf(rootJ, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
		json_decref(rootJ);
	}

private:
	std::map<std::string, bool> favorites;
	std::map<std::string, bool> permanentlyHidden;
	std::string mkFavoriteName(rack::plugin::Model *model)
	{
		return model->plugin->slug + ":" + model->slug;
	}

};

static float modelScore(plugin::Model* model, const std::string& search) {
	if (search.empty())
		return 1.f;
	std::string s;
	s += model->plugin->brand;
	s += " ";
	s += model->plugin->name;
	s += " ";
	s += model->name;
	s += " ";
	s += model->slug;
	for (int tagId : model->tags) {
		// Add all aliases of a tag
		for (const std::string& alias : tag::tagAliases[tagId]) {
			s += " ";
			s += alias;
		}
	}
	float score = string::fuzzyScore(string::lowercase(s), string::lowercase(search));
	return score;
};

static bool isModelVisible(plugin::Model* model, const std::string& search, const std::string& brand, int tagId, struct Favourites &favor) {
	// Filter search query
	if(favor.isHidden(model))
		return false;

	if (search != "") {
		float score = modelScore(model, search);
		if (score <= 0.f)
			return false;
	}

	// Filter brand
	if (brand != "") {
		if (model->plugin->brand != brand)
			return false;
	}

	// Filter tag
	if (tagId >= 0) {
		auto it = std::find(model->tags.begin(), model->tags.end(), tagId);
		if (it == model->tags.end())
			return false;
	}

	// filter favorites
	if(favor.showFavorites)
		return favor.isFavorite(model);

	return true;
}

static ModuleWidget* chooseModel(plugin::Model* model, bool *hide) {
	// Create module
	ModuleWidget* moduleWidget = model->createModuleWidget();
	assert(moduleWidget);
	APP->scene->rack->addModuleAtMouse(moduleWidget);

	// Push ModuleAdd history action
	history::ModuleAdd* h = new history::ModuleAdd;
	h->name = "create module";
	h->setModule(moduleWidget);
	APP->history->push(h);

	// Hide Module Browser
	*hide = ((APP->window->getMods() & RACK_MOD_MASK) != GLFW_MOD_SHIFT);
	return moduleWidget;
}

template <typename K, typename V>
V get_default(const std::map<K, V>& m, const K& key, const V& def) {
	auto it = m.find(key);
	if (it == m.end())
		return def;
	return it->second;
}


// Widgets

struct ModuleBrowser;
struct BrowserOverlay : widget::OpaqueWidget {
	BrowserOverlay *Create();
	void step() override {
		box = parent->box.zeroPos();
		// Only step if visible, since there are potentially thousands of descendants that don't need to be stepped.
		if (visible)
			OpaqueWidget::step();
	}

	void toggleFavorite(rack::plugin::Model *model);
	void permanentlyHide(rack::plugin::Model *model);

	void onButton(const event::Button& e) override {
		OpaqueWidget::onButton(e);
		if (e.getTarget() != this)
			return;

		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			Hide();
			e.consume(this);
		}
	}

	void Hide();
	
	ModuleBrowser* mybrowser;
};


static const float MODEL_BOX_ZOOM = 0.5f;


struct ModelBox : widget::OpaqueWidget {
	plugin::Model* model;
	widget::Widget* previewWidget;
	ui::Tooltip* tooltip = NULL;
	/** Lazily created */
	widget::FramebufferWidget* previewFb = NULL;

	ModelBox() {
		// Approximate size as 10HP before we know the actual size.
		// We need a nonzero size, otherwise the parent widget will consider it not in the draw bounds, so its preview will not be lazily created.
		box.size.x = 10 * RACK_GRID_WIDTH * MODEL_BOX_ZOOM;
		box.size.y = RACK_GRID_HEIGHT * MODEL_BOX_ZOOM;
		box.size = box.size.ceil();
	}

	void setModel(plugin::Model* model) {
		this->model = model;

		previewWidget = new widget::TransparentWidget;
		previewWidget->box.size.y = std::ceil(RACK_GRID_HEIGHT * MODEL_BOX_ZOOM);
		addChild(previewWidget);
	}

	void createPreview() {
		previewFb = new widget::FramebufferWidget;
		if (math::isNear(APP->window->pixelRatio, 1.0)) {
			// Small details draw poorly at low DPI, so oversample when drawing to the framebuffer
			previewFb->oversample = 2.0;
		}
		previewWidget->addChild(previewFb);

		widget::ZoomWidget* zoomWidget = new widget::ZoomWidget;
		zoomWidget->setZoom(MODEL_BOX_ZOOM);
		previewFb->addChild(zoomWidget);

		ModuleWidget* moduleWidget = model->createModuleWidgetNull();
		zoomWidget->addChild(moduleWidget);

		zoomWidget->box.size.x = moduleWidget->box.size.x * MODEL_BOX_ZOOM;
		zoomWidget->box.size.y = RACK_GRID_HEIGHT * MODEL_BOX_ZOOM;
		previewWidget->box.size.x = std::ceil(zoomWidget->box.size.x);

		box.size.x = previewWidget->box.size.x;
	}

	bool AmIfavourite();
	void draw(const DrawArgs& args) override {
		// Lazily create preview when drawn
		if (!previewFb) {
			createPreview();
		}

		bool fav = AmIfavourite();
		// Draw shadow
		nvgBeginPath(args.vg);
		float r = 10; // Blur radius
		float c = 10; // Corner radius
		nvgRect(args.vg, -r, -r, box.size.x + 2 * r, box.size.y + 2 * r);
		NVGcolor shadowColor = nvgRGBAf(0, fav ? 1 : 0,  0, fav ? 0.9 : 0.5);
		NVGcolor transparentColor = nvgRGBAf(0, 0, 0, 0);
		nvgFillPaint(args.vg, nvgBoxGradient(args.vg, 0, 0, box.size.x, box.size.y, c, r, shadowColor, transparentColor));
		nvgFill(args.vg);

		OpaqueWidget::draw(args);
	}

	void setTooltip(ui::Tooltip* tooltip) {
		if (this->tooltip) {
			this->tooltip->requestDelete();
			this->tooltip = NULL;
		}

		if (tooltip) {
			APP->scene->addChild(tooltip);
			this->tooltip = tooltip;
		}
	}

	void onButton(const event::Button& e) override {
		OpaqueWidget::onButton(e);
		if (e.getTarget() != this)
			return;

		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			if(((APP->window->getMods() & RACK_MOD_MASK) == GLFW_MOD_CONTROL))
			{
				BrowserOverlay* overlay = getAncestorOfType<BrowserOverlay>();
				overlay->toggleFavorite(model);
				e.consume(this);
			} else if(((APP->window->getMods() & RACK_MOD_MASK) == GLFW_MOD_ALT))
			{
				BrowserOverlay* overlay = getAncestorOfType<BrowserOverlay>();
				overlay->permanentlyHide(model);
				e.consume(this);
			} else
			{
				bool hide;
				ModuleWidget* mw = chooseModel(model, &hide);
				if(hide)
				{
					BrowserOverlay* overlay = getAncestorOfType<BrowserOverlay>();
					overlay->Hide();
				}
				// Pretend the moduleWidget was clicked so it can be dragged in the RackWidget
				e.consume(mw);
			}
		}
	}

	void onEnter(const event::Enter& e) override {
		std::string text;
		text = model->plugin->brand;
		text += " " + model->name;
		// Tags
		text += "\nTags: ";
		for (size_t i = 0; i < model->tags.size(); i++) {
			if (i > 0)
				text += ", ";
			int tagId = model->tags[i];
			text += tag::tagAliases[tagId][0];
		}
		// Description
		if (model->description != "") {
			text += "\n" + model->description;
		}
		ui::Tooltip* tooltip = new ui::Tooltip;
		tooltip->text = text;
		setTooltip(tooltip);
	}

	void onLeave(const event::Leave& e) override {
		setTooltip(NULL);
	}

	void onHide(const event::Hide& e) override {
		// Hide tooltip
		setTooltip(NULL);
		OpaqueWidget::onHide(e);
	}
};


struct BrandItem : ui::MenuItem {
	void onAction(const event::Action& e) override;
	void step() override;
};


struct TagItem : ui::MenuItem {
	int tagId;
	void onAction(const event::Action& e) override;
	void step() override;
};


struct BrowserSearchField : ui::TextField {
	void step() override {
		// Steal focus when step is called
		APP->event->setSelected(this);
		TextField::step();
	}

	void onSelectKey(const event::SelectKey& e) override;
	void onChange(const event::Change& e) override;
	void onAction(const event::Action& e) override;

	void onHide(const event::Hide& e) override {
		APP->event->setSelected(NULL);
		ui::TextField::onHide(e);
	}

	void onShow(const event::Show& e) override {
		selectAll();
		TextField::onShow(e);
	}
};


struct ClearButton : ui::Button {
	void onAction(const event::Action& e) override;
};


struct BrowserSidebar : widget::Widget {
	BrowserSearchField* searchField;
	ClearButton* clearButton;
	ui::Label* tagLabel;
	ui::List* tagList;
	ui::ScrollWidget* tagScroll;
	ui::Label* brandLabel;
	ui::List* brandList;
	ui::ScrollWidget* brandScroll;

	BrowserSidebar() {
		// Search
		searchField = new BrowserSearchField;
		addChild(searchField);

		// Clear filters
		clearButton = new ClearButton;
		clearButton->text = "Reset filters (F1/F2/F3/F4)";
		addChild(clearButton);

		// Tag label
		tagLabel = new ui::Label;
		// tagLabel->fontSize = 16;
		tagLabel->color = nvgRGB(0x80, 0x80, 0x80);
		tagLabel->text = "Tags";
		addChild(tagLabel);

		// Tag list
		tagScroll = new ui::ScrollWidget;
		addChild(tagScroll);

		tagList = new ui::List;
		tagScroll->container->addChild(tagList);

		for (int tagId = 0; tagId < (int) tag::tagAliases.size(); tagId++) {
			TagItem* item = new TagItem;
			item->text = tag::tagAliases[tagId][0];
			item->tagId = tagId;
			tagList->addChild(item);
		}

		// Brand label
		brandLabel = new ui::Label;
		// brandLabel->fontSize = 16;
		brandLabel->color = nvgRGB(0x80, 0x80, 0x80);
		brandLabel->text = "Brands";
		addChild(brandLabel);

		// Brand list
		brandScroll = new ui::ScrollWidget;
		addChild(brandScroll);

		brandList = new ui::List;
		brandScroll->container->addChild(brandList);

		// Collect brands from all plugins
		std::set<std::string, string::CaseInsensitiveCompare> brands;
		for (plugin::Plugin* plugin : plugin::plugins) {
			brands.insert(plugin->brand);
		}

		for (const std::string& brand : brands) {
			BrandItem* item = new BrandItem;
			item->text = brand;
			brandList->addChild(item);
		}
	}

	void step() override {
		searchField->box.size.x = box.size.x;
		clearButton->box.pos = searchField->box.getBottomLeft();
		clearButton->box.size.x = box.size.x;

		float listHeight = (box.size.y - clearButton->box.getBottom()) / 2;
		listHeight = std::floor(listHeight);

		tagLabel->box.pos = clearButton->box.getBottomLeft();
		tagLabel->box.size.x = box.size.x;
		tagScroll->box.pos = tagLabel->box.getBottomLeft();
		tagScroll->box.size.y = listHeight - tagLabel->box.size.y;
		tagScroll->box.size.x = box.size.x;
		tagList->box.size.x = tagScroll->box.size.x;

		brandLabel->box.pos = tagScroll->box.getBottomLeft();
		brandLabel->box.size.x = box.size.x;
		brandScroll->box.pos = brandLabel->box.getBottomLeft();
		brandScroll->box.size.y = listHeight - brandLabel->box.size.y;
		brandScroll->box.size.x = box.size.x;
		brandList->box.size.x = brandScroll->box.size.x;

		Widget::step();
	}
};


struct ModuleBrowser : widget::OpaqueWidget {
	BrowserSidebar* sidebar;
	ui::ScrollWidget* modelScroll;
	ui::Label* modelLabel;
	ui::MarginLayout* modelMargin;
	ui::SequentialLayout* modelContainer;
	Favourites favour;

	std::string search;
	std::string brand;
	int tagId = -1;

	ModuleBrowser() {
		sidebar = new BrowserSidebar;
		sidebar->box.size.x = 200;
		addChild(sidebar);

		modelLabel = new ui::Label;
		// modelLabel->fontSize = 16;
		// modelLabel->box.size.x = 400;
		addChild(modelLabel);

		modelScroll = new ui::ScrollWidget;
		addChild(modelScroll);

		modelMargin = new ui::MarginLayout;
		modelMargin->margin = math::Vec(10, 10);
		modelScroll->container->addChild(modelMargin);

		modelContainer = new ui::SequentialLayout;
		modelContainer->spacing = math::Vec(10, 10);
		modelMargin->addChild(modelContainer);

		favour.Init(plugin::plugins);

		// Add ModelBoxes for each Model
		for (plugin::Plugin* plugin : plugin::plugins) {
			for (plugin::Model* model : plugin->models) {
				if(!favour.isHidden(model))
				{
					ModelBox* moduleBox = new ModelBox;
					moduleBox->setModel(model);
					modelContainer->addChild(moduleBox);
				}
			}
		}

		clear();
	}

	void step() override {
		box = parent->box.zeroPos().grow(math::Vec(-70, -70));

		sidebar->box.size.y = box.size.y;

		modelLabel->box.pos = sidebar->box.getTopRight().plus(math::Vec(5, 5));

		modelScroll->box.pos = sidebar->box.getTopRight().plus(math::Vec(0, 30));
		modelScroll->box.size = box.size.minus(modelScroll->box.pos);
		modelMargin->box.size.x = modelScroll->box.size.x;
		modelMargin->box.size.y = modelContainer->getChildrenBoundingBox().size.y + 2 * modelMargin->margin.y;

		OpaqueWidget::step();
	}

	void draw(const DrawArgs& args) override {
		bndMenuBackground(args.vg, 0.0, 0.0, box.size.x, box.size.y, 0);
		Widget::draw(args);
	}

	void refresh() {
		// Reset scroll position
		modelScroll->offset = math::Vec();

		// Filter ModelBoxes
		for (Widget* w : modelContainer->children) {
			ModelBox* m = dynamic_cast<ModelBox*>(w);
			assert(m);
			m->visible = isModelVisible(m->model, search, brand, tagId, favour);
		}

		// Sort ModelBoxes
		modelContainer->children.sort([&](Widget * w1, Widget * w2) {
			ModelBox* m1 = dynamic_cast<ModelBox*>(w1);
			ModelBox* m2 = dynamic_cast<ModelBox*>(w2);
			// Sort by (modifiedTimestamp descending, plugin brand)
			auto t1 = std::make_tuple(-m1->model->plugin->modifiedTimestamp, m1->model->plugin->brand);
			auto t2 = std::make_tuple(-m2->model->plugin->modifiedTimestamp, m2->model->plugin->brand);
			return t1 < t2;
		});

		if (search.empty()) {
			// We've already sorted above
		}
		else {
			std::map<Widget*, float> scores;
			// Compute scores
			for (Widget* w : modelContainer->children) {
				ModelBox* m = dynamic_cast<ModelBox*>(w);
				assert(m);
				if (!m->visible)
					continue;
				scores[m] = modelScore(m->model, search);
			}
			// // Sort by score
			// modelContainer->children.sort([&](Widget *w1, Widget *w2) {
			// 	// If score was not computed, scores[w] returns 0, but this doesn't matter because those widgets aren't visible.
			// 	return get_default(scores, w1, 0.f) > get_default(scores, w2, 0.f);
			// });
		}

		// Filter the brand and tag lists

		// Get modules that would be filtered by just the search query
		std::vector<plugin::Model*> filteredModels;
		for (Widget* w : modelContainer->children) {
			ModelBox* m = dynamic_cast<ModelBox*>(w);
			assert(m);
			if (isModelVisible(m->model, search, "", -1, favour))
				filteredModels.push_back(m->model);
		}

		auto hasModel = [&](const std::string & brand, int tagId) -> bool {
			for (plugin::Model* model : filteredModels) {
				if (isModelVisible(model, "", brand, tagId, favour))
					return true;
			}
			return false;
		};

		// Enable brand and tag items that are available in visible ModelBoxes
		int brandsLen = 0;
		for (Widget* w : sidebar->brandList->children) {
			BrandItem* item = dynamic_cast<BrandItem*>(w);
			assert(item);
			item->disabled = !hasModel(item->text, tagId);
			if (!item->disabled)
				brandsLen++;
		}
		sidebar->brandLabel->text = string::f("Brands (%d)", brandsLen);

		int tagsLen = 0;
		for (Widget* w : sidebar->tagList->children) {
			TagItem* item = dynamic_cast<TagItem*>(w);
			assert(item);
			item->disabled = !hasModel(brand, item->tagId);
			if (!item->disabled)
				tagsLen++;
		}
		sidebar->tagLabel->text = string::f("Tags (%d)", tagsLen);

		// Count models
		int modelsLen = 0;
		for (Widget* w : modelContainer->children) {
			if (w->visible)
				modelsLen++;
		}
		modelLabel->text = string::f("Modules (%d) Click and drag a module to place it in the rack. SHIFT=multiple modules, CTRL=favourite, ALT = permanently hide", modelsLen);
	}

	void clear() {
		search = "";
		sidebar->searchField->setText("");
		brand = "";
		tagId = -1;
		refresh();
	}

	void onShow(const event::Show& e) override {
		refresh();
		OpaqueWidget::onShow(e);
	}
};


// Implementations to resolve dependencies


inline void BrandItem::onAction(const event::Action& e) {
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	if (browser->brand == text)
		browser->brand = "";
	else
		browser->brand = text;
	browser->refresh();
}

inline void BrandItem::step() {
	MenuItem::step();
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	active = (browser->brand == text);
}

inline void TagItem::onAction(const event::Action& e) {
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	if (browser->tagId == tagId)
		browser->tagId = -1;
	else
		browser->tagId = tagId;
	browser->refresh();
}

inline void TagItem::step() {
	MenuItem::step();
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	active = (browser->tagId == tagId);
}

inline void BrowserSearchField::onSelectKey(const event::SelectKey& e) {
	if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
		switch (e.key) {
			case GLFW_KEY_ESCAPE: {
				BrowserOverlay* overlay = getAncestorOfType<BrowserOverlay>();
				overlay->Hide();
				e.consume(this);
			} break;
			case GLFW_KEY_BACKSPACE: {
				if (text == "") {
					ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
					browser->clear();
					e.consume(this);
				}
			} break;

			case GLFW_KEY_F1: {
				text = "";
				ModuleBrowser *browser = getAncestorOfType<ModuleBrowser>();
				browser->favour.showFavorites = false;
				browser->clear();
				e.consume(this);
			}
			break;

			case GLFW_KEY_F2:
			{
				ModuleBrowser *browser = getAncestorOfType<ModuleBrowser>();
				browser->clear();
				text = "";
				browser->brand = settings::F2Filter;
				browser->refresh();
			}
			break;
			case GLFW_KEY_F3:
			{
				ModuleBrowser *browser = getAncestorOfType<ModuleBrowser>();
				browser->clear();
				text = "";
				browser->brand = "VCV";
				browser->refresh();
			}
			break;
			case GLFW_KEY_F4:
			{
				ModuleBrowser *browser = getAncestorOfType<ModuleBrowser>();
				browser->clear();
				browser->favour.showFavorites = !browser->favour.showFavorites;
				text = "";
				browser->refresh();
			}
			break;
		}
	}

	if (!e.getTarget())
		ui::TextField::onSelectKey(e);
}

inline void BrowserSearchField::onChange(const event::Change& e) {
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	browser->search = string::trim(text);
	browser->refresh();
}

inline void BrowserSearchField::onAction(const event::Action& e) {
	// Get first ModelBox
	ModelBox* mb = NULL;
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	for (Widget* w : browser->modelContainer->children)
				{
					if (w->visible)
					{
						mb = dynamic_cast<ModelBox *>(w);
						break;
					}
				}

	if (mb) {
		bool hide;
		chooseModel(mb->model, &hide);
		if(hide)
		{
			BrowserOverlay* overlay = getAncestorOfType<BrowserOverlay>();
			overlay->Hide();
		}
	}
}

inline void ClearButton::onAction(const event::Action& e) {
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	browser->clear();
}

BrowserOverlay *BrowserOverlay::Create()
{
	mybrowser = new ModuleBrowser;
	addChild(mybrowser);
	return this;
}

void BrowserOverlay::Hide()
{
	mybrowser->favour.save();
	hide();
}

void BrowserOverlay::permanentlyHide(rack::plugin::Model *model)
{
	if(mybrowser->favour.hide(model))
		mybrowser->refresh();
}

void BrowserOverlay::toggleFavorite(rack::plugin::Model *model)
{
	mybrowser->favour.toggle(model);
	if(mybrowser->favour.showFavorites)
		mybrowser->refresh();
}

bool ModelBox::AmIfavourite()
{
	ModuleBrowser *browser = getAncestorOfType<ModuleBrowser>();
	return browser->favour.isFavorite(model);
}

// Global functions


widget::Widget* moduleBrowserCreate() {
	BrowserOverlay* overlay = new BrowserOverlay;
	return overlay->Create();
}


} // namespace app
} // namespace rack
