#include "window_manager.hpp"
#include "layouts.hpp"
#include "text_input.hpp"
#include "race.hpp"
#include "class.hpp"
#include "birth.hpp"
#include "review.hpp"
#include "dialogue.hpp"
#include "dialogue_history.hpp"
#include "stats_window.hpp"
#include "messagebox.hpp"
#include "class_questions.hpp"

#include "../mwmechanics/mechanicsmanager.hpp"
#include "../mwinput/inputmanager.hpp"

#include "console.hpp"

#include <assert.h>
#include <iostream>
#include <iterator>

using namespace MWGui;

WindowManager::WindowManager(MyGUI::Gui *_gui, MWWorld::Environment& environment,
    const Compiler::Extensions& extensions, bool fpsSwitch, bool newGame)
  : environment(environment)
  , gui(_gui)
  , mode(GM_Game)
  , nextMode(GM_Game)
  , needModeChange(false)
  , shown(GW_ALL)
  , allowed(newGame ? GW_None : GW_ALL)
{
    showFPSCounter = fpsSwitch;

    creationStage = NotStarted;

    //Register own widgets with MyGUI
    MyGUI::FactoryManager::getInstance().registerFactory<DialogeHistory>("Widget");

    // Get size info from the Gui object
    assert(gui);
    int w = gui->getViewSize().width;
    int h = gui->getViewSize().height;

    hud = new HUD(w,h, showFPSCounter);
    menu = new MainMenu(w,h);
    map = new MapWindow();
    stats = new StatsWindow(*this);
    mClassQuestions = new ClassQuestions();
#if 0
    inventory = new InventoryWindow ();
#endif
    console = new Console(w,h, environment, extensions);
    mMessageBoxManager = new MessageBoxManager(this);

    // The HUD is always on
    hud->setVisible(true);

    // Setup player stats
    for (int i = 0; i < ESM::Attribute::Length; ++i)
    {
        playerAttributes.insert(std::make_pair(ESM::Attribute::attributeIds[i], MWMechanics::Stat<int>()));
    }

    for (int i = 0; i < ESM::Skill::Length; ++i)
    {
        playerSkillValues.insert(std::make_pair(ESM::Skill::skillIds[i], MWMechanics::Stat<float>()));
    }

    // Set up visibility
    updateVisible();
}

WindowManager::~WindowManager()
{
    delete console;
    delete mMessageBoxManager;
    delete hud;
    delete map;
    delete menu;
    delete stats;
#if 0
    delete inventory;
#endif

    delete mClassQuestions;

    for (WindowMap::iterator it = mWindows.begin(); it != mWindows.end(); ++it)
    {
        delete it->second;
    }
}

void WindowManager::update()
{
    if (needModeChange)
    {
        needModeChange = false;
        environment.mInputManager->setGuiMode(nextMode);
        nextMode = GM_Game;
    }
    if (showFPSCounter)
    {
        hud->setFPS(mFPS);
    }
}

void WindowManager::setNextMode(GuiMode newMode)
{
    nextMode = newMode;
    needModeChange = true;
}

void WindowManager::setGuiMode(GuiMode newMode)
{
    environment.mInputManager->setGuiMode(newMode);
}

