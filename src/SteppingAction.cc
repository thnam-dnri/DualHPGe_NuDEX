//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
// SteppingAction.cc - Enhanced for Angular Correlation & Cascade Analysis

#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"

#include "G4Step.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"
#include "G4Track.hh"
#include "G4StepPoint.hh"
#include "G4ParticleDefinition.hh"
#include "G4VProcess.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

// Quiet mode flag
extern bool g_quietMode;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SteppingAction::SteppingAction(EventAction* eventAction)
: G4UserSteppingAction(),
  fEventAction(eventAction),
  fScoringVolume1(0),
  fScoringVolume2(0)
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SteppingAction::~SteppingAction()
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    if (!fScoringVolume1 || !fScoringVolume2) { 
        const DetectorConstruction* detectorConstruction
            = static_cast<const DetectorConstruction*>
                (G4RunManager::GetRunManager()->GetUserDetectorConstruction());
        fScoringVolume1 = detectorConstruction->GetScoringVolume1();   
        fScoringVolume2 = detectorConstruction->GetScoringVolume2();   
    }

    // Get volume of the current step
    G4LogicalVolume* volume 
        = step->GetPreStepPoint()->GetTouchableHandle()
            ->GetVolume()->GetLogicalVolume();

    // Track gamma energies for first 20 events
    static int gammaEventCounter = 0;
    static int gammaPrintCount = 0;

    G4Track* track = step->GetTrack();
    G4String particleName = track->GetDefinition()->GetParticleName();

    // Print gamma energies for first 20 events (disabled in quiet mode)
    if (!g_quietMode && particleName == "gamma" && gammaEventCounter < 20 && gammaPrintCount < 20) {
        G4double gammaEnergy = track->GetKineticEnergy();
        G4cout << "Gamma event " << gammaEventCounter << ": E=" << gammaEnergy/keV << " keV" << G4endl;
        gammaPrintCount++;
        if (gammaPrintCount >= 20) {
            gammaEventCounter++;
            gammaPrintCount = 0;
        }
    }

    // Get energy deposit
    G4double edepStep = step->GetTotalEnergyDeposit();

    // Skip if no energy deposited
    if (edepStep <= 0.) return;
    
    // Determine which detector and add energy
    if (volume == fScoringVolume1) {
        fEventAction->AddEnergyDeposit(edepStep, 1);
        
    } else if (volume == fScoringVolume2) {
        fEventAction->AddEnergyDeposit(edepStep, 2);
        
    }
}
