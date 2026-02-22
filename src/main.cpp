#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;


static const char* getSpeedPortal(Speed speed) {
    switch (speed) {
        case Speed::Slow:    return "boost_01_001.png";
        case Speed::Normal:  return "boost_02_001.png";
        case Speed::Fast:    return "boost_03_001.png";
        case Speed::Faster:  return "boost_04_001.png";
        case Speed::Fastest: return "boost_05_001.png";
        default:             return "boost_02_001.png";
    }
}

//01-02=gravity 03=cube 04=ship 05-06=mirror 07=ball
//08-09=size 10=UFO 11-12=dual 13=wave 14=robot 15=spider 16=swing
//robtop adding shit in release order instead of category order wtf
//he did the same shit with the speed portals
//tbh i would do the same thing lmaoo im too lazy to organize
static const char* getModePortal(int mode) {
    switch (mode) {
        case 0: return "portal_03_front_001.png";
        case 1: return "portal_04_front_001.png";
        case 2: return "portal_07_front_001.png";
        case 3: return "portal_10_front_001.png";
        case 4: return "portal_13_front_001.png";
        case 5: return "portal_14_front_001.png";
        case 6: return "portal_17_front_001.png";
        case 7: return "portal_18_front_001.png";
        default: return "portal_03_front_001.png";
    }
}

//name storage (i hope this doesnt corrupt level saves heehee)

static std::unordered_map<StartPosObject*, std::string> s_nameCache;

static std::string getNameKey(StartPosObject* sp) {
    auto p = sp->getPosition();
    return fmt::format("sp_name_{:.0f}_{:.0f}", p.x, p.y);
}

static std::string getDisplayName(StartPosObject* sp, size_t idx) {
    auto it = s_nameCache.find(sp);
    if (it != s_nameCache.end() && !it->second.empty()) return it->second;
    auto saved = Mod::get()->getSavedValue<std::string>(getNameKey(sp), "");
    if (!saved.empty()) {
        s_nameCache[sp] = saved;
        return saved;
    }
    return fmt::format("Start Pos {}", idx + 1);
}

static void setStartPosName(StartPosObject* sp, const std::string& name) {
    s_nameCache[sp] = name;
    Mod::get()->setSavedValue<std::string>(getNameKey(sp), name);
}

class StartPosPopup;

class PopupReopener : public CCNode {
    EditorUI* m_editorUI;
public:
    static PopupReopener* create(EditorUI* e) {
        auto r = new PopupReopener(); r->m_editorUI = e; r->autorelease(); return r;
    }
    void start() {
        CCDirector::sharedDirector()->getScheduler()->scheduleSelector(
            schedule_selector(PopupReopener::check), this, 0.1f, false
        );
    }
    void check(float);
    void die() { 
        CCDirector::sharedDirector()->getScheduler()->unscheduleSelector(
            schedule_selector(PopupReopener::check), this
        );
        removeFromParent(); 
    }
};

//name editor popup
class NamePopup : public geode::Popup {
    StartPosObject* m_sp = nullptr;
    TextInput* m_input = nullptr;
    StartPosPopup* m_parent = nullptr;

    bool setup(StartPosObject* sp, StartPosPopup* parent) {
        m_sp = sp; m_parent = parent;
        this->setTitle("Rename Start Position");
        m_input = TextInput::create(200.f, "Enter name...", "bigFont.fnt");
        auto existing = Mod::get()->getSavedValue<std::string>(getNameKey(sp), "");
        if (!existing.empty()) m_input->setString(existing);
        m_input->setMaxCharCount(24);
        m_buttonMenu->addChildAtPosition(m_input, Anchor::Center, {0.f, 5.f});

        auto saveSpr = ButtonSprite::create("Save", "bigFont.fnt", "GJ_button_01.png", 0.8f);
        saveSpr->setScale(0.7f);
        auto saveBtn = CCMenuItemSpriteExtra::create(saveSpr, this, menu_selector(NamePopup::onSave));
        m_buttonMenu->addChildAtPosition(saveBtn, Anchor::Bottom, {0.f, 25.f});
        return true;
    }
    void onSave(CCObject*);
public:
    static NamePopup* create(StartPosObject* sp, StartPosPopup* parent) {
        auto r = new NamePopup();
        if (r->init(250.f, 140.f)) { r->setup(sp, parent); r->autorelease(); return r; }
        delete r; return nullptr;
    }
};

