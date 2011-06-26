
#include "dialoguemanager.hpp"

#include <cctype>
#include <algorithm>
#include <iterator>

#include <components/esm/loaddial.hpp>

#include <components/esm_store/store.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "../mwworld/refdata.hpp"
#include "../mwworld/player.hpp"

#include "../mwinput/inputmanager.hpp"

#include <iostream>

namespace
{
    std::string toLower (const std::string& name)
    {
        std::string lowerCase;

        std::transform (name.begin(), name.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);

        return lowerCase;
    }

    template<typename T1, typename T2>
    bool selectCompare (char comp, T1 value1, T2 value2)
    {
        switch (comp)
        {
            case '0': return value1==value2;
            case '1': return value1!=value2;
            case '2': return value1>value2;
            case '3': return value1>=value2;
            case '4': return value1<value2;
            case '5': return value1<=value2;
        }

        throw std::runtime_error ("unknown compare type in dialogue info select");
    }

    template<typename T>
    bool checkLocal (char comp, const std::string& name, T value, const MWWorld::Ptr& actor,
        const ESMS::ESMStore& store)
    {
        std::string scriptName = MWWorld::Class::get (actor).getScript (actor);

        if (scriptName.empty())
            return false; // no script

        const ESM::Script *script = store.scripts.find (scriptName);

        int i = 0;

        for (; i<static_cast<int> (script->varNames.size()); ++i)
            if (script->varNames[i]==name)
                break;

        if (i>=static_cast<int> (script->varNames.size()))
            return false; // script does not have a variable of this name

        const MWScript::Locals& locals = actor.getRefData().getLocals();

        if (i<script->data.numShorts)
            return selectCompare (comp, locals.mShorts[i], value);
        else
            i -= script->data.numShorts;

        if (i<script->data.numLongs)
            return selectCompare (comp, locals.mLongs[i], value);
        else
            i -= script->data.numShorts;

        return selectCompare (comp, locals.mFloats.at (i), value);
    }

    template<typename T>
    bool checkGlobal (char comp, const std::string& name, T value, MWWorld::World& world)
    {
        switch (world.getGlobalVariableType (name))
        {
            case 's':

                return selectCompare (comp, value, world.getGlobalVariable (name).mShort);

            case 'l':

                return selectCompare (comp, value, world.getGlobalVariable (name).mLong);

            case 'f':

                return selectCompare (comp, value, world.getGlobalVariable (name).mFloat);

            case ' ':

                world.getGlobalVariable (name); // trigger exception
                break;

            default:

                throw std::runtime_error ("unsupported gobal variable type");
        }

        return false;
    }
}

namespace MWDialogue
{
    bool DialogueManager::isMatching (const MWWorld::Ptr& actor,
        const ESM::DialInfo::SelectStruct& select) const
    {
        char type = select.selectRule[1];

        if (type!='0')
        {
            char comp = select.selectRule[4];
            std::string name = select.selectRule.substr (5);

            // TODO types 4, 5, 6, 7, 8, 9, A, B, C

            switch (type)
            {
                case '1': // function

                    return false; // TODO implement functions

                case '2': // global

                    if (select.type==ESM::VT_Short || select.type==ESM::VT_Int ||
                        select.type==ESM::VT_Long)
                    {
                        if (!checkGlobal (comp, toLower (name), select.i, *mEnvironment.mWorld))
                            return false;
                    }
                    else if (select.type==ESM::VT_Float)
                    {
                        if (!checkGlobal (comp, toLower (name), select.f, *mEnvironment.mWorld))
                            return false;
                    }
                    else
                        throw std::runtime_error (
                            "unsupported variable type in dialogue info select");

                    return true;

                case '3': // local

                    if (select.type==ESM::VT_Short || select.type==ESM::VT_Int ||
                        select.type==ESM::VT_Long)
                    {
                        if (!checkLocal (comp, toLower (name), select.i, actor,
                            mEnvironment.mWorld->getStore()))
                            return false;
                    }
                    else if (select.type==ESM::VT_Float)
                    {
                        if (!checkLocal (comp, toLower (name), select.f, actor,
                            mEnvironment.mWorld->getStore()))
                            return false;
                    }
                    else
                        throw std::runtime_error (
                            "unsupported variable type in dialogue info select");

                    return true;

                default:

                    std::cout << "unchecked select: " << type << " " << comp << " " << name << std::endl;
            }
        }

        return true;
    }

