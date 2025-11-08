// ==============================================================================
// ActionInitialization.hh - Action Initialization for Multi-Threading
// ==============================================================================

#ifndef ActionInitialization_h
#define ActionInitialization_h 1

#include "G4VUserActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "globals.hh"
#include <string>

class ActionInitialization : public G4VUserActionInitialization
{
public:
    ActionInitialization(bool generateCascades = true,
                        SourceMode sourceMode = (SourceMode)0,
                        int nudexZA = 17035,
                        const std::string& nudexLibDir = "../NuDEX/NuDEXlib/");
    virtual ~ActionInitialization();

    virtual void BuildForMaster() const;
    virtual void Build() const;

private:
    bool fGenerateCascades;
    SourceMode fSourceMode;
    int fNuDEX_ZA;
    std::string fNuDEXLibDir;
};

#endif
