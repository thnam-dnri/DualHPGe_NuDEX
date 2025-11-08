// ==============================================================================
// RunAction.hh/cc - Phase 1 Run-level analysis for dual detectors
// ==============================================================================

#ifndef RunAction_h
#define RunAction_h 1

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "G4AnalysisManager.hh"
#include "globals.hh"

class G4Run;

class RunAction : public G4UserRunAction
{
public:
    RunAction();
    virtual ~RunAction();

    virtual G4Run* GenerateRun();
    virtual void BeginOfRunAction(const G4Run*);
    virtual void   EndOfRunAction(const G4Run*);

    void AddEnergyDepositDet1(G4double edep);
    void AddEnergyDepositDet2(G4double edep);

private:
    G4Accumulable<G4double> fEnergyDepositDet1;
    G4Accumulable<G4double> fEnergyDepositDet2;
    G4Accumulable<G4int> fEventCountDet1;
    G4Accumulable<G4int> fEventCountDet2;
};
#endif