// ==============================================================================
// DetectorConstruction.hh - Phase 2 Dual HPGe Detector System
// ==============================================================================

#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Material.hh"

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction(G4double detector2Angle = 180.0); // Default 180 degrees
    virtual ~DetectorConstruction();

    virtual G4VPhysicalVolume* Construct();
    virtual void ConstructSDandField();

    // Get methods for analysis
    G4LogicalVolume* GetScoringVolume1() const { return fScoringVolume1; }
    G4LogicalVolume* GetScoringVolume2() const { return fScoringVolume2; }
    
    // Efficiency calculation method
    G4double CalculateDetectionEfficiency(G4double gammaEnergy) const;
    
    // Get detector angle
    G4double GetDetector2Angle() const { return fDetector2Angle; }

private:
    // Detector parameters
    static const G4double fSourceDetectorDistance;  // 10 cm
    static const G4double fWorldSize;              // 100 cm (increased for dual setup)
    G4double fDetector2Angle;                      // Angle for second detector (degrees)

    // Materials
    void DefineMaterials();
    G4Material* fWorldMaterial;
    G4Material* fGermanium;
    G4Material* fAluminum;
    G4Material* fVacuum;
    G4Material* fMylar;
    G4Material* fLithium;
    G4Material* fBoron;
    G4Material* fLead;

    // Volumes
    G4LogicalVolume* fWorldLV;
    G4LogicalVolume* fScoringVolume1;  // Points to first Ge crystal
    G4LogicalVolume* fScoringVolume2;  // Points to second Ge crystal
    G4VPhysicalVolume* fWorldPV;

    // Construction methods
    G4VPhysicalVolume* DefineVolumes();
    void ConstructSingleDetector(G4LogicalVolume* motherVolume,
                                G4ThreeVector position,
                                G4RotationMatrix* rotation,
                                G4String namePrefix,
                                G4LogicalVolume*& scoringVolume);
    void ConstructLeadShield(G4LogicalVolume* motherVolume,
                            G4ThreeVector position,
                            G4RotationMatrix* rotation,
                            G4String namePrefix);
};

#endif