void WindowManager::updateVisible()
{
    // Start out by hiding everything except the HUD
    map->setVisible(false);
    menu->setVisible(false);
    stats->setVisible(false);
#if 0
    inventory->setVisible(false);
#endif
    console->disable();

    // Mouse is visible whenever we're not in game mode
    gui->setVisiblePointer(isGuiMode());

    // If in game mode, don't show anything.
    if(mode == GM_Game)
    {
        return;
    }

    if(mode == GM_MainMenu)
    {
        // Enable the main menu
        menu->setVisible(true);
        return;
    }

    if(mode == GM_Console)
    {
        console->enable();
        return;
    }

    if (mode == GM_Name)
    {
        TextInputDialog* nameDialog = static_cast<TextInputDialog*>(getWindow("nameDialog"));

        if(nameDialog)
        {
            removeWindow(nameDialog);
            nameDialog = NULL;
        }

        nameDialog = new TextInputDialog(*this);
        std::string sName = getGameSettingString("sName", "Name");
        nameDialog->setTextLabel(sName);
        nameDialog->setTextInput(playerName);
        nameDialog->setNextButtonShow(creationStage >= NameChosen);
        nameDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onDialogDone);
        nameDialog->eventSaveName = MyGUI::newDelegate(environment.mMechanicsManager, &MWMechanics::MechanicsManager::setPlayerName);
        nameDialog->open();
        addWindow("nameDialog", nameDialog);
        return;
    }

    if (mode == GM_Race)
    {
        RaceDialog* raceDialog = static_cast<RaceDialog*>(getWindow("raceDialog"));
        if (raceDialog)
        {
            removeWindow(raceDialog);
            raceDialog = NULL;
        }
        raceDialog = new RaceDialog(*this);
        raceDialog->setNextButtonShow(creationStage >= RaceChosen);
        raceDialog->setRaceId(playerRaceId);
        raceDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onDialogDone);
        raceDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onDialogBack);
        raceDialog->eventSave = MyGUI::newDelegate(environment.mMechanicsManager, &MWMechanics::MechanicsManager::setPlayerRace);
        raceDialog->open();
        addWindow("raceDialog", raceDialog);
        return;
    }

    if (mode == GM_Class)
    {
        ClassChoiceDialog* classChoiceDialog = static_cast<ClassChoiceDialog*>(getWindow("classChoiceDialog"));
        if (classChoiceDialog)
            removeWindow(classChoiceDialog);
        classChoiceDialog = new ClassChoiceDialog(*this);
        classChoiceDialog->eventButtonSelected = MyGUI::newDelegate(this, &WindowManager::onClassChoice);
        classChoiceDialog->open();
        addWindow("classChoiceDialog", classChoiceDialog);
        return;
    }

    if (mode == GM_ClassGenerate)
    {
        mClassQuestions->begin();
        generateClass = "";
        showClassQuestionDialog();
        return;
    }

    if (mode == GM_ClassPick)
    {
        PickClassDialog* pickClassDialog = static_cast<PickClassDialog*>(getWindow("pickClassDialog"));
        if (pickClassDialog)
            removeWindow(pickClassDialog);
        pickClassDialog = new PickClassDialog(*this);
        pickClassDialog->setNextButtonShow(creationStage >= ClassChosen);
        pickClassDialog->setClassId(playerClass.name);
        pickClassDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onPickClassDialogDone);
        pickClassDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onDialogBack);
        pickClassDialog->open();
        addWindow("pickClassDialog", pickClassDialog);
        return;
    }

    if (mode == GM_ClassCreate)
    {
        CreateClassDialog* createClassDialog = static_cast<CreateClassDialog*>(getWindow("createClassDialog"));
        if (createClassDialog)
            removeWindow(createClassDialog);
        createClassDialog = new CreateClassDialog(*this);
        createClassDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onCreateClassDialogDone);
        createClassDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onDialogBack);
        createClassDialog->open();
        addWindow("createClassDialog", createClassDialog);
        return;
    }

    if (mode == GM_Birth)
    {
        BirthDialog* birthSignDialog = static_cast<BirthDialog*>(getWindow("birthSignDialog"));
        if (birthSignDialog)
            removeWindow(birthSignDialog);
        birthSignDialog = new BirthDialog(*this);
        birthSignDialog->setNextButtonShow(creationStage >= BirthSignChosen);
        birthSignDialog->setBirthId(playerBirthSignId);
        birthSignDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onBirthSignDialogDone);
        birthSignDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onDialogBack);
        birthSignDialog->open();
        addWindow("birthSignDialog", birthSignDialog);
        return;
    }

    if (mode == GM_Review)
    {
        ReviewDialog* reviewDialog = static_cast<ReviewDialog*>(getWindow("reviewDialog"));
        if (reviewDialog)
            removeWindow(reviewDialog);
        reviewDialog = new ReviewDialog(*this);
        reviewDialog->setPlayerName(playerName);
        reviewDialog->setRace(playerRaceId);
        reviewDialog->setClass(playerClass);
        reviewDialog->setBirthSign(playerBirthSignId);

        reviewDialog->setHealth(playerHealth);
        reviewDialog->setMagicka(playerMagicka);
        reviewDialog->setFatigue(playerFatigue);

        {
            std::map<ESM::Attribute::AttributeID, MWMechanics::Stat<int> >::iterator end = playerAttributes.end();
            for (std::map<ESM::Attribute::AttributeID, MWMechanics::Stat<int> >::iterator it = playerAttributes.begin(); it != end; ++it)
            {
                reviewDialog->setAttribute(it->first, it->second);
            }
        }

        {
            std::map<ESM::Skill::SkillEnum, MWMechanics::Stat<float> >::iterator end = playerSkillValues.end();
            for (std::map<ESM::Skill::SkillEnum, MWMechanics::Stat<float> >::iterator it = playerSkillValues.begin(); it != end; ++it)
            {
                reviewDialog->setSkillValue(it->first, it->second);
            }
            reviewDialog->configureSkills(playerMajorSkills, playerMinorSkills);
        }

        reviewDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onReviewDialogDone);
        reviewDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onDialogBack);
        reviewDialog->eventActivateDialog = MyGUI::newDelegate(this, &WindowManager::onReviewActivateDialog);
        reviewDialog->open();
        addWindow("reviewDialog", reviewDialog);
        return;
    }

    if(mode == GM_Inventory)
    {
        // Ah, inventory mode. First, compute the effective set of
        // windows to show. This is controlled both by what windows the
        // user has opened/closed (the 'shown' variable) and by what
        // windows we are allowed to show (the 'allowed' var.)
        int eff = shown & allowed;

        // Show the windows we want
        map   -> setVisible( (eff & GW_Map) != 0 );
        stats -> setVisible( (eff & GW_Stats) != 0 );
#if 0
        //      inventory -> setVisible( eff & GW_Inventory );
#endif
        return;
    }

    if (mode == GM_Dialogue)
    {
        DialogueWindow* dialogueWindow = static_cast<DialogueWindow*>(getWindow("dialogueWindow"));
        if (!dialogueWindow)
        {
            dialogueWindow = new DialogueWindow(*this);
            dialogueWindow->eventBye = MyGUI::newDelegate(this, &WindowManager::onDialogueWindowBye);
            addWindow("dialogueWindow", dialogueWindow);
        }
        dialogueWindow->open();
        return;
    }
    
    if(mode == GM_InterMessageBox)
    {
        if(!mMessageBoxManager->isInteractiveMessageBox()) {
            setGuiMode(GM_Game);
        }
        return;
    }


    // Unsupported mode, switch back to game
    // Note: The call will eventually end up this method again but
    // will stop at the check if(mode == GM_Game) above.
    setGuiMode(GM_Game);
}

