//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
// EventAction.cc - Simple Total Energy per Detector Approach

#include "EventAction.hh"
#include "RunAction.hh"

#include "G4Event.hh"
#include "Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4PhysicalConstants.hh"
#include <algorithm>
#include <cmath>

// External global variable for quiet mode
extern bool g_quietMode;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

EventAction::EventAction(RunAction* runAction)
: G4UserEventAction(),
  fRunAction(runAction),
  fEnergyDepositDet1(0.),
  fEnergyDepositDet2(0.),
  fCoincidenceWindow(20.0*ns),
  fMinimumEnergy(0.010*MeV)  // 10 keV threshold per detector
{
    if (!g_quietMode) {
        G4cout << "EventAction: Coincidence window = " << fCoincidenceWindow/ns 
               << " ns, threshold = " << fMinimumEnergy/keV << " keV" << G4endl;
    }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

EventAction::~EventAction()
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void EventAction::BeginOfEventAction(const G4Event*)
{
    fEnergyDepositDet1 = 0.;
    fEnergyDepositDet2 = 0.;
    fCoincidences.clear();
    
    // Clear collections if needed for other analysis
    fAllHits.clear();
    fHitsDet1.clear();
    fHitsDet2.clear();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void EventAction::EndOfEventAction(const G4Event* event)
{
    static int eventCounter = 0;
    bool debugThis = (!g_quietMode && eventCounter < 10);
    
    if (debugThis) {
        G4cout << "Event " << event->GetEventID() << ": Det1=" << fEnergyDepositDet1/keV 
               << " keV, Det2=" << fEnergyDepositDet2/keV << " keV" << G4endl;
    }
    
    // Apply energy thresholds
    bool det1Hit = (fEnergyDepositDet1 >= fMinimumEnergy);
    bool det2Hit = (fEnergyDepositDet2 >= fMinimumEnergy);
    
    
    // Save all detector hits to ROOT using G4AnalysisManager
    if (det1Hit || det2Hit) {
        auto analysisManager = G4AnalysisManager::Instance();
        // Convert energies from MeV to keV
        analysisManager->FillNtupleDColumn(0, fEnergyDepositDet1 / keV);
        analysisManager->FillNtupleDColumn(1, fEnergyDepositDet2 / keV);
        analysisManager->AddNtupleRow();
    }

    // Update Run class
    Run* currentRun = static_cast<Run*>(G4RunManager::GetRunManager()->GetNonConstCurrentRun());
    if (currentRun) {
        // Add single detector spectra
        if (det1Hit) {
            currentRun->AddEnergySpectrumDet1(fEnergyDepositDet1);
        }
        if (det2Hit) {
            currentRun->AddEnergySpectrumDet2(fEnergyDepositDet2);
        }
    }
    
    // Maintain compatibility with RunAction
    fRunAction->AddEnergyDepositDet1(fEnergyDepositDet1);
    fRunAction->AddEnergyDepositDet2(fEnergyDepositDet2);
    
    eventCounter++;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void EventAction::AddEnergyDeposit(G4double energy, G4int detectorID)
{
    // Simple energy accumulation per detector (like original code)
    if (detectorID == 1) {
        fEnergyDepositDet1 += energy;
    } else if (detectorID == 2) {
        fEnergyDepositDet2 += energy;
    }
}