//speed popup and mode popup

class SpeedPopup : public geode::Popup {
protected:
    StartPosObject* m_sp;
    StartPosPopup* m_parent;

    bool init(StartPosObject* sp, StartPosPopup* parent);
    void onSelect(CCObject* sender);

public:
    static SpeedPopup* create(StartPosObject* sp, StartPosPopup* parent) {
        auto r = new SpeedPopup();
        if (r->init(sp, parent)) { 
            r->autorelease(); 
            return r; 
        }
        delete r; return nullptr;
    }
};

class ModePopup : public geode::Popup {
protected:
    StartPosObject* m_sp;
    StartPosPopup* m_parent;

    bool init(StartPosObject* sp, StartPosPopup* parent);
    void onSelect(CCObject* sender);

public:
    static ModePopup* create(StartPosObject* sp, StartPosPopup* parent) {
        auto r = new ModePopup();
        if (r->init(sp, parent)) { 
            r->autorelease(); 
            return r; 
        }
        delete r; return nullptr;
    }
};

//startpos popup

class StartPosPopup : public geode::Popup {
protected:
    EditorUI* m_editorUI = nullptr;
    ScrollLayer* m_scrollLayer = nullptr;
    static float s_savedScroll;
    static bool s_hasScroll;

    void onClose(CCObject* s) override {
        s_savedScroll = m_scrollLayer->m_contentLayer->getPositionY();
        s_hasScroll = true;
        Popup::onClose(s);
    }

    void onInfo(CCObject*) {
        geode::createQuickPopup(
            "Start Position Viewer",
            "You can <cy>view</c> and <cl>manage</c> all of the start positions placed in the level in one place.\n\n<cp>Saved you some time? Buy me a coffee!</c>",
            "Cancel", "Open",
            [](auto, bool btn2) {
                if (btn2) {
                    geode::utils::web::openLinkInBrowser("https://buymeacoffee.com/d050");
                }
            },
            false
        )->show();
    }

    bool setup(EditorUI* editorUI) {
        m_editorUI = editorUI;
        this->setTitle("Start Positions");

        float sw = 360.f, sh = 195.f;
        auto bg = m_bgSprite->getContentSize();

        m_scrollLayer = ScrollLayer::create({sw, sh});
        m_scrollLayer->m_cutContent = true;
        m_scrollLayer->setPosition({(bg.width - sw) / 2.f, 42.f});
        m_bgSprite->addChild(m_scrollLayer);

        //info button (i hope to become a millionaire because of this)
        auto infoMenu = CCMenu::create();
        infoMenu->setPosition({bg.width - 20.f, bg.height - 20.f});
        m_bgSprite->addChild(infoMenu);

        auto infoSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        if (infoSpr) {
            infoSpr->setScale(0.8f);
            auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, this, menu_selector(StartPosPopup::onInfo));
            infoMenu->addChild(infoBtn);
        }

        refreshList();
        if (s_hasScroll) m_scrollLayer->m_contentLayer->setPositionY(s_savedScroll);

        //bottom bar
        auto bar = CCMenu::create();
        bar->setContentSize({sw, 22.f});
        bar->ignoreAnchorPointForPosition(false);
        bar->setAnchorPoint({0.5f, 0.5f});
        bar->setPosition({bg.width / 2.f, 28.f});
        m_bgSprite->addChild(bar);

        auto mkBtn = [&](const char* t, const char* s, SEL_MenuHandler cb) {
            auto sp = ButtonSprite::create(t, "bigFont.fnt", s, 0.6f);
            sp->setScale(0.4f);
            return CCMenuItemSpriteExtra::create(sp, this, cb);
        };
        bar->addChild(mkBtn("Enable All",  "GJ_button_01.png", menu_selector(StartPosPopup::onEnableAll)));
        bar->addChild(mkBtn("Disable All", "GJ_button_06.png", menu_selector(StartPosPopup::onDisableAll)));
        bar->setLayout(RowLayout::create()->setGap(8.f)->setAxisAlignment(AxisAlignment::Center));
        bar->updateLayout();