void WindowManager::setValue (const std::string& id, const MWMechanics::Stat<int>& value)
{
    stats->setValue (id, value);

    static const char *ids[] =
    {
        "AttribVal1", "AttribVal2", "AttribVal3", "AttribVal4", "AttribVal5",
        "AttribVal6", "AttribVal7", "AttribVal8"
    };
    static ESM::Attribute::AttributeID attributes[] =
    {
        ESM::Attribute::Strength,
        ESM::Attribute::Intelligence,
        ESM::Attribute::Willpower,
        ESM::Attribute::Agility,
        ESM::Attribute::Speed,
        ESM::Attribute::Endurance,
        ESM::Attribute::Personality,
        ESM::Attribute::Luck
    };
    for (size_t i = 0; i < sizeof(ids)/sizeof(ids[0]); ++i)
    {
        if (id != ids[i])
            continue;
        playerAttributes[attributes[i]] = value;
        break;
    }
}


void WindowManager::setValue(const ESM::Skill::SkillEnum parSkill, const MWMechanics::Stat<float>& value)
{
    stats->setValue(parSkill, value);
    playerSkillValues[parSkill] = value;
}

void WindowManager::setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value)
{
    stats->setValue (id, value);
    hud->setValue (id, value);
    if (id == "HBar")
        playerHealth = value;
    else if (id == "MBar")
        playerMagicka = value;
    else if (id == "FBar")
        playerFatigue = value;
}

void WindowManager::setValue (const std::string& id, const std::string& value)
{
    stats->setValue (id, value);
    if (id=="name")
        playerName = value;
    else if (id=="race")
        playerRaceId = value;
}

void WindowManager::setValue (const std::string& id, int value)
{
    stats->setValue (id, value);
}

void WindowManager::setPlayerClass (const ESM::Class &class_)
{
    playerClass = class_;
    stats->setValue("class", playerClass.name);
}

