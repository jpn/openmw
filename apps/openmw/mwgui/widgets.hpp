#ifndef MWGUI_WIDGETS_H
#define MWGUI_WIDGETS_H

#include <components/esm_store/store.hpp>

#include <MyGUI.h>

#include "../mwmechanics/stat.hpp"

/*
  This file contains various custom widgets used in OpenMW.
 */

namespace MWGui
{
    using namespace MyGUI;
    class WindowManager;

    namespace Widgets
    {
        class MYGUI_EXPORT MWSkill : public Widget
        {
            MYGUI_RTTI_DERIVED( MWSkill );
        public:
            MWSkill();

            typedef MWMechanics::Stat<float> SkillValue;

            void setWindowManager(WindowManager *m) { manager = m; }
            void setSkillId(ESM::Skill::SkillEnum skillId);
            void setSkillNumber(int skillId);
            void setSkillValue(const SkillValue& value);

            WindowManager *getWindowManager() const { return manager; }
            ESM::Skill::SkillEnum getSkillId() const { return skillId; }
            const SkillValue& getSkillValue() const { return value; }

        /*internal:*/
		    virtual void _initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);

        protected:
		    virtual ~MWSkill();

		    void baseChangeWidgetSkin(ResourceSkin* _info);

	    private:
		    void initialiseWidgetSkin(ResourceSkin* _info);
		    void shutdownWidgetSkin();

            void updateWidgets();

            WindowManager *manager;
            ESM::Skill::SkillEnum skillId;
            SkillValue value;
            MyGUI::StaticTextPtr skillNameWidget, skillValueWidget;
        };
        typedef MWSkill* MWSkillPtr;

        class MYGUI_EXPORT MWAttribute : public Widget
        {
            MYGUI_RTTI_DERIVED( MWAttribute );
        public:
            MWAttribute();

            typedef MWMechanics::Stat<int> AttributeValue;

            void setWindowManager(WindowManager *m) { manager = m; }
            void setAttributeId(int attributeId);
            void setAttributeValue(const AttributeValue& value);

            WindowManager *getWindowManager() const { return manager; }
            int getAttributeId() const { return id; }
            const AttributeValue& getAttributeValue() const { return value; }

        /*internal:*/
		    virtual void _initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);

        protected:
		    virtual ~MWAttribute();

		    void baseChangeWidgetSkin(ResourceSkin* _info);

	    private:
		    void initialiseWidgetSkin(ResourceSkin* _info);
		    void shutdownWidgetSkin();

            void updateWidgets();

            WindowManager *manager;
            int id;
            AttributeValue value;
            MyGUI::StaticTextPtr attributeNameWidget, attributeValueWidget;
        };
        typedef MWAttribute* MWAttributePtr;
    }
}

#endif