        //refresh button
        auto refreshMenu = CCMenu::create();
        refreshMenu->setContentSize({30.f, 30.f});
        refreshMenu->ignoreAnchorPointForPosition(false);
        refreshMenu->setAnchorPoint({1.f, 0.f});
        refreshMenu->setPosition({bg.width - 8.f, 8.f});
        m_bgSprite->addChild(refreshMenu);

        auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
        if (!refreshSpr) refreshSpr = CCSprite::createWithSpriteFrameName("GJ_replayBtn_001.png");
        if (refreshSpr) {
            refreshSpr->setScale(0.5f);
            auto rb = CCMenuItemSpriteExtra::create(refreshSpr, this, menu_selector(StartPosPopup::onRefresh));
            rb->setPosition({15.f, 15.f});
            refreshMenu->addChild(rb);
        }

        return true;
    }

    //card
    CCNode* createCard(StartPosObject* sp, size_t idx, float w) {
        float h = 65.f;
        auto card = CCNode::create();
        card->setContentSize({w, h});
        card->setAnchorPoint({0.f, 0.5f});

        auto settings = sp->m_startSettings;
        bool off = settings ? settings->m_disableStartPos : false;

        //panel bg
        auto panel = CCScale9Sprite::create("square02_small.png");
        panel->setContentSize({w - 8.f, h - 4.f});
        panel->setAnchorPoint({0.5f, 0.5f});
        panel->setPosition({w / 2.f, h / 2.f});
        panel->setColor({0, 0, 0});
        panel->setOpacity(120);
        card->addChild(panel, -1);

        if (off) {
            auto redOv = CCLayerColor::create(ccc4(180, 20, 20, 80), w - 8.f, h - 4.f);
            redOv->setAnchorPoint({0.5f, 0.5f});
            redOv->ignoreAnchorPointForPosition(false);
            redOv->setPosition({w / 2.f, h / 2.f});
            card->addChild(redOv, 0);
        }

        GLubyte alpha = off ? 80 : 255;

        //button shit
        auto menu = CCMenu::create();
        menu->setContentSize(card->getContentSize());
        menu->setPosition({0.f, 0.f});
        menu->setAnchorPoint({0.f, 0.f});
        menu->ignoreAnchorPointForPosition(false);
        card->addChild(menu);

        //name and rename icon
        auto nameLbl = CCLabelBMFont::create(getDisplayName(sp, idx).c_str(), "goldFont.fnt");
        nameLbl->setScale(0.48f);
        nameLbl->setAnchorPoint({0.5f, 0.5f});
        nameLbl->setPosition({w / 2.f, h - 14.f});
        nameLbl->limitLabelWidth(w * 0.45f, 0.48f, 0.15f);
        card->addChild(nameLbl);

        auto pencilSpr = CCSprite::createWithSpriteFrameName("GJ_chatBtn_001.png");
        if (!pencilSpr) pencilSpr = CCSprite::createWithSpriteFrameName("GJ_infoBtn_001.png");
        if (pencilSpr) {
            pencilSpr->setScale(0.18f);
            auto pencilBtn = CCMenuItemSpriteExtra::create(pencilSpr, this, menu_selector(StartPosPopup::onRename));
            pencilBtn->setUserObject(sp);
            pencilBtn->setTag(idx);
            pencilBtn->setPosition({w / 2.f + nameLbl->getScaledContentWidth() / 2.f + 12.f, h - 14.f});
            menu->addChild(pencilBtn);
        }

        //PORTALS
        if (settings) {
            float topY = 46.f;
            float botY = 18.f;

            //le background for aforementioned portals
            auto portalsBg = CCScale9Sprite::create("square02_small.png");
            portalsBg->setContentSize({90.f, 50.f});
            portalsBg->setPosition({60.f, h / 2.f});
            portalsBg->setColor({0, 0, 0});
            portalsBg->setOpacity(120);
            card->addChild(portalsBg, -1);

            auto wrapBox = [](CCNode* spr) {
                return spr;
            };
            //top row
            auto spSpr = CCSprite::createWithSpriteFrameName(getSpeedPortal(settings->m_startSpeed));
            if (spSpr) {
                spSpr->setScale(0.28f); spSpr->setOpacity(alpha);
                auto spBtn = CCMenuItemSpriteExtra::create(wrapBox(spSpr), this, menu_selector(StartPosPopup::onSpeedPopup));
                spBtn->setUserObject(sp); spBtn->setPosition({45.f, topY});
                spBtn->m_scaleMultiplier = 1.15f; menu->addChild(spBtn);
            }

            auto gmSpr = CCSprite::createWithSpriteFrameName(getModePortal(settings->m_startMode));
            if (gmSpr) {
                gmSpr->setScale(0.28f); gmSpr->setOpacity(alpha);
                auto gmBtn = CCMenuItemSpriteExtra::create(wrapBox(gmSpr), this, menu_selector(StartPosPopup::onModePopup));
                gmBtn->setUserObject(sp); gmBtn->setPosition({75.f, topY});
                gmBtn->m_scaleMultiplier = 1.15f; menu->addChild(gmBtn);
            }

            //bottom row
            float subScale = 0.18f;

            auto miniSpr = CCSprite::createWithSpriteFrameName(
                settings->m_startMini ? "portal_09_front_001.png" : "portal_08_front_001.png");
            if (miniSpr) {
                miniSpr->setScale(subScale); miniSpr->setOpacity(alpha);
                auto btn = CCMenuItemSpriteExtra::create(wrapBox(miniSpr), this, menu_selector(StartPosPopup::onToggleMini));
                btn->setUserObject(sp); btn->setPosition({30.f, botY}); 
                btn->m_scaleMultiplier = 1.15f; menu->addChild(btn);
            }

            auto dualSpr = CCSprite::createWithSpriteFrameName(
                settings->m_startDual ? "portal_11_front_001.png" : "portal_12_front_001.png");
            if (dualSpr) {
                dualSpr->setScale(subScale); dualSpr->setOpacity(alpha);
                auto btn = CCMenuItemSpriteExtra::create(wrapBox(dualSpr), this, menu_selector(StartPosPopup::onToggleDual));
                btn->setUserObject(sp); btn->setPosition({60.f, botY}); 
                btn->m_scaleMultiplier = 1.15f; menu->addChild(btn);
            }

            auto mirSpr = CCSprite::createWithSpriteFrameName(
                settings->m_mirrorMode ? "portal_05_front_001.png" : "portal_06_front_001.png");
            if (mirSpr) {
                mirSpr->setScale(subScale); mirSpr->setOpacity(alpha);
                auto btn = CCMenuItemSpriteExtra::create(wrapBox(mirSpr), this, menu_selector(StartPosPopup::onToggleMirror));
                btn->setUserObject(sp); btn->setPosition({90.f, botY}); 
                btn->m_scaleMultiplier = 1.15f; menu->addChild(btn);
            }
        }

        //coords
        auto pos = sp->getPosition();
        auto posLbl = CCLabelBMFont::create(
            fmt::format("X:{:.0f} Y:{:.0f}", pos.x, pos.y).c_str(), "chatFont.fnt");
        posLbl->setScale(0.3f);
        posLbl->setAnchorPoint({0.5f, 0.5f});
        posLbl->setPosition({w / 2.f, h - 23.f});
        posLbl->setColor({120, 120, 120});
        card->addChild(posLbl);

        //buttons on the right
        float listBtnY = h / 2.f;

        //goto button
        auto goSpr = ButtonSprite::create("Go To", "bigFont.fnt", "GJ_button_04.png", 0.6f);
        goSpr->setScale(0.35f);
        auto goBtn = CCMenuItemSpriteExtra::create(goSpr, this, menu_selector(StartPosPopup::onGoTo));
        goBtn->setUserObject(sp);
        goBtn->setPosition({w - 85.f, listBtnY + 14.f});
        menu->addChild(goBtn);

        //edit button
        auto editSpr = ButtonSprite::create("Edit", "bigFont.fnt", "GJ_button_02.png", 0.6f);
        editSpr->setScale(0.35f);
        auto editBtn = CCMenuItemSpriteExtra::create(editSpr, this, menu_selector(StartPosPopup::onEdit));
        editBtn->setUserObject(sp);
        editBtn->setPosition({w - 85.f, listBtnY - 12.f});
        menu->addChild(editBtn);

        //on off button
        auto tLbl = CCLabelBMFont::create(off ? "Off" : "On", "bigFont.fnt");
        tLbl->setScale(0.55f);
        auto tSpr = geode::CircleButtonSprite::create(tLbl,
            off ? geode::CircleBaseColor::Gray : geode::CircleBaseColor::Green,
            geode::CircleBaseSize::Medium);
        tSpr->setScale(0.7f);
        auto tBtn = CCMenuItemSpriteExtra::create(tSpr, this, menu_selector(StartPosPopup::onToggle));
        tBtn->setUserObject(sp);
        tBtn->setPosition({w - 32.f, listBtnY});
        menu->addChild(tBtn);

        return card;
    }