void WindowManager::configureSkills (const SkillList& major, const SkillList& minor)
{
    stats->configureSkills (major, minor);
    playerMajorSkills = major;
    playerMinorSkills = minor;
}

void WindowManager::setFactions (const FactionList& factions)
{
    stats->setFactions (factions);
}

void WindowManager::setBirthSign (const std::string &signId)
{
    stats->setBirthSign (signId);
    playerBirthSignId = signId;
}

void WindowManager::setReputation (int reputation)
{
    stats->setReputation (reputation);
}

void WindowManager::setBounty (int bounty)
{
    stats->setBounty (bounty);
}

void WindowManager::updateSkillArea()
{
    stats->updateSkillArea();
}


WindowBase* WindowManager::getWindow(const std::string& parName)
{
    WindowMap::iterator it;
    it = mWindows.find(parName);
    if(it != mWindows.end())
    {
        return it->second;
    }
    return NULL;
}

void WindowManager::addWindow(const std::string& parName, WindowBase* parWindow)
{
    mWindows.insert(WindowMap::value_type(parName, parWindow));
}

void WindowManager::removeWindow(WindowBase* parWindow)
{
    assert(parWindow);
    parWindow->setVisible(false);
    for (WindowMap::iterator it = mWindows.begin(); it != mWindows.end(); ++it)
    {
        if(it->second == parWindow)
        {
            delete it->second;
            mWindows.erase(it);
            return;
        }
    }

    delete parWindow;
}

const std::string WindowManager::getWindowName(WindowBase* parWindow)
{
    assert(parWindow);
    for (WindowMap::iterator it = mWindows.begin(); it != mWindows.end(); ++it)
    {
        if(it->second == parWindow)
            return it->first;
    }
    return "";
}

void WindowManager::messageBox (const std::string& message, const std::vector<std::string>& buttons)
{
    if (buttons.empty())
    {
        mMessageBoxManager->createMessageBox(message);
    }
    else
    {
        mMessageBoxManager->createInteractiveMessageBox(message, buttons);
        setGuiMode(GM_InterMessageBox);
    }
}

int WindowManager::readPressedButton ()
{
    return mMessageBoxManager->readPressedButton();
}

const std::string &WindowManager::getGameSettingString(const std::string &id, const std::string &default_)
{
    const ESM::GameSetting *setting = environment.mWorld->getStore().gameSettings.search(id);
    if (setting && setting->type == ESM::VT_String)
        return setting->str;
    return default_;
}

void WindowManager::onDialogDone(WindowBase* parWindow)
{
    std::string theWindowName = getWindowName(parWindow);

    if(theWindowName.compare("nameDialog") == 0)
    {
        // Go to next dialog if name was previously chosen
        if (creationStage == ReviewNext)
            setGuiMode(GM_Review);
        else if (creationStage >= NameChosen)
            setGuiMode(GM_Race);
        else
        {
            creationStage = NameChosen;
            setGuiMode(GM_Game);
        }
    }
    else if(theWindowName.compare("raceDialog") == 0)
    {
        // Go to next dialog if race was previously chosen
        if (creationStage == ReviewNext)
            setGuiMode(GM_Review);
        else if(creationStage >= RaceChosen)
            setGuiMode(GM_Class);
        else
        {
            creationStage = RaceChosen;
            setGuiMode(GM_Game);
        }
    }
    else if(theWindowName.compare("generateClassResultDialog") == 0)
    {
        environment.mMechanicsManager->setPlayerClass(generateClass);

        // Go to next dialog if class was previously chosen
        if (creationStage == ReviewNext)
            setGuiMode(GM_Review);
        else if (creationStage >= ClassChosen)
            setGuiMode(GM_Birth);
        else
        {
            creationStage = ClassChosen;
            setGuiMode(GM_Game);
        }
    }

    //Remove the window
    removeWindow(parWindow);
}

void WindowManager::onDialogueWindowBye()
{
    DialogueWindow* dialogueWindow = static_cast<DialogueWindow*>(getWindow("dialogueWindow"));
    if (dialogueWindow)
    {
        //FIXME set some state and stuff?
        removeWindow(dialogueWindow);
    }
    setGuiMode(GM_Game);
}

