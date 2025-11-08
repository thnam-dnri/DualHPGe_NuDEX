//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
// Run.cc - Updated with Basic Coincidence Support

#include "Run.hh"
#include "EventAction.hh"  // Include to get full CoincidenceEvent definition
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include <fstream>
#include <cmath>

// External global variable for quiet mode
extern bool g_quietMode;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

Run::Run()
: G4Run(),
  fTotalEnergyDepositDet1(0.),
  fTotalEnergyDepositDet2(0.),
  fTotalEventsDet1(0),
  fTotalEventsDet2(0)
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

Run::~Run()
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::Merge(const G4Run* run)
{
    const Run* localRun = static_cast<const Run*>(run);
    
    // Merge original single detector histograms
    for (const auto& bin : localRun->fEnergyHistogramDet1) {
        fEnergyHistogramDet1[bin.first] += bin.second;
    }
    for (const auto& bin : localRun->fEnergyHistogramDet2) {
        fEnergyHistogramDet2[bin.first] += bin.second;
    }
    
    
    // Merge statistics
    fTotalEnergyDepositDet1 += localRun->fTotalEnergyDepositDet1;
    fTotalEnergyDepositDet2 += localRun->fTotalEnergyDepositDet2;
    fTotalEventsDet1 += localRun->fTotalEventsDet1;
    fTotalEventsDet2 += localRun->fTotalEventsDet2;
    
    G4Run::Merge(run);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::AddEnergySpectrumDet1(G4double energy)
{
    G4int bin = EnergyToBin(energy);
    if (bin >= 0 && bin < fNbins) {
        fEnergyHistogramDet1[bin]++;
    }
    fTotalEnergyDepositDet1 += energy;
    fTotalEventsDet1++;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::AddEnergySpectrumDet2(G4double energy)
{
    G4int bin = EnergyToBin(energy);
    if (bin >= 0 && bin < fNbins) {
        fEnergyHistogramDet2[bin]++;
    }
    fTotalEnergyDepositDet2 += energy;
    fTotalEventsDet2++;
}


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4int Run::EnergyToBin(G4double energy) const
{
    // Convert energy from MeV to keV and bin it (1 keV per bin)
    G4double energy_keV = energy / keV;
    if (energy_keV < 0 || energy_keV >= fEmax * 1000) {
        return -1; // Out of range
    }
    return static_cast<G4int>(energy_keV);
}


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::PrintResults() const
{
    G4cout << "\n========== Dual Detector Results ==========" << G4endl;

    // Single detector results
    G4cout << "\n=== DETECTOR RESULTS ===" << G4endl;
    G4cout << "Detector 1 - Total events: " << fTotalEventsDet1
           << ", Total energy: " << G4BestUnit(fTotalEnergyDepositDet1, "Energy") << G4endl;
    G4cout << "Detector 2 - Total events: " << fTotalEventsDet2
           << ", Total energy: " << G4BestUnit(fTotalEnergyDepositDet2, "Energy") << G4endl;

    // Peak analysis for single detectors
    G4cout << "\n=== SIGNIFICANT PEAKS (>10 counts) ===" << G4endl;
    G4cout << "Detector 1:" << G4endl;
    for (const auto& bin : fEnergyHistogramDet1) {
        if (bin.second > 10) {
            G4cout << "  " << bin.first << " keV: " << bin.second << " counts" << G4endl;
        }
    }
    G4cout << "Detector 2:" << G4endl;
    for (const auto& bin : fEnergyHistogramDet2) {
        if (bin.second > 10) {
            G4cout << "  " << bin.first << " keV: " << bin.second << " counts" << G4endl;
        }
    }

    // All data saved to ROOT file (output.root)
    G4cout << "\nAll spectral data saved to ROOT file: output.root" << G4endl;
    G4cout << "==========================================================\n" << G4endl;
}

