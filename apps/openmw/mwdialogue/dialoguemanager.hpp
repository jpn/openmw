#ifndef GAME_MWDIALOG_DIALOGUEMANAGER_H
#define GAME_MWDIALOG_DIALOGUEMANAGER_H

#include <components/esm/loadinfo.hpp>

#include "../mwworld/ptr.hpp"

namespace MWWorld
{
    class Environment;
}

namespace MWDialogue
{
    typedef struct
    {
        std::string mHeading;
        std::string mText;
        std::vector<std::string> mOptions;
    } DialogueMessage;

    class DialogueInterface
    {
        public:
            virtual void startDialogue(std::string parName, int parDisposition, DialogueMessage& parAnswer, std::vector<std::string>& parTopics) = 0;
            virtual void setDisposition(int parDisposition) = 0;
            virtual void addAnswer(DialogueMessage& parAnswer) = 0;
            virtual void setTopics(std::vector<std::string>& parTopics) = 0;
    };

    class DialogueManager
    {
        private:
            MWWorld::Environment& mEnvironment;
            MWWorld::Ptr   mActor;
            DialogueInterface* mDiaInt;
            std::vector<std::string>    mTopics;

            bool isMatching (const MWWorld::Ptr& actor, const ESM::DialInfo::SelectStruct& select) const;
            bool isMatching (const MWWorld::Ptr& actor, const ESM::DialInfo& info) const;
            void updateTopics();
            const DialogueMessage getAnswer(const std::string& parTopic, int parCond = -1);

        public:
            DialogueManager (MWWorld::Environment& environment);
            void startDialogue (const MWWorld::Ptr& actor);
            void endDialogue();
            void setUi(DialogueInterface* parDi) { mDiaInt = parDi; }
            void selectTopic(const std::string& parTopic, int parCond = -1);

    };
}

#endif
