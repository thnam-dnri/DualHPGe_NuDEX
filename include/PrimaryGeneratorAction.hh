// ==============================================================================
// PrimaryGeneratorAction.hh - Enhanced for Cascade Generation
// ==============================================================================

#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"
#include "G4ThreeVector.hh"
#include <string>
#include <vector>
#include <random>

// NuDEX: statistical de-excitation cascades after neutron capture
#include "NuDEXStatisticalNucleus.hh"

class G4ParticleGun;
class G4Event;

// Enhanced structure for gamma cascade data
struct GammaData {
    double energy;      // MeV
    double intensity;   // relative intensity
    double time;        // ns (cascade timing)
    int cascadeIndex;   // Which cascade this gamma belongs to
    int sequenceOrder;  // Order in cascade (1st, 2nd, etc.)
};

// Structure for complete cascade sequences
struct CascadeData {
    std::vector<GammaData> gammas;
    double totalIntensity;
    std::string cascadeName;
};

// Structure for two-gamma pairs (Sn -> intermediate -> ground)
struct TwoGammaPair {
    double gamma1;              // First gamma energy (MeV): Sn - E_intermediate
    double gamma2;              // Second gamma energy (MeV): E_intermediate
    int intermediateLevel;      // Intermediate level number
    double intermediateEnergy;  // Intermediate level energy (MeV)
    double spin;                // Intermediate level spin
    int parity;                 // Intermediate level parity
};

// Source mode enumeration
enum SourceMode {
    CO60_CASCADE,    // Co-60 cascade (2 gammas: 1.173 + 1.332 MeV)
    SINGLE_GAMMA,    // Single gamma mode (random Co-60 gamma)
    NUDEX_CAPTURE    // Thermal neutron capture cascades via NuDEX
};

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
    PrimaryGeneratorAction(bool generateCascades = true,
                           SourceMode initialMode = CO60_CASCADE);
    virtual ~PrimaryGeneratorAction();

    virtual void GeneratePrimaries(G4Event*);

    const G4ParticleGun* GetParticleGun() const { return fParticleGun; }

    void SetSourceMode(SourceMode mode);
    // NuDEX configuration
    void SetNuDEXConfig(int za, const std::string& libdir) { fNuDEX_ZA = za; fNuDEXLibDir = libdir; }

private:
    G4ParticleGun* fParticleGun;
    std::vector<GammaData> fGammaData;        // Individual gammas (legacy)
    std::vector<CascadeData> fCascadeData;    // Complete cascades
    std::mt19937 fRandomGenerator;
    bool fGenerateCascades;                   // Flag for cascade mode

    SourceMode fSourceMode;
    // NuDEX members
    NuDEXStatisticalNucleus* fNuDEX = nullptr;
    int fNuDEX_ZA = -1;
    std::string fNuDEXLibDir;

    // Methods for cascade handling
    GammaData SampleGamma();                  // Sample individual gamma (legacy)

    // Position and direction sampling
    G4ThreeVector SampleSourcePosition();
    G4ThreeVector SampleDirection();

    // Cascade generation methods
    void GenerateSingleGammaEvent(G4Event* anEvent);
    void GenerateCo60Cascade(G4Event* anEvent);
    void GenerateNuDEXCascade(G4Event* anEvent);
    const char* SourceModeToString(SourceMode mode) const;
};

#endif
