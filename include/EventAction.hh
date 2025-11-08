// ==============================================================================
// EventAction.hh - Enhanced for Angular Correlation & Cascade Analysis
// ==============================================================================

#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"
#include <vector>

class RunAction;

// Structure for individual gamma hits with full information
struct GammaHit {
    G4double energy;           // Energy deposited (MeV)
    G4double time;            // Global time (ns)
    G4int detectorID;         // 1 or 2
    G4ThreeVector position;   // Hit position in detector
    G4int trackID;           // Track ID for cascade tracking
    G4int parentID;          // Parent track ID
    G4String particleName;   // Particle type
    G4String processName;    // Creation process
    G4ThreeVector momentum;  // Initial momentum direction
};

// Structure for coincidence events
struct CoincidenceEvent {
    GammaHit hit1;           // First detector hit
    GammaHit hit2;           // Second detector hit
    G4double timeDifference; // Time difference (ns)
    G4double angleCorrelation; // Angle between momentum vectors
    G4double energySum;      // Sum energy
    G4bool isTrue;          // True coincidence vs random
};

// Structure for cascade sequence tracking
struct CascadeSequence {
    std::vector<GammaHit> sequence;
    G4int primaryTrackID;
    G4double totalTime;
    G4double totalEnergy;
};

class EventAction : public G4UserEventAction
{
public:
    EventAction(RunAction* runAction);
    virtual ~EventAction();

    virtual void BeginOfEventAction(const G4Event* event);
    virtual void EndOfEventAction(const G4Event* event);

    // Methods for recording energy deposits (simplified approach)
    void AddEnergyDeposit(G4double energy, G4int detectorID);
    
    // Backward compatibility methods
    void AddEnergyDepositDet1(G4double edep) { fEnergyDepositDet1 += edep; }
    void AddEnergyDepositDet2(G4double edep) { fEnergyDepositDet2 += edep; }

    // Getters for analysis
    const std::vector<CoincidenceEvent>& GetCoincidences() const { return fCoincidences; }

private:
    RunAction* fRunAction;
    
    // Total energy deposits per detector (like original code)
    G4double fEnergyDepositDet1;
    G4double fEnergyDepositDet2;
    
    // Collections for coincidence analysis (simplified)
    std::vector<GammaHit> fAllHits;
    std::vector<GammaHit> fHitsDet1;
    std::vector<GammaHit> fHitsDet2;
    std::vector<CoincidenceEvent> fCoincidences;
    
    // Analysis parameters
    G4double fCoincidenceWindow;
    G4double fMinimumEnergy;
    
    // Analysis methods
    void AnalyzeCoincidences();
    G4double CalculateAngleCorrelation(const GammaHit& hit1, const GammaHit& hit2);
    G4bool IsTrueCoincidence(const GammaHit& hit1, const GammaHit& hit2);
};

#endif