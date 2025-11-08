// ==============================================================================
// ActionInitialization.cc - Action Initialization for Multi-Threading
// ==============================================================================

#include "ActionInitialization.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"

// External global variable for quiet mode
extern bool g_quietMode;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

ActionInitialization::ActionInitialization(bool generateCascades,
                                         SourceMode sourceMode,
                                         int nudexZA,
                                         const std::string& nudexLibDir)
: G4VUserActionInitialization(),
  fGenerateCascades(generateCascades),
  fSourceMode(sourceMode),
  fNuDEX_ZA(nudexZA),
  fNuDEXLibDir(nudexLibDir)
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

ActionInitialization::~ActionInitialization()
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void ActionInitialization::BuildForMaster() const
{
    // Master thread only creates RunAction for global run accumulation
    SetUserAction(new RunAction);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void ActionInitialization::Build() const
{
    // Primary generator
    PrimaryGeneratorAction* primaryGenerator =
        new PrimaryGeneratorAction(fGenerateCascades, fSourceMode);
    // Pass NuDEX configuration
    primaryGenerator->SetNuDEXConfig(fNuDEX_ZA, fNuDEXLibDir);

    // CASCADE mode removed

    SetUserAction(primaryGenerator);

    // Run action
    RunAction* runAction = new RunAction();
    SetUserAction(runAction);

    // Event action
    EventAction* eventAction = new EventAction(runAction);
    SetUserAction(eventAction);

    // Stepping action
    SteppingAction* steppingAction = new SteppingAction(eventAction);
    SetUserAction(steppingAction);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