void WindowManager::onDialogBack(WindowBase* parWindow)
{
    std::map<std::string, GuiMode> theNextMode;
    theNextMode["raceDialog"]           =GM_Name;
    theNextMode["createClassDialog"]    = GM_Class;
    theNextMode["reviewDialog"]         = GM_Birth;
    theNextMode["generateClassResultDialog"] = GM_Class;
    theNextMode["pickClassDialog"]      = GM_Class;
    theNextMode["birthSignDialog"]      = GM_Class;

    std::string theWindowName = getWindowName(parWindow);
    if(theWindowName.compare("generateClassResultDialog") == 0)
    {
        if(creationStage < ClassChosen)
            creationStage = ClassChosen;
        environment.mMechanicsManager->setPlayerClass(generateClass);
    }
    else if(theWindowName.compare("pickClassDialog") == 0)
    {
        PickClassDialog* pickClassDialog = static_cast<PickClassDialog*>(parWindow);
        const std::string classId = pickClassDialog->getClassId();
        if (!classId.empty())
            environment.mMechanicsManager->setPlayerClass(classId);
    }
    else if(theWindowName.compare("birthSignDialog") == 0)
    {
        BirthDialog* birthSignDialog = static_cast<BirthDialog*>(parWindow);
        environment.mMechanicsManager->setPlayerBirthsign(birthSignDialog->getBirthId());
    }

    removeWindow(parWindow);
    setGuiMode(theNextMode[theWindowName]);
}

void WindowManager::onClassChoice(int _index)
{
    WindowBase* classChoiceDialog = getWindow("classChoiceDialog");
    if (classChoiceDialog)
    {
        removeWindow(classChoiceDialog);
    }

    switch(_index)
    {
        case ClassChoiceDialog::Class_Generate:
            setGuiMode(GM_ClassGenerate);
            break;
        case ClassChoiceDialog::Class_Pick:
            setGuiMode(GM_ClassPick);
            break;
        case ClassChoiceDialog::Class_Create:
            setGuiMode(GM_ClassCreate);
            break;
        case ClassChoiceDialog::Class_Back:
            setGuiMode(GM_Race);
            break;
    };
}

void WindowManager::onFrame (float frameDuration)
{
    mMessageBoxManager->onFrame(frameDuration);
}

void WindowManager::showClassQuestionDialog()
{
    if (mClassQuestions->isDone())
    {
        generateClass = mClassQuestions->getGeneratedClass();

        GenerateClassResultDialog* generateClassResultDialog = static_cast<GenerateClassResultDialog*>(getWindow("generateClassResultDialog"));
        if (generateClassResultDialog)
            removeWindow(generateClassResultDialog);
        generateClassResultDialog = new GenerateClassResultDialog(*this);
        generateClassResultDialog->setClassId(generateClass);
        generateClassResultDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onDialogBack);
        generateClassResultDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onDialogDone);
        generateClassResultDialog->open();
        addWindow("generateClassResultDialog", generateClassResultDialog);
        return;
    }
    else
    {
        InfoBoxDialog* generateClassQuestionDialog = static_cast<InfoBoxDialog*>(getWindow("generateClassQuestionDialog"));
        if (generateClassQuestionDialog)
            removeWindow(generateClassQuestionDialog);
        generateClassQuestionDialog = new InfoBoxDialog(*this);

        InfoBoxDialog::ButtonList buttons;
        const struct Step* theStep = mClassQuestions->getStep();
        generateClassQuestionDialog->setText(theStep->text);
        buttons.push_back(theStep->buttons[0]);
        buttons.push_back(theStep->buttons[1]);
        buttons.push_back(theStep->buttons[2]);
        generateClassQuestionDialog->setButtons(buttons);
        generateClassQuestionDialog->eventButtonSelected = MyGUI::newDelegate(this, &WindowManager::onClassQuestionChosen);
        generateClassQuestionDialog->open();
        addWindow("generateClassQuestionDialog", generateClassQuestionDialog);
    }
}

void WindowManager::onClassQuestionChosen(int _index)
{
    WindowBase* generateClassQuestionDialog = getWindow("generateClassQuestionDialog");
    if (generateClassQuestionDialog)
        removeWindow(generateClassQuestionDialog);
    if (_index < 0 || _index >= 3)
    {
        setGuiMode(GM_Class);
        return;
    }

    ESM::Class::Specialization specialization = mClassQuestions->getStep()->specializations[_index];
    mClassQuestions->next(specialization);
    showClassQuestionDialog();
}

