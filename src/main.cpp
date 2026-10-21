#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <unordered_map>
#include <unordered_set>

using namespace geode::prelude;

class $modify(GroupIDEditorUI, EditorUI) {
    struct Fields {
        std::unordered_map<GameObject*, CCLabelBMFont*> labels;
    };

    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer)) return false;
        this->schedule(schedule_selector(GroupIDEditorUI::refreshGroupLabels), 0.05f);
        return true;
    }

    static std::string buildText(GameObject* obj) {
        if (!obj->m_groups || obj->m_groupCount <= 0) return "";
        bool showAll = Mod::get()->getSettingValue<bool>("show-all-groups");
        int count = showAll ? obj->m_groupCount : 1;

        std::string out;
        for (int i = 0; i < count; i++) {
            if (i) out += ",";
            out += std::to_string((*obj->m_groups)[i]);
        }
        return out;
    }

    void refreshGroupLabels(float) {
        auto fields = m_fields.self();
        auto lel = m_editorLayer;
        if (!lel || !lel->m_objectLayer) return;

        // Playtesting: hide everything
        if (lel->m_playbackMode == PlaybackMode::Playing) {
            for (auto& [obj, label] : fields->labels) label->setVisible(false);
            return;
        }

        auto layer = lel->m_objectLayer;
        auto winSize = CCDirector::get()->getWinSize();
        CCPoint a = layer->convertToNodeSpace(ccp(0, 0));
        CCPoint b = layer->convertToNodeSpace(ccp(winSize.width, winSize.height));
        float margin = 60.f;
        CCRect view(
            std::min(a.x, b.x) - margin, std::min(a.y, b.y) - margin,
            std::abs(b.x - a.x) + margin * 2, std::abs(b.y - a.y) + margin * 2
        );

        auto opacity = static_cast<GLubyte>(Mod::get()->getSettingValue<int64_t>("label-opacity"));
        std::unordered_set<GameObject*> seen;

        CCObject* it;
        CCARRAY_FOREACH(lel->m_objects, it) {
            auto obj = static_cast<GameObject*>(it);
            if (!obj) continue;

            auto pos = obj->getPosition();
            if (!view.containsPoint(pos)) continue;

            std::string text = buildText(obj);
            if (text.empty()) continue;

            seen.insert(obj);

            CCLabelBMFont* label = nullptr;
            auto found = fields->labels.find(obj);
            if (found != fields->labels.end()) {
                label = found->second;
            } else {
                label = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
                label->setID("group-label"_spr);
                layer->addChild(label, 10000);
                fields->labels[obj] = label;
            }

            if (text != label->getString()) label->setString(text.c_str());

            // Fit the text inside a ~30 unit block, scaled with the object
            float objScale = std::max(0.3f, std::abs(obj->getScale()));
            float maxWidth = 26.f * objScale;
            float scale = 0.4f * objScale;
            float w = label->getContentSize().width * scale;
            if (w > maxWidth) scale *= maxWidth / w;

            label->setScale(scale);
            label->setPosition(pos);
            label->setOpacity(opacity);
            label->setVisible(obj->isVisible());
        }

        // Remove labels for anything off-screen or deleted (never dereference the key)
        for (auto itr = fields->labels.begin(); itr != fields->labels.end();) {
            if (!seen.count(itr->first)) {
                itr->second->removeFromParent();
                itr = fields->labels.erase(itr);
            } else {
                ++itr;
            }
        }
    }
};