    bool DialogueManager::isMatching (const MWWorld::Ptr& actor, const ESM::DialInfo& info) const
    {
        // actor id
        if (!info.actor.empty())
            if (toLower (info.actor)!=MWWorld::Class::get (actor).getId (actor))
                return false;

        if (!info.race.empty())
        {
            ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *cellRef = actor.get<ESM::NPC>();

            if (!cellRef)
                return false;

            if (toLower (info.race)!=toLower (cellRef->base->race))
                return false;
        }

        if (!info.clas.empty())
        {
            ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *cellRef = actor.get<ESM::NPC>();

            if (!cellRef)
            {
                return false;
            }

            if (toLower (info.clas)!=toLower (cellRef->base->cls))
            {
                return false;
            }
        }

        if (!info.npcFaction.empty())
        {
            ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *cellRef = actor.get<ESM::NPC>();

            if (!cellRef)
                return false;

            if (toLower (info.npcFaction)!=toLower (cellRef->base->faction))
                return false;
        }

        // TODO check player faction

        // check cell
        if (!info.cell.empty())
        {
            if (mEnvironment.mWorld->getPlayer().getPlayer().getCell()->cell->name != info.cell)
            {
                return false;
            }
        }

        // TODO check DATAstruct

        for (std::vector<ESM::DialInfo::SelectStruct>::const_iterator iter (info.selects.begin());
            iter != info.selects.end(); ++iter)
            if (!isMatching (actor, *iter))
                return false;

        std::cout
            << "unchecked entries:" << std::endl
            << "    player faction: " << info.pcFaction << std::endl
            << "    disposition: " << info.data.disposition << std::endl
            << "    NPC rank: " << static_cast<int> (info.data.rank) << std::endl
            << "    gender: " << static_cast<int> (info.data.gender) << std::endl
            << "    PC rank: " << static_cast<int> (info.data.PCrank) << std::endl;

        return true;
    }

    DialogueManager::DialogueManager (MWWorld::Environment& environment) :
        mEnvironment (environment)
        , mActor()
        , mTopics()
    {
        mDiaInt = NULL;
    }

    void DialogueManager::updateTopics()
    {
        mTopics.clear();
        std::map<std::string,ESM::Dialogue>::const_iterator i = mEnvironment.mWorld->getStore().dialogs.list.begin();
        while(i != mEnvironment.mWorld->getStore().dialogs.list.end())
        {
            if(i->second.type == ESM::Dialogue::Topic)
            {
                bool include = false;
                for (std::vector<ESM::DialInfo>::const_iterator j = i->second.mInfo.begin(); j != i->second.mInfo.end(); ++j)
                {
                    if (isMatching(mActor, *j))
                    {
                        include = true;
                        break;
                    }
                }

                if(include)
                {
                    mTopics.push_back(i->first);
                }
            }
            i++;
        }
    }

    void DialogueManager::startDialogue (const MWWorld::Ptr& actor)
    {
        mActor = actor;

        std::string theName = MWWorld::Class::get(mActor).getName(mActor);
        int theDisposition = 100; //FIXME set this to proper dispisition

        std::cout << "talking with " << theName << std::endl;

        //Update the list of available topics
        updateTopics();

        //Say hello
        DialogueMessage theAnswer = getAnswer(std::string("hello"));

        if(mDiaInt)
            mDiaInt->startDialogue(theName, theDisposition, theAnswer, mTopics);
    }

    const DialogueMessage DialogueManager::getAnswer(const std::string& parTopic, int parCond)
    {
        DialogueMessage theAnswer;

        std::cout << "Saying '"<< parTopic << "' to " << MWWorld::Class::get(mActor).getName(mActor) << std::endl;
        const ESM::Dialogue *dialogue = mEnvironment.mWorld->getStore().dialogs.find(parTopic);

        for (std::vector<ESM::DialInfo>::const_iterator iter (dialogue->mInfo.begin()); iter!=dialogue->mInfo.end(); ++iter)
        {
            if (isMatching (mActor, *iter))
            {
                // start dialogue
                std::cout << "found matching info record" << std::endl;

                std::cout << "response: " << iter->response << std::endl;

                if (!iter->sound.empty())
                {
                    // TODO play sound
                }

                if (!iter->resultScript.empty())
                {
                    std::cout << "script: " << iter->resultScript << std::endl;
                    // TODO execute script
                }

                mEnvironment.mInputManager->setGuiMode(MWGui::GM_Dialogue);

                theAnswer.mHeading = parTopic;
                theAnswer.mText = iter->response;
                break;
            }
        }
        return theAnswer;
    }

    void DialogueManager::selectTopic(const std::string& parTopic, int parCond)
    {
        DialogueMessage theAnswer;
        theAnswer = getAnswer(parTopic, parCond);
        if(mDiaInt)
            mDiaInt->addAnswer(theAnswer);
    }

    void DialogueManager::endDialogue()
    {
        //mActor = NULL;
        mEnvironment.mInputManager->setGuiMode(MWGui::GM_Game);
    }
}