void WindowManager::onPickClassDialogDone(WindowBase* parWindow)
{
    PickClassDialog* pickClassDialog = static_cast<PickClassDialog*>(parWindow);
    if (pickClassDialog)
    {
        const std::string &classId = pickClassDialog->getClassId();
        if (!classId.empty())
            environment.mMechanicsManager->setPlayerClass(classId);
        const ESM::Class *klass = environment.mWorld->getStore().classes.find(classId);
        if (klass)
            playerClass = *klass;
        removeWindow(pickClassDialog);
    }

    // Go to next dialog if class was previously chosen
    if (creationStage == ReviewNext)
        setGuiMode(GM_Review);
    else if (creationStage >= ClassChosen)
        setGuiMode(GM_Birth);
    else
    {
        creationStage = ClassChosen;
        setGuiMode(GM_Game);
    }
}

void WindowManager::onCreateClassDialogDone(WindowBase* parWindow)
{
    CreateClassDialog* createClassDialog = static_cast<CreateClassDialog*>(getWindow("createClassDialog"));
    if (createClassDialog)
    {
        ESM::Class klass;
        klass.name = createClassDialog->getName();
        klass.description = createClassDialog->getDescription();
        klass.data.specialization = createClassDialog->getSpecializationId();
        klass.data.isPlayable = 0x1;

        std::vector<int> attributes = createClassDialog->getFavoriteAttributes();
        assert(attributes.size() == 2);
        klass.data.attribute[0] = attributes[0];
        klass.data.attribute[1] = attributes[1];

        std::vector<ESM::Skill::SkillEnum> majorSkills = createClassDialog->getMajorSkills();
        std::vector<ESM::Skill::SkillEnum> minorSkills = createClassDialog->getMinorSkills();
        assert(majorSkills.size() >= sizeof(klass.data.skills)/sizeof(klass.data.skills[0]));
        assert(minorSkills.size() >= sizeof(klass.data.skills)/sizeof(klass.data.skills[0]));
        for (size_t i = 0; i < sizeof(klass.data.skills)/sizeof(klass.data.skills[0]); ++i)
        {
            klass.data.skills[i][1] = majorSkills[i];
            klass.data.skills[i][0] = minorSkills[i];
        }
        environment.mMechanicsManager->setPlayerClass(klass);
        playerClass = klass;

        removeWindow(createClassDialog);
    }

    // Go to next dialog if class was previously chosen
    if (creationStage == ReviewNext)
        setGuiMode(GM_Review);
    else if (creationStage >= ClassChosen)
        setGuiMode(GM_Birth);
    else
    {
        creationStage = ClassChosen;
        setGuiMode(GM_Game);
    }
}

void WindowManager::onBirthSignDialogDone(WindowBase* parWindow)
{
    BirthDialog* birthSignDialog = static_cast<BirthDialog*>(parWindow);
    if (birthSignDialog)
    {
        playerBirthSignId = birthSignDialog->getBirthId();
        if (!playerBirthSignId.empty())
            environment.mMechanicsManager->setPlayerBirthsign(playerBirthSignId);
        removeWindow(birthSignDialog);
    }

    // Go to next dialog if birth sign was previously chosen
    if (creationStage >= BirthSignChosen)
        setGuiMode(GM_Review);
    else
    {
        creationStage = BirthSignChosen;
        setGuiMode(GM_Game);
    }
}

void WindowManager::onReviewDialogDone(WindowBase* parWindow)
{
    removeWindow(parWindow);
    setGuiMode(GM_Game);
}

void WindowManager::onReviewActivateDialog(int parDialog)
{
    WindowBase* reviewDialog = getWindow("reviewDialog");
    if (reviewDialog)
        removeWindow(reviewDialog);
    creationStage = ReviewNext;

    switch(parDialog)
    {
        case ReviewDialog::NAME_DIALOG:
            setGuiMode(GM_Name);
            break;
        case ReviewDialog::RACE_DIALOG:
            setGuiMode(GM_Race);
            break;
        case ReviewDialog::CLASS_DIALOG:
            setGuiMode(GM_Class);
            break;
        case ReviewDialog::BIRTHSIGN_DIALOG:
            setGuiMode(GM_Birth);
    };
}

const ESMS::ESMStore& WindowManager::getStore() const
{
    return environment.mWorld->getStore();
}
