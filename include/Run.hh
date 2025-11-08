// ==============================================================================
// Run.hh - Updated with Basic Coincidence Support
// ==============================================================================

#ifndef Run_h
#define Run_h 1

#include "G4Run.hh"
#include "globals.hh"
#include <map>
#include <vector>
#include <string>

// Forward declarations for coincidence analysis (structures defined in EventAction.hh)
struct GammaHit;
struct CoincidenceEvent;

class Run : public G4Run
{
public:
    Run();
    virtual ~Run();

    virtual void Merge(const G4Run*);
    
    // Original single detector methods (maintain compatibility)
    void AddEnergySpectrumDet1(G4double energy);
    void AddEnergySpectrumDet2(G4double energy);
    
    void PrintResults() const;

private:
    // Original single detector data
    std::map<G4int, G4int> fEnergyHistogramDet1;
    std::map<G4int, G4int> fEnergyHistogramDet2;
    G4double fTotalEnergyDepositDet1;
    G4double fTotalEnergyDepositDet2;
    G4int fTotalEventsDet1;
    G4int fTotalEventsDet2;


    static constexpr G4int fNbins = 10000;       // Energy bins (1 keV per bin)
    static constexpr G4int fNAngleBins = 18;     // Angular bins (10° each)
    static constexpr G4double fEmax = 10.0;      // MeV

    // Helper methods
    G4int EnergyToBin(G4double energy) const;
};

#endif