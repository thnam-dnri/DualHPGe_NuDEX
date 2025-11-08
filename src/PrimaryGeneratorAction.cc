// ==============================================================================
// PrimaryGeneratorAction.cc - Enhanced for Cascade Generation
// ==============================================================================

#include "PrimaryGeneratorAction.hh"

#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4IonTable.hh"
#include "G4NuclideTable.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "G4PhysicalConstants.hh"
#include "G4Event.hh"
#include "G4Gamma.hh"
#include "G4ReactionProduct.hh"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <vector>
#include <sstream>

// External global variable for quiet mode
extern bool g_quietMode;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::PrimaryGeneratorAction(bool generateCascades,
                                               SourceMode initialMode)
: G4VUserPrimaryGeneratorAction(),
  fParticleGun(0),
  fRandomGenerator(std::random_device{}()),
  fGenerateCascades(generateCascades),
  fSourceMode(initialMode)
{
    G4int n_particle = 1;
    fParticleGun = new G4ParticleGun(n_particle);

    // Default particle type and properties - set to gamma
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    G4String particleName = "gamma";
    G4ParticleDefinition* particle = particleTable->FindParticle(particleName);
    fParticleGun->SetParticleDefinition(particle);
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
    fParticleGun->SetParticleEnergy(1.*MeV);

    // Debug output to verify constructor parameters
    if (!g_quietMode) {
        G4cout << "PrimaryGeneratorAction constructor called with:" << G4endl;
        G4cout << "  generateCascades = " << fGenerateCascades << G4endl;
        G4cout << "  sourceMode = " << SourceModeToString(fSourceMode) << G4endl;
    }

    if (!g_quietMode) {
        switch (fSourceMode) {
            case CO60_CASCADE:
                G4cout << "Using Co-60 cascade source (1.173 and 1.332 MeV)" << G4endl;
                break;
            case SINGLE_GAMMA:
                G4cout << "Using single gamma mode (randomized Co-60 gamma)" << G4endl;
                break;
            default: break;
    }
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction::SetSourceMode(SourceMode mode)
{
    if (fSourceMode == mode) return;

    fSourceMode = mode;

    if (!g_quietMode) {
        G4cout << "PrimaryGeneratorAction: Switching source mode to "
               << SourceModeToString(mode) << G4endl;
    }

}

const char* PrimaryGeneratorAction::SourceModeToString(SourceMode mode) const
{
    switch (mode) {
        case CO60_CASCADE:
            return "Co-60 cascade";
        case SINGLE_GAMMA:
            return "single gamma";
        case NUDEX_CAPTURE:
            return "NuDEX thermal capture";
        default:
            break;
    }
    return "unknown";
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
    switch(fSourceMode) {
        case CO60_CASCADE:
            GenerateCo60Cascade(anEvent);
            break;
        case SINGLE_GAMMA:
            GenerateSingleGammaEvent(anEvent);
            break;
        case NUDEX_CAPTURE:
            GenerateNuDEXCascade(anEvent);
            break;
        default:
            break;
    }
}


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction::GenerateSingleGammaEvent(G4Event* anEvent)
{
    // Original single gamma generation (for compatibility)
    GammaData gamma = SampleGamma();
    
    fParticleGun->SetParticleEnergy(gamma.energy * MeV);
    
    G4ThreeVector sourcePos = SampleSourcePosition();
    fParticleGun->SetParticlePosition(sourcePos);
    
    G4ThreeVector direction = SampleDirection();
    fParticleGun->SetParticleMomentumDirection(direction);
    
    fParticleGun->SetParticleTime(0.0);
    
    fParticleGun->GeneratePrimaryVertex(anEvent);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction::GenerateCo60Cascade(G4Event* anEvent)
{
    // Generate Co-60 cascade (2 gammas per event)
    G4ThreeVector sourcePos = SampleSourcePosition();

    // First gamma: 1.173 MeV
    fParticleGun->SetParticleEnergy(1.173 * MeV);
    fParticleGun->SetParticlePosition(sourcePos);
    fParticleGun->SetParticleMomentumDirection(SampleDirection());
    fParticleGun->SetParticleTime(0.0);
    fParticleGun->GeneratePrimaryVertex(anEvent);

    // Second gamma: 1.332 MeV
    fParticleGun->SetParticleEnergy(1.332 * MeV);
    fParticleGun->SetParticlePosition(sourcePos);
    fParticleGun->SetParticleMomentumDirection(SampleDirection());
    fParticleGun->SetParticleTime(0.0);
    fParticleGun->GeneratePrimaryVertex(anEvent);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// CASCADE mode removed

void PrimaryGeneratorAction::GenerateNuDEXCascade(G4Event* anEvent)
{
    // Lazy init NuDEX if needed
    if (!fNuDEX) {
        if (fNuDEX_ZA <= 0 || fNuDEXLibDir.empty()) {
            G4cerr << "ERROR: NuDEX configuration missing (ZA/libdir)." << G4endl;
            return;
        }
        int Z = fNuDEX_ZA / 1000;
        int A = fNuDEX_ZA % 1000;
        fNuDEX = new NuDEXStatisticalNucleus(Z, A);
        // Resolve library directory (handle different working directories)
        std::vector<std::string> candidates = {
            fNuDEXLibDir,
            std::string("NuDEX/NuDEXlib/"),
            std::string("./NuDEX/NuDEXlib/"),
            std::string("../NuDEX/NuDEXlib/"),
            std::string("/Users/namtran/Project/DualHPGe_NuDEX/NuDEX/NuDEXlib/")
        };
        std::string resolved;
        for (const auto& c : candidates) {
            std::ifstream test((c + "GeneralStatNuclParameters.dat").c_str());
            if (test.good()) { resolved = c; break; }
        }
        if (resolved.empty()) { resolved = fNuDEXLibDir; }
        // Initialize from resolved library directory
        if (fNuDEX->Init(resolved.c_str()) < 0) {
            G4cerr << "ERROR: NuDEX initialization failed for ZA=" << fNuDEX_ZA
                   << " using libdir='" << resolved << "'" << G4endl;
            delete fNuDEX;
            fNuDEX = nullptr;
            return;
        }
        if (!g_quietMode) {
            G4cout << "NuDEX initialized: ZA=" << fNuDEX_ZA
                   << ", libdir resolved" << G4endl;
        }
    }

    std::vector<char> types;
    std::vector<double> energies;
    std::vector<double> times;

    // Start from thermal capture level with ~thermal neutron energy (negative to indicate En)
    int npar = fNuDEX->GenerateCascade(-1, -1e-6, types, energies, times);
    if (npar <= 0) {
        // On failure, do nothing for this event
        return;
    }

    G4ThreeVector sourcePos = SampleSourcePosition();
    for (int i = 0; i < npar; ++i) {
        char t = types[i];
        double E = energies[i];    // MeV
        double T = times[i];       // seconds

        if (t == 'g') {
            fParticleGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
        } else if (t == 'e') {
            fParticleGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
        } else {
            // Skip unknown particle types
            continue;
        }

        fParticleGun->SetParticleEnergy(E * MeV);
        fParticleGun->SetParticlePosition(sourcePos);
        fParticleGun->SetParticleMomentumDirection(SampleDirection());
        fParticleGun->SetParticleTime(T * s);
        fParticleGun->GeneratePrimaryVertex(anEvent);
    }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

GammaData PrimaryGeneratorAction::SampleGamma()
{
    // Co-60 gamma source: randomly select between 1.173 and 1.332 MeV
    GammaData gamma;

    std::uniform_real_distribution<> dist(0.0, 1.0);
    if (dist(fRandomGenerator) < 0.5) {
        gamma.energy = 1.173;  // 1.173 MeV
    } else {
        gamma.energy = 1.332;  // 1.332 MeV
    }

    gamma.intensity = 100.0;
    gamma.time = 0.0;
    gamma.cascadeIndex = -1;
    gamma.sequenceOrder = 1;

    return gamma;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4ThreeVector PrimaryGeneratorAction::SampleSourcePosition()
{
    // Point source at origin
    return G4ThreeVector(0., 0., 0.);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4ThreeVector PrimaryGeneratorAction::SampleDirection()
{
    // Isotropic emission direction
    G4double cosTheta = 2.*G4UniformRand() - 1.;
    G4double sinTheta = std::sqrt(1. - cosTheta*cosTheta);
    G4double phi = twopi*G4UniformRand();

    return G4ThreeVector(sinTheta*std::cos(phi),
                        sinTheta*std::sin(phi),
                        cosTheta);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// Removed all RAINIER-related methods
