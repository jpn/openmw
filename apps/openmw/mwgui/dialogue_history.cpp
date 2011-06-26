#include "dialogue_history.hpp"
#include "window_manager.hpp"
#include "widgets.hpp"
#include "components/esm_store/store.hpp"

#include <assert.h>
#include <iostream>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace MWGui;
using namespace Widgets;

DialogeHistory::DialogeHistory() : MyGUI::Edit()
{
    mTextColor      = "#FFFFFF";
    mHeadingColor   = "#00FF00";
    mLinkColor      = "#FF0000";
    mOptionColor    = "#0000FF";
}

void DialogeHistory::initialiseOverride()
{
    //FIXME When updating myGUI remove call to this function from DialogueWindow and make it call base
    //Base::initialiseOverride();
    mClient->eventMouseButtonClick = MyGUI::newDelegate(this, &DialogeHistory::onHistoryClicked);
}

UString DialogeHistory::getColorAtPos(size_t _pos)
{
    UString colour = TextIterator::convertTagColour(mText->getTextColour());
    TextIterator iterator(mText->getCaption());
    while(iterator.moveNext())
    {
        size_t pos = iterator.getPosition();
        iterator.getTagColour(colour);
        if (pos < _pos)
            continue;
        else if (pos == _pos)
            break;
    }
    return colour;
}

UString DialogeHistory::getColorTextAt(size_t _pos)
{
    bool breakOnNext = false;
    UString colour = TextIterator::convertTagColour(mText->getTextColour());
    UString colour2 = colour;
    TextIterator iterator(mText->getCaption());
    TextIterator col_start = iterator;
    while(iterator.moveNext())
    {
        size_t pos = iterator.getPosition();
        iterator.getTagColour(colour);
        if(colour != colour2)
        {
            if(breakOnNext)
            {
                return getOnlyText().substr(col_start.getPosition(), iterator.getPosition()-col_start.getPosition());
            }
            col_start = iterator;
            colour2 = colour;
        }
        if (pos < _pos)
            continue;
        else if (pos == _pos)
        {
            breakOnNext = true;
        }
    }
    return "";
}


void DialogeHistory::onHistoryClicked(MyGUI::Widget* _sender)
{
    int option = -1;
    ISubWidgetText* t = getSubWidgetText();
    if(t == nullptr)
        return;

    const IntPoint& lastPressed = InputManager::getInstance().getLastLeftPressed();

    size_t cursorPosition = t->getCursorPosition(lastPressed);
    UString theColor = getColorAtPos(cursorPosition);

    //Clicked on a topic
    if(theColor == mLinkColor)
    {
        UString key = getColorTextAt(cursorPosition);
        eventTopicSelected(key.asUTF8(), option);
    }
    //Clicked on an option
    else if(theColor == mOptionColor)
    {
        UString key = getColorTextAt(cursorPosition);

        MWDialogue::DialogueMessage msg = mHistory.back();
        std::vector<std::string>::const_iterator i;
        int index = 0;
        for(i = msg.mOptions.begin(); i < msg.mOptions.end(); i++)
        {
            if(*i == key.asUTF8())
            {
                option = index;
                key = msg.mHeading;
            }
            index++;
        }

        //Remove the question
        mHistory.pop_back();
        update();

        eventTopicSelected(key.asUTF8(), option);
    }
}

void DialogeHistory::setTopics(std::vector<std::string>& parTopics)
{
    mTopics = parTopics;
    update();
}

void DialogeHistory::addDialogHeading(const UString& parText)
{
    UString head("\n");
    head.append(mHeadingColor);
    head.append(parText);
    head.append(mTextColor);
    head.append("\n");
    addText(head);
}

std::string::size_type DialogeHistory::replaceOnce(std::string& str, std::string what, std::string to, std::string::size_type start)
{
    std::string::size_type pos = str.find(what, start);
    if (pos != std::string::npos)
    {
        str.replace(pos,what.length(),to);
        return (pos+to.length());
    }
    return 0;
}

int DialogeHistory::replaceAll(std::string& str, std::string what, std::string to)
{
    int num = 0;
    std::string::size_type start = 0;
    while ((start = replaceOnce(str,what,to, start)) != 0)
    {
        num++;
        if(num > 100)
            break;
    }
    return num;
}

void DialogeHistory::addDialogText(const UString& parText)
{
    std::string tmp = parText.asUTF8();

    //Mark all topics in a different colour
    std::vector<std::string>::iterator i;
    for(i = mTopics.begin(); i < mTopics.end(); i++)
    {
        std::string word(*i);
        std::transform(word.begin(), word.begin()+1,word.begin(), ::toupper);

        std::stringstream ss;
        ss << mLinkColor << *i << mTextColor;
        replaceAll(tmp, *i, ss.str());

        std::stringstream ss2;
        ss2 << mLinkColor << word << mTextColor;
        replaceAll(tmp, word, ss2.str());
    }
    addText(tmp);
    addText("\n");
}

void DialogeHistory::addHistory(MWDialogue::DialogueMessage& parHistory)
{
    mHistory.push_back(parHistory);
    addHistoryText(parHistory);
}

void DialogeHistory::addHistoryText(const MWDialogue::DialogueMessage& parHistory)
{
    if(!parHistory.mHeading.empty())
        addDialogHeading(parHistory.mHeading);
    if(!parHistory.mText.empty())
        addDialogText(parHistory.mText);

    if(!parHistory.mOptions.empty())
    {
        std::vector<std::string>::const_iterator i;
        for(i = parHistory.mOptions.begin(); i < parHistory.mOptions.end(); i++)
        {
            std::stringstream ss;
            ss << "\n" << mOptionColor << *i << mTextColor << "\n";
            addText(ss.str());
        }
    }
}

void DialogeHistory::update()
{
    setCaption("");
    for(std::vector<MWDialogue::DialogueMessage>::const_iterator i = mHistory.begin(); i != mHistory.end(); i++)
    {
        addHistoryText(*i);
    }
}

void DialogeHistory::clear()
{
    mHistory.clear();
    setCaption("");
}