public:
    //list panels
    float m_savedY = 0.f;
    void saveScroll() { m_savedY = m_scrollLayer->m_contentLayer->getPositionY(); }
    void restoreScroll() { m_scrollLayer->m_contentLayer->setPositionY(m_savedY); }

    void refreshList(bool keep = false) {
        if (keep) saveScroll();
        auto content = m_scrollLayer->m_contentLayer;
        content->removeAllChildren();

        auto objects = m_editorUI->m_editorLayer->m_objects;
        std::vector<StartPosObject*> sps;
        if (objects) {
            for (int i = 0; i < objects->count(); i++) {
                auto o = static_cast<GameObject*>(objects->objectAtIndex(i));
                if (o && o->m_objectID == 31) sps.push_back(static_cast<StartPosObject*>(o));
            }
        }

        float cardH = 65.f, gap = 5.f, sw = 360.f, sh = 195.f;

        if (sps.empty()) {
            auto lbl = CCLabelBMFont::create("No start positions found", "bigFont.fnt");
            lbl->setScale(0.35f); lbl->setColor({150,150,150});
            lbl->setPosition({sw/2.f, sh/2.f});
            content->setContentSize({sw, sh});
            content->addChild(lbl);
            return;
        }

        std::sort(sps.begin(), sps.end(), [](auto a, auto b) {
            return a->getPositionX() < b->getPositionX();
        });

        float total = std::max((float)(sps.size() * (cardH + gap) + gap), sh);
        content->setContentSize({sw, total});

        for (size_t i = 0; i < sps.size(); i++) {
            auto card = createCard(sps[i], i, sw);
            card->setPosition({0.f, total - (i + 0.5f) * (cardH + gap)});
            content->addChild(card);
        }

        if (keep) restoreScroll();
        else m_scrollLayer->scrollToTop();
    }

