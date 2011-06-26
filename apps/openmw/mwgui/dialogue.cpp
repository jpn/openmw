#include "dialogue.hpp"
#include "dialogue_history.hpp"
#include "window_manager.hpp"
#include "../mwdialogue/dialoguemanager.hpp"
#include "widgets.hpp"
#include "components/esm_store/store.hpp"

#include <assert.h>
#include <iostream>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace MWGui;
using namespace Widgets;

DialogueWindow::DialogueWindow(WindowManager& parWindowManager)
  : WindowBase("openmw_dialogue_window_layout.xml", parWindowManager)
{
    // Centre dialog
    center();

    //NPC Name text field
    getWidget(npcName, "NpcName");

    //Disposition
    getWidget(disposition, "NpcDisposition");

    //History view
    getWidget(history, "History");
    history->initialiseOverride(); //FIXME remove when upgrading MyGUI
    history->setOverflowToTheLeft(true);
    history->eventTopicSelected = MyGUI::newDelegate(this, &DialogueWindow::onHistorySelectTopic);
   
    //Topics list 
    getWidget(topicsList, "TopicsList");
    topicsList->setScrollVisible(true);
    topicsList->eventListMouseItemActivate = MyGUI::newDelegate(this, &DialogueWindow::onSelectTopic);

    //Bye button
    MyGUI::ButtonPtr byeButton;
    getWidget(byeButton, "ByeButton");
    byeButton->eventMouseButtonClick = MyGUI::newDelegate(this, &DialogueWindow::onByeClicked);
}

void DialogueWindow::open()
{
    setVisible(true);
}


void DialogueWindow::onByeClicked(MyGUI::Widget* _sender)
{
    history->clear();
    setVisible(false);
    eventBye();
}


void DialogueWindow::onHistorySelectTopic(const std::string& parTopic, int parCond)
{
    eventTopicSelected(parTopic, parCond);
}


void DialogueWindow::onSelectTopic(MyGUI::List* _sender, size_t _index)
{
    if (_index == MyGUI::ITEM_NONE)
        return;

    const std::string* theTopic  = topicsList->getItemDataAt<std::string>(_index);
    eventTopicSelected(*theTopic, -1);
}


void DialogueWindow::startDialogue(std::string parName, int parDisposition, MWDialogue::DialogueMessage& parAnswer, std::vector<std::string>& parTopics)
{
    //Set Name
    npcName->setCaption(parName);

    //Set Disposition
    setDisposition(parDisposition);

    setTopics(parTopics);
    addAnswer(parAnswer);
}


void DialogueWindow::setDisposition(int parDisposition)
{
    std::stringstream ss;
    ss << parDisposition;
    disposition->setProperty("Progress_Position", ss.str());
}


void DialogueWindow::addAnswer(MWDialogue::DialogueMessage& parAnswer)
{
    history->addHistory(parAnswer);
}


void DialogueWindow::setTopics(std::vector<std::string>& parTopics)
{
    //Update the list of available topics
    topicsList->removeAllItems();
    std::vector<std::string>::iterator i;
    for(i = parTopics.begin(); i < parTopics.end(); i++)
    {
        topicsList->addItem(*i, *i);
    }

    //Update links in history view
    history->setTopics(parTopics);
}
