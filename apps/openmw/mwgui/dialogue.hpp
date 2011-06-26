#ifndef MWGUI_DIALOGE_H
#define MWGUI_DIALOGE_H

#include "window_base.hpp"
#include "../mwdialogue/dialoguemanager.hpp"

#include <boost/array.hpp>

namespace MWGui
{
    class WindowManager;
}

/*
  This file contains the dialouge window
  Layout is defined by resources/mygui/openmw_dialogue_window_layout.xml.
 */

namespace MWGui
{
    class DialogeHistory;

    using namespace MyGUI;

    class DialogueWindow: public WindowBase, public MWDialogue::DialogueInterface
    {
    public:
        DialogueWindow(WindowManager& parWindowManager);

        void open();

        // Events
        typedef delegates::CDelegate0 EventHandle_Void;
        typedef delegates::CDelegate2<const std::string&, int> EventHandle_String;

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBye;
        EventHandle_String eventTopicSelected;

        virtual void startDialogue(std::string parName, int parDisposition, MWDialogue::DialogueMessage& parAnswer, std::vector<std::string>& parTopics);
        virtual void setDisposition(int parDisposition);
        virtual void addAnswer(MWDialogue::DialogueMessage& parAnswer);
        virtual void setTopics(std::vector<std::string>& parTopics);

    protected:
        void onSelectTopic(MyGUI::List* _sender, size_t _index);
        void onByeClicked(MyGUI::Widget* _sender);

    private:
        void onHistorySelectTopic(const std::string& parTopic, int parCond);

        DialogeHistory*         history;
        MyGUI::ListPtr          topicsList;
        MyGUI::StaticTextPtr    npcName;
        MyGUI::ProgressPtr      disposition;
    };
}
#endif