protected:
    void onToggle(CCObject* s) {
        auto sp = static_cast<StartPosObject*>(static_cast<CCNode*>(s)->getUserObject());
        if (sp && sp->m_startSettings) {
            sp->m_startSettings->m_disableStartPos = !sp->m_startSettings->m_disableStartPos;
            refreshList(true);
        }
    }
    void onGoTo(CCObject* s) {
        auto sp = static_cast<StartPosObject*>(static_cast<CCNode*>(s)->getUserObject());
        if (!sp || !m_editorUI) return;
        this->onClose(nullptr);
        m_editorUI->selectObject(sp, true);
        auto ws = CCDirector::sharedDirector()->getWinSize();
        auto ol = m_editorUI->m_editorLayer->m_objectLayer;
        float sc = ol->getScale(); auto lp = ol->getPosition();
        m_editorUI->moveGamelayer({
            -sp->getPositionX()*sc + ws.width/2.f - lp.x,
            -sp->getPositionY()*sc + ws.height/2.f - lp.y
        });
    }
    void onEdit(CCObject* s) {
        auto sp = static_cast<StartPosObject*>(static_cast<CCNode*>(s)->getUserObject());
        if (!sp || !m_editorUI) return;
        auto editor = m_editorUI;
        this->onClose(nullptr);
        editor->selectObject(sp, true);
        editor->editObject(sp);
        PopupReopener::create(editor)->start();
    }
    void onEnableAll(CCObject*) { setAll(false); }
    void onDisableAll(CCObject*) { setAll(true); }
    void setAll(bool disable) {
        auto objects = m_editorUI->m_editorLayer->m_objects;
        if (!objects) return;
        for (int i = 0; i < objects->count(); i++) {
            auto o = static_cast<GameObject*>(objects->objectAtIndex(i));
            if (o && o->m_objectID == 31) {
                auto sp = static_cast<StartPosObject*>(o);
                if (sp->m_startSettings) sp->m_startSettings->m_disableStartPos = disable;
            }
        }
        refreshList(true);
    }
    void onRefresh(CCObject*) { refreshList(true); }
    void onRename(CCObject* s) {
        auto sp = static_cast<StartPosObject*>(static_cast<CCNode*>(s)->getUserObject());
        if (!sp) return;
        NamePopup::create(sp, this)->show();
    }

    //portal icon shenanigans
    void onToggleMini(CCObject* s) {
        auto sp = static_cast<StartPosObject*>(static_cast<CCNode*>(s)->getUserObject());
        if (sp && sp->m_startSettings) {
            sp->m_startSettings->m_startMini = !sp->m_startSettings->m_startMini;
            refreshList(true);
        }
    }
    void onToggleDual(CCObject* s) {
        auto sp = static_cast<StartPosObject*>(static_cast<CCNode*>(s)->getUserObject());
        if (sp && sp->m_startSettings) {
            sp->m_startSettings->m_startDual = !sp->m_startSettings->m_startDual;
            refreshList(true);
        }
    }
    void onToggleMirror(CCObject* s) {
        auto sp = static_cast<StartPosObject*>(static_cast<CCNode*>(s)->getUserObject());
        if (sp && sp->m_startSettings) {
            sp->m_startSettings->m_mirrorMode = !sp->m_startSettings->m_mirrorMode;
            refreshList(true);
        }
    }

    //speed and gamemode popups
    void onSpeedPopup(CCObject* s) {
        auto sp = static_cast<StartPosObject*>(static_cast<CCNode*>(s)->getUserObject());
        if (!sp) return;
        SpeedPopup::create(sp, this)->show();
    }
    void onModePopup(CCObject* s) {
        auto sp = static_cast<StartPosObject*>(static_cast<CCNode*>(s)->getUserObject());
        if (!sp) return;
        ModePopup::create(sp, this)->show();
    }

