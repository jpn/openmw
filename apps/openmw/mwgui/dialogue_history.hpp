#ifndef MWGUI_DIALOGE_HISTORY_H
#define MWGUI_DIALOGE_HISTORY_H
#include <openengine/gui/layout.hpp>
#include <vector>
#include "../mwdialogue/dialoguemanager.hpp"

namespace MWGui
{
    using namespace MyGUI;
    class MYGUI_EXPORT DialogeHistory : public MyGUI::Edit
    {
        MYGUI_RTTI_DERIVED( DialogeHistory )
        private:
            //Data
            std::vector<MWDialogue::DialogueMessage> mHistory;
            std::vector<std::string> mTopics;

            //Colours
            std::string mTextColor;
            std::string mHeadingColor;
            std::string mLinkColor;
            std::string mOptionColor;

            void onHistoryClicked(MyGUI::Widget* _sender);
            UString getColorAtPos(size_t _pos);
            UString getColorTextAt(size_t _pos);

            std::string::size_type replaceOnce(std::string& str, std::string what, std::string to, std::string::size_type start = 0);
            int replaceAll(std::string& str, std::string what, std::string to);
            void update();
            void addDialogHeading(const UString& parText);
            void addDialogText(const UString& parText);
            void addHistoryText(const MWDialogue::DialogueMessage& parHistory);

        public:
            typedef delegates::CDelegate2<const std::string&, int> EventHandle_String;
            EventHandle_String eventTopicSelected;

            DialogeHistory();
            virtual void initialiseOverride();
            //Widget* getClient() { return mClient; }
            void addHistory(MWDialogue::DialogueMessage& parHistory);
            void setTopics(std::vector<std::string>& parTopics);
            void clear();
    };
}
#endif

