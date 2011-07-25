#include <components/esm_store/store.hpp>

#ifndef MWGUI_CLASS_QUESTIONS_H
#define MWGUI_CLASS_QUESTIONS_H

namespace MWGui
{
    struct ClassPoint
    {
        const char *id;
        // Specialization points to match, in order: Stealth, Combat, Magic
        // Note: Order is taken from http://www.uesp.net/wiki/Morrowind:Class_Quiz
        unsigned int points[3];
    };

    struct Step
    {
        const char* text;
        const char* buttons[3];
        // The specialization for each answer
        ESM::Class::Specialization specializations[3];
    };


    class ClassQuestions
    {
        private:
            unsigned generateClassSpecializations[3];
            unsigned generateClassStep;
        public:
            void begin();
            bool isDone();
            void next(ESM::Class::Specialization parSpecialization);
            const Step* getStep();
            const Step* getStep(int parIndex);
            std::string getGeneratedClass();
    };
}

#endif