public:
    static StartPosPopup* create(EditorUI* e) {
        auto r = new StartPosPopup();
        if (r->init(410.f, 290.f)) { r->setup(e); r->autorelease(); return r; }
        delete r; return nullptr;
    }
    void doRefresh() { refreshList(true); }
};

float StartPosPopup::s_savedScroll = 0.f;
bool StartPosPopup::s_hasScroll = false;

//deferred implementation


void NamePopup::onSave(CCObject*) {
    if (!m_sp || !m_input) return;
    setStartPosName(m_sp, m_input->getString());
    if (m_parent) m_parent->refreshList(true);
    
    //fuck this shit bro i hate memory management
    geode::queueInMainThread([this] {
        if (this->getParent()) this->onClose(nullptr);
    });
}

void PopupReopener::check(float) {
    if (!m_editorUI || !m_editorUI->m_editorLayer || !m_editorUI->m_editorLayer->m_editorUI) {
        this->die();
        return;
    }
    
    bool isSettingsOpen = false;
    if (auto scene = CCDirector::sharedDirector()->getRunningScene()) {
        for (auto node : CCArrayExt<CCNode*>(scene->getChildren())) {
            if (dynamic_cast<FLAlertLayer*>(node) && node->getZOrder() >= 100) {
                isSettingsOpen = true;
                break;
            }
        }
    }

    if (!isSettingsOpen) {
        StartPosPopup::create(m_editorUI)->show();
        this->die();
    }
}

bool SpeedPopup::init(StartPosObject* sp, StartPosPopup* parent) {
    if (!Popup::init(340.f, 120.f)) return false;
    
    m_sp = sp; m_parent = parent;
    this->setTitle("Select Speed");

    auto menu = CCMenu::create();
    menu->setContentSize(m_mainLayer->getContentSize());
    menu->setPosition({0, 0});
    m_mainLayer->addChild(menu);

    float spacing = 60.f;
    float startX = m_mainLayer->getContentSize().width / 2.f - (spacing * 2);
    
    for (int i = 0; i < 5; i++) {
        // 0 = 1x speed, 1 = 0.5x speed, 2 = 2x speed, 3 = 3x speed, 4 = 4x speed
        //why the fuck is it like this?? i guess because 1x was added first before 0.5x. i guess bro
        int realID = (i == 0) ? 1 : (i == 1) ? 0 : i;

        auto spr = CCSprite::createWithSpriteFrameName(getSpeedPortal(static_cast<Speed>(realID)));
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(SpeedPopup::onSelect));
        btn->setTag(realID);
        btn->setPosition({startX + i * spacing, m_mainLayer->getContentSize().height / 2.f - 10.f});
        menu->addChild(btn);
    }
    return true;
}

