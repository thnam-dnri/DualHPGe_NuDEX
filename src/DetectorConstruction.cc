// ==============================================================================
// DetectorConstruction.cc - Phase 2 Dual HPGe Detector System
// ==============================================================================

#include "DetectorConstruction.hh"

#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Cons.hh"
#include "G4UnionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SubtractionSolid.hh"
#include "G4SDManager.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4VPrimitiveScorer.hh"
#include "G4PSEnergyDeposit.hh"
#include "G4RotationMatrix.hh"

#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

// Static member definitions
const G4double DetectorConstruction::fWorldSize = 100.0*cm;  // Increased for dual setup
const G4double DetectorConstruction::fSourceDetectorDistance = 5.0*cm;  // Distance from detector surface to source aka origin

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::DetectorConstruction(G4double detector2Angle)
: G4VUserDetectorConstruction(),
  fDetector2Angle(detector2Angle),
  fWorldMaterial(nullptr), fGermanium(nullptr), fAluminum(nullptr),
  fVacuum(nullptr), fMylar(nullptr), fLithium(nullptr), fBoron(nullptr), fLead(nullptr),
  fWorldLV(nullptr), fScoringVolume1(nullptr), fScoringVolume2(nullptr), fWorldPV(nullptr)
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::~DetectorConstruction()
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    DefineMaterials();
    return DefineVolumes();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::DefineMaterials()
{
    G4NistManager* nist = G4NistManager::Instance();
    
    // Air for world
    fWorldMaterial = nist->FindOrBuildMaterial("G4_AIR");
    
    // Vacuum for gaps and housing interior
    fVacuum = nist->FindOrBuildMaterial("G4_Galactic");
    
    // Aluminum for housing and windows
    fAluminum = nist->FindOrBuildMaterial("G4_Al");
    
    // High purity germanium
    fGermanium = nist->FindOrBuildMaterial("G4_Ge");
    
    // Mylar for window layer
    fMylar = nist->FindOrBuildMaterial("G4_MYLAR");
    
    // Create realistic dead layer materials (Ge with dopants)
    // Lithium-doped germanium (n+ contact, outer)
    fLithium = new G4Material("Li_doped_Ge", 5.32*g/cm3, 2);
    fLithium->AddMaterial(fGermanium, 99.9*perCent);
    fLithium->AddMaterial(nist->FindOrBuildMaterial("G4_Li"), 0.1*perCent);
    
    // Boron-doped germanium (p+ contact, inner)
    fBoron = new G4Material("B_doped_Ge", 5.32*g/cm3, 2);
    fBoron->AddMaterial(fGermanium, 99.9*perCent);
    fBoron->AddMaterial(nist->FindOrBuildMaterial("G4_B"), 0.1*perCent);

    // Lead for shielding
    fLead = nist->FindOrBuildMaterial("G4_Pb");
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* DetectorConstruction::DefineVolumes()
{
    // World volume
    G4Box* worldS = new G4Box("World", fWorldSize/2, fWorldSize/2, fWorldSize/2);
    fWorldLV = new G4LogicalVolume(worldS, fWorldMaterial, "World");
    fWorldPV = new G4PVPlacement(0, G4ThreeVector(), fWorldLV, "World", 0, false, 0, true);
    // Housing length is 76mm, so housing center should be at distance + half housing length
    G4double housingLength = 76.0*mm;
    G4double housingCenterDistance = fSourceDetectorDistance + housingLength/2;

    // Position and rotation for first detector (along +Z axis)
    G4ThreeVector detector1Position(0, 0, housingCenterDistance);
    G4RotationMatrix* detector1Rotation = nullptr; // No rotation needed

    // Position and rotation for second detector
    G4double angleRad = fDetector2Angle * deg;
    G4ThreeVector detector2Position(
        housingCenterDistance * sin(angleRad),
        0,
        housingCenterDistance * cos(angleRad)
    );
    
    G4RotationMatrix* detector2Rotation = new G4RotationMatrix();
    detector2Rotation->rotateY(-angleRad); // Rotate around Y axis to point towards source

    // Construct both detectors
    ConstructSingleDetector(fWorldLV, detector1Position, detector1Rotation, "Det1_", fScoringVolume1);
    ConstructSingleDetector(fWorldLV, detector2Position, detector2Rotation, "Det2_", fScoringVolume2);

    // Construct both lead shields (they use detector positions to determine direction)
    ConstructLeadShield(fWorldLV, detector1Position, detector1Rotation, "Det1_");
    ConstructLeadShield(fWorldLV, detector2Position, detector2Rotation, "Det2_");

    // Visualization attributes for world
    fWorldLV->SetVisAttributes(G4VisAttributes::GetInvisible());

    return fWorldPV;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::ConstructSingleDetector(G4LogicalVolume* motherVolume, 
                                                  G4ThreeVector position, 
                                                  G4RotationMatrix* rotation,
                                                  G4String namePrefix,
                                                  G4LogicalVolume*& scoringVolume)
{
    // Detector parameters
    G4double housingInnerDiam = 64.46*mm;
    G4double housingOuterDiam = 67.0*mm;
    G4double housingLength    = 76.0*mm;
    
    G4double windowDiam       = 64.46*mm;
    G4double alWindowThick    = 1.27*mm;
    G4double mylarThick       = 0.025*mm;
    G4double alFoilThick      = 0.025*mm;
    G4double alCupThick       = 0.5*mm;
    
    G4double geCrystalDiam    = 57.6*mm;
    G4double geCrystalLength  = 66.8*mm;
    G4double boreHoleDiam     = 10.5*mm;
    G4double boreHoleDepth    = 53.5*mm;
    
    G4double liDeadThick      = 0.7*mm;
    G4double bDeadThick       = 0.3*micrometer;
    
    G4double windowGap        = 3.0*mm;
    G4double totalWindowThick = alWindowThick + mylarThick + alFoilThick + alCupThick;

    // =========================================================
    // 1. Aluminum Housing (rotated + positioned in world)
    // =========================================================
    G4Tubs* housingSolid = new G4Tubs(namePrefix + "Housing", 
                                     housingInnerDiam/2, housingOuterDiam/2, 
                                     housingLength/2, 0*deg, 360*deg);
    G4LogicalVolume* housingLV = new G4LogicalVolume(housingSolid, fAluminum, namePrefix + "Housing");

    new G4PVPlacement(rotation, position, housingLV,
                      namePrefix + "Housing", motherVolume,
                      false, 0, true);

    // =========================================================
    // 2. Vacuum inside housing (placed with same rotation!)
    // =========================================================
    G4Tubs* vacuumSolid = new G4Tubs(namePrefix + "VacuumInside",
                                     0.0*mm, housingInnerDiam/2 - 0.1*mm,
                                     housingLength/2 - 0.1*mm, 0*deg, 360*deg);
    G4LogicalVolume* vacuumLV = new G4LogicalVolume(vacuumSolid, fVacuum, namePrefix + "VacuumInside");

    new G4PVPlacement(rotation, position, vacuumLV,
                      namePrefix + "VacuumInside", motherVolume,
                      false, 0, true);

    // =========================================================
    // Now place everything relative to vacuum center
    // =========================================================
    G4double windowStartZ = -housingLength/2;
    G4double geDistanceFromFront = totalWindowThick + windowGap + geCrystalLength/2;
    G4double geRelativeZ = windowStartZ + geDistanceFromFront;

    // ------------------ Windows -------------------
    G4double currentZ = windowStartZ;

    // Al window
    G4Tubs* alWindowSolid = new G4Tubs(namePrefix + "AlWindow", 0, windowDiam/2, alWindowThick/2, 0, 360*deg);
    G4LogicalVolume* alWindowLV = new G4LogicalVolume(alWindowSolid, fAluminum, namePrefix + "AlWindow");
    new G4PVPlacement(0, G4ThreeVector(0,0,currentZ + alWindowThick/2), alWindowLV,
                      namePrefix + "AlWindow", vacuumLV, false, 0, true);
    currentZ += alWindowThick;

    // Mylar
    G4Tubs* mylarSolid = new G4Tubs(namePrefix + "Mylar", 0, windowDiam/2, mylarThick/2, 0, 360*deg);
    G4LogicalVolume* mylarLV = new G4LogicalVolume(mylarSolid, fMylar, namePrefix + "Mylar");
    new G4PVPlacement(0, G4ThreeVector(0,0,currentZ + mylarThick/2), mylarLV,
                      namePrefix + "Mylar", vacuumLV, false, 0, true);
    currentZ += mylarThick;

    // Al foil
    G4Tubs* alFoilSolid = new G4Tubs(namePrefix + "AlFoil", 0, windowDiam/2, alFoilThick/2, 0, 360*deg);
    G4LogicalVolume* alFoilLV = new G4LogicalVolume(alFoilSolid, fAluminum, namePrefix + "AlFoil");
    new G4PVPlacement(0, G4ThreeVector(0,0,currentZ + alFoilThick/2), alFoilLV,
                      namePrefix + "AlFoil", vacuumLV, false, 0, true);
    currentZ += alFoilThick;

    // Al cup
    G4Tubs* alCupSolid = new G4Tubs(namePrefix + "AlCup", 0, windowDiam/2, alCupThick/2, 0, 360*deg);
    G4LogicalVolume* alCupLV = new G4LogicalVolume(alCupSolid, fAluminum, namePrefix + "AlCup");
    new G4PVPlacement(0, G4ThreeVector(0,0,currentZ + alCupThick/2), alCupLV,
                      namePrefix + "AlCup", vacuumLV, false, 0, true);

    // ------------------ Germanium Crystal -------------------
    G4Tubs* geOuter = new G4Tubs(namePrefix + "GeOuter", 0, geCrystalDiam/2, geCrystalLength/2, 0, 360*deg);
    G4Tubs* boreHole = new G4Tubs(namePrefix + "BoreHole", 0, boreHoleDiam/2, boreHoleDepth/2, 0, 360*deg);

    // Bore hole extends from BACK surface (away from origin) of crystal
    G4double boreHoleZ = +geCrystalLength/2 - boreHoleDepth/2;
    G4SubtractionSolid* geSolid = new G4SubtractionSolid(namePrefix + "GeCrystal", geOuter, boreHole, 0, G4ThreeVector(0,0,boreHoleZ));
    G4LogicalVolume* geLV = new G4LogicalVolume(geSolid, fGermanium, namePrefix + "GeCrystal");

    new G4PVPlacement(0, G4ThreeVector(0,0,geRelativeZ), geLV,
                      namePrefix + "GeCrystal", vacuumLV, false, 0, true);

    scoringVolume = geLV; // set scoring volume

    // ------------------ Dead layers -------------------
    G4Tubs* liSolid = new G4Tubs(namePrefix + "LiDead", geCrystalDiam/2, (geCrystalDiam/2 + liDeadThick), geCrystalLength/2, 0, 360*deg);
    G4LogicalVolume* liLV = new G4LogicalVolume(liSolid, fLithium, namePrefix + "LiDead");
    new G4PVPlacement(0, G4ThreeVector(0,0,geRelativeZ), liLV,
                      namePrefix + "LiDead", vacuumLV, false, 0, true);

    G4Tubs* bSolid = new G4Tubs(namePrefix + "BDead", (boreHoleDiam/2 - bDeadThick), boreHoleDiam/2, boreHoleDepth/2, 0, 360*deg);
    G4LogicalVolume* bLV = new G4LogicalVolume(bSolid, fBoron, namePrefix + "BDead");
    new G4PVPlacement(0, G4ThreeVector(0,0,geRelativeZ + boreHoleZ), bLV,
                      namePrefix + "BDead", vacuumLV, false, 0, true);

    // =========================================================
    // Visualization attributes
    // =========================================================
    // Housing and vacuum as wireframe to see inner crystal
    G4VisAttributes* housingVis = new G4VisAttributes(G4Colour(0.5,0.5,0.5,0.8));
    housingVis->SetForceWireframe(true);
    housingLV->SetVisAttributes(housingVis);

    G4VisAttributes* vacuumVis = new G4VisAttributes(G4Colour(0.8,0.8,0.8,0.3));
    vacuumVis->SetForceWireframe(true);
    vacuumLV->SetVisAttributes(vacuumVis);

    G4VisAttributes* alVis = new G4VisAttributes(G4Colour(0.7,0.7,0.7,0.6));
    alVis->SetForceSolid(true);
    alWindowLV->SetVisAttributes(alVis);
    alFoilLV->SetVisAttributes(alVis);
    alCupLV->SetVisAttributes(alVis);

    G4VisAttributes* mylarVis = new G4VisAttributes(G4Colour(0.8,0.2,0.8,0.6));
    mylarVis->SetForceSolid(true);
    mylarLV->SetVisAttributes(mylarVis);

    G4Colour geColor = (namePrefix == "Det1_") ? G4Colour(0.0,1.0,1.0,0.8) : G4Colour(0.0,0.8,0.0,0.8);
    G4VisAttributes* geVis = new G4VisAttributes(geColor);
    geVis->SetForceSolid(true);
    geLV->SetVisAttributes(geVis);

    G4VisAttributes* liVis = new G4VisAttributes(G4Colour(1.0,0.0,0.0,0.5));
    liVis->SetForceSolid(true);
    liLV->SetVisAttributes(liVis);

    G4VisAttributes* bVis = new G4VisAttributes(G4Colour(0.0,0.0,1.0,0.7));
    bVis->SetForceSolid(true);
    bLV->SetVisAttributes(bVis);
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::ConstructSDandField()
{
    // We don't use Geant4's sensitive detector system here
    // Instead, we handle energy deposits directly in SteppingAction
    // by checking the logical volume pointers
    
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::ConstructLeadShield(G4LogicalVolume* motherVolume,
                                               G4ThreeVector position,
                                               G4RotationMatrix* rotation,
                                               G4String namePrefix)
{
    // Shield geometry parameters
    G4double housingLength = 76.0*mm;

    // Shield specifications
    G4double targetOpeningRadius = 10.0*mm;      // 2cm diameter opening at target
    G4double collimatorRadius = 38.5*mm;         // 77mm diameter (housing + 1cm)
    G4double shieldStartDist = 25.0*mm;          // 2.5cm from origin (5cm gap between shields)
    G4double leadThickness = 50.0*mm;            // 5cm lead thickness

    // Absolute positions along Z axis
    G4double detectorFrontDist = fSourceDetectorDistance;  // 50mm from origin
    G4double detectorBackDist = detectorFrontDist + housingLength;  // 126mm from origin

    // Shield dimensions
    G4double shieldLength = detectorBackDist - shieldStartDist + 5.0*mm;  // Extra 5mm clearance
    G4double shieldOuterRadius = collimatorRadius + leadThickness;
    G4double shieldCenterDist = shieldStartDist + shieldLength/2;

    // 1. Create outer cylinder (simple cylindrical shape)
    G4Tubs* outerCylinder = new G4Tubs(namePrefix + "ShieldOuter",
                                       0.0, shieldOuterRadius,
                                       shieldLength/2,
                                       0*deg, 360*deg);

    // 2. Create collimator cone (from shield front to detector front)
    G4double collimatorLength = detectorFrontDist - shieldStartDist;  // 25mm
    G4double collimatorCenterZ = -shieldLength/2 + collimatorLength/2;

    G4Cons* collimatorCone = new G4Cons(namePrefix + "Collimator",
                                        0.0, targetOpeningRadius,      // front radius
                                        0.0, collimatorRadius,         // back radius
                                        collimatorLength/2,
                                        0*deg, 360*deg);

    // 3. Create detector clearance cylinder (from detector front to detector back)
    G4double clearanceLength = housingLength;
    G4double clearanceCenterZ = -shieldLength/2 + collimatorLength + clearanceLength/2;

    G4Tubs* clearanceCylinder = new G4Tubs(namePrefix + "Clearance",
                                           0.0, collimatorRadius,
                                           clearanceLength/2,
                                           0*deg, 360*deg);

    // 4. Union the collimator cone and clearance cylinder
    G4UnionSolid* innerOpening = new G4UnionSolid(namePrefix + "InnerOpening",
                                                  collimatorCone, clearanceCylinder,
                                                  0, G4ThreeVector(0, 0, clearanceCenterZ - collimatorCenterZ));

    // 5. Subtract the inner opening from outer cylinder
    G4SubtractionSolid* shieldSolid = new G4SubtractionSolid(namePrefix + "Shield",
                                                             outerCylinder, innerOpening,
                                                             0, G4ThreeVector(0, 0, collimatorCenterZ));

    G4LogicalVolume* shieldLV = new G4LogicalVolume(shieldSolid, fLead, namePrefix + "Shield");

    // Position shield at its center distance from origin
    // Use the detector position to determine direction
    G4ThreeVector direction = position.unit();  // Get unit direction vector
    G4ThreeVector shieldPosition = direction * shieldCenterDist;  // Scale to shield center

    new G4PVPlacement(rotation, shieldPosition, shieldLV,
                     namePrefix + "Shield", motherVolume,
                     false, 0, true);

    // Visualization attributes
    G4VisAttributes* shieldVis = new G4VisAttributes(G4Colour(0.3, 0.3, 0.3, 0.7));
    shieldVis->SetForceSolid(true);
    shieldLV->SetVisAttributes(shieldVis);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double DetectorConstruction::CalculateDetectionEfficiency(G4double gammaEnergy) const
{
    // Geometric efficiency calculation for coaxial HPGe detector
    // This provides an analytical estimate based on detector geometry
    
    G4double geCrystalDiam = 57.6*mm;
    G4double geCrystalLength = 66.8*mm;
    G4double boreHoleDiam = 10.5*mm;
    G4double boreHoleDepth = 53.5*mm;
    G4double sourceDistance = fSourceDetectorDistance;
    
    // Calculate solid angle for cylindrical detector with bore hole
    G4double R = geCrystalDiam/2;  // Outer radius
    G4double r = boreHoleDiam/2;   // Bore hole radius
    G4double H = geCrystalLength;  // Crystal height
    G4double h = boreHoleDepth;    // Bore hole depth
    G4double d = sourceDistance;   // Source distance
    
    // Solid angle for full cylinder
    G4double solidAngleFull = 2*pi * (1 - d/sqrt(d*d + R*R));
    
    // Solid angle for bore hole (subtracted)
    G4double dBore = d + (H - h);  // Distance to bore hole bottom
    G4double solidAngleBore = 2*pi * (1 - dBore/sqrt(dBore*dBore + r*r));
    
    // Net geometric efficiency
    G4double geometricEff = (solidAngleFull - solidAngleBore) / (4*pi);
    
    // Attenuation corrections (simplified)
    G4double windowAttenuation = 1.0;
    if (gammaEnergy < 0.1) {  // MeV
        windowAttenuation = 0.95;  // 5% loss for low energy
    } else if (gammaEnergy < 1.0) {
        windowAttenuation = 0.98;  // 2% loss for medium energy
    } else {
        windowAttenuation = 0.99;  // 1% loss for high energy
    }
    
    // Dead layer effects (energy dependent)
    G4double deadLayerEff = 1.0;
    if (gammaEnergy < 0.05) {
        deadLayerEff = 0.8;   // 20% loss for very low energy
    } else if (gammaEnergy < 0.2) {
        deadLayerEff = 0.95;  // 5% loss for low energy
    }
    
    G4double totalEfficiency = geometricEff * windowAttenuation * deadLayerEff;
    
    return totalEfficiency;
}