void SpeedPopup::onSelect(CCObject* sender) {
    if (m_sp && m_sp->m_startSettings) {
        m_sp->m_startSettings->m_startSpeed = static_cast<Speed>(sender->getTag());
        m_parent->refreshList(true);
    }
    geode::queueInMainThread([this] {
        if (this->getParent()) this->onClose(nullptr);
    });
}

bool ModePopup::init(StartPosObject* sp, StartPosPopup* parent) {
    if (!Popup::init(380.f, 130.f)) return false;

    m_sp = sp; m_parent = parent;
    this->setTitle("Select Gamemode");

    auto menu = CCMenu::create();
    menu->setContentSize(m_mainLayer->getContentSize());
    menu->setPosition({0, 0});
    m_mainLayer->addChild(menu);

    float spacing = 42.f;
    float startX = m_mainLayer->getContentSize().width / 2.f - (spacing * 3.5f);
    
    for (int i = 0; i < 8; i++) {
        auto spr = CCSprite::createWithSpriteFrameName(getModePortal(i));
        spr->setScale(0.8f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ModePopup::onSelect));
        btn->setTag(i);
        btn->setPosition({startX + i * spacing, m_mainLayer->getContentSize().height / 2.f - 10.f});
        menu->addChild(btn);
    }
    return true;
}

void ModePopup::onSelect(CCObject* sender) {
    if (m_sp && m_sp->m_startSettings) {
        m_sp->m_startSettings->m_startMode = sender->getTag();
        m_parent->refreshList(true);
    }
    geode::queueInMainThread([this] {
        if (this->getParent()) this->onClose(nullptr);
    });
}

//EditorUI hook (thank you geode owo)

class $modify(StartPosEditorUI, EditorUI) {
    struct Fields { CCMenuItemSpriteExtra* m_spBtn = nullptr; };

    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto innerSpr = CCSprite::create("logo2.png"_spr);
        CCNode* btnNode = nullptr;

        if (innerSpr) {
            innerSpr->setScale(22.f / std::max(innerSpr->getContentSize().width, innerSpr->getContentSize().height));
            btnNode = CircleButtonSprite::create(innerSpr, CircleBaseColor::Gray, CircleBaseSize::Small);
        } else {
            //fallback incase logo dont work (but itll work trust me)
            auto p = CCSprite::createWithSpriteFrameName("GJ_plainBtn_001.png");
            p->setScale(0.55f);
            auto l = CCLabelBMFont::create("SP", "bigFont.fnt");
            l->setScale(0.45f); l->setPosition(p->getContentSize() / 2.f);
            p->addChild(l);
            btnNode = p;
        }

        auto btn = CCMenuItemSpriteExtra::create(
            btnNode, this, menu_selector(StartPosEditorUI::onStartPosViewer));
        m_fields->m_spBtn = btn;

        if (m_playtestBtn && m_playbackBtn && m_playtestBtn->getParent()) {
            auto menu = m_playtestBtn->getParent();
            menu->addChild(btn);

            auto playtestWorld = m_playtestBtn->getParent()->convertToWorldSpace(m_playtestBtn->getPosition());
            auto playbackWorld = m_playbackBtn->getParent()->convertToWorldSpace(m_playbackBtn->getPosition());
            auto midWorld = ccp((playtestWorld.x + playbackWorld.x) / 2.f, (playtestWorld.y + playbackWorld.y) / 2.f);
            auto localPos = menu->convertToNodeSpace(midWorld);

            btn->setPosition({localPos.x + 20.f, localPos.y});
            btn->setScale(0.55f);
            btn->m_baseScale = 0.55f;
        }
        return true;
    }

    void showUI(bool show) {
        EditorUI::showUI(show);
        if (m_fields->m_spBtn) m_fields->m_spBtn->setVisible(show);
    }

    void onStartPosViewer(CCObject*) { StartPosPopup::create(this)->show(); }
};