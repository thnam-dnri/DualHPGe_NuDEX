//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
// RunAction.cc - Implementation for dual detectors

#include "RunAction.hh"
#include "PrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"
#include "Run.hh"

#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4AccumulableManager.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

RunAction::RunAction()
: G4UserRunAction(),
  fEnergyDepositDet1("EnergyDepositDet1", 0.),
  fEnergyDepositDet2("EnergyDepositDet2", 0.),
  fEventCountDet1("EventCountDet1", 0),
  fEventCountDet2("EventCountDet2", 0)
{
    // Register accumulables to the accumulable manager
    G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
    accumulableManager->Register(fEnergyDepositDet1);
    accumulableManager->Register(fEnergyDepositDet2);
    accumulableManager->Register(fEventCountDet1);
    accumulableManager->Register(fEventCountDet2);

    // Setup G4AnalysisManager for thread-safe ROOT output
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetDefaultFileType("root");
    analysisManager->SetVerboseLevel(0);

    // Enable ntuple merging for multi-threading
    analysisManager->SetNtupleMerging(true);

    // Create ntuple for event-by-event data
    analysisManager->CreateNtuple("Tree", "All detector events from dual HPGe detectors");
    analysisManager->CreateNtupleDColumn("e1");  // Detector 1 energy (keV)
    analysisManager->CreateNtupleDColumn("e2");  // Detector 2 energy (keV)
    analysisManager->FinishNtuple();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

RunAction::~RunAction()
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Run* RunAction::GenerateRun()
{
    return new Run;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::BeginOfRunAction(const G4Run*)
{
    // inform the runManager to save random number seed
    G4RunManager::GetRunManager()->SetRandomNumberStore(false);

    // reset accumulables to their initial values
    G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
    accumulableManager->Reset();

    // Open ROOT output file
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->OpenFile("output.root");

    G4cout << "\n-------- Starting Run (Dual Detector System) --------" << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::EndOfRunAction(const G4Run* run)
{
    G4int nofEvents = run->GetNumberOfEvent();
    if (nofEvents == 0) return;

    // Merge accumulables
    G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
    accumulableManager->Merge();

    // Compute dose for detector 1
    G4double energyDepositDet1 = fEnergyDepositDet1.GetValue();
    G4double energyDepositDet12 = energyDepositDet1 * energyDepositDet1;
    G4int eventCountDet1 = fEventCountDet1.GetValue();
    
    G4double rmsDet1 = 0.;
    if (eventCountDet1 > 0) {
        rmsDet1 = energyDepositDet12 - energyDepositDet1*energyDepositDet1/eventCountDet1;
        if (rmsDet1 > 0.) rmsDet1 = std::sqrt(rmsDet1); 
    }

    // Compute dose for detector 2  
    G4double energyDepositDet2 = fEnergyDepositDet2.GetValue();
    G4double energyDepositDet22 = energyDepositDet2 * energyDepositDet2;
    G4int eventCountDet2 = fEventCountDet2.GetValue();
    
    G4double rmsDet2 = 0.;
    if (eventCountDet2 > 0) {
        rmsDet2 = energyDepositDet22 - energyDepositDet2*energyDepositDet2/eventCountDet2;
        if (rmsDet2 > 0.) rmsDet2 = std::sqrt(rmsDet2);
    }

    const DetectorConstruction* detectorConstruction
     = static_cast<const DetectorConstruction*>
       (G4RunManager::GetRunManager()->GetUserDetectorConstruction());

    // Get masses (assuming both detectors have same mass)
    G4double mass1 = detectorConstruction->GetScoringVolume1()->GetMass();
    G4double mass2 = detectorConstruction->GetScoringVolume2()->GetMass();
    
    G4double doseDet1 = (eventCountDet1 > 0) ? energyDepositDet1/mass1 : 0.;
    G4double rmsDoseDet1 = (eventCountDet1 > 0) ? rmsDet1/mass1 : 0.;
    G4double doseDet2 = (eventCountDet2 > 0) ? energyDepositDet2/mass2 : 0.;
    G4double rmsDoseDet2 = (eventCountDet2 > 0) ? rmsDet2/mass2 : 0.;

    // Run conditions
    const PrimaryGeneratorAction* generatorAction
     = static_cast<const PrimaryGeneratorAction*>
       (G4RunManager::GetRunManager()->GetUserPrimaryGeneratorAction());
    G4String runCondition;
    if (generatorAction)
    {
      const G4ParticleGun* particleGun = generatorAction->GetParticleGun();
      runCondition += particleGun->GetParticleDefinition()->GetParticleName();
      runCondition += " of ";
      G4double particleEnergy = particleGun->GetParticleEnergy();
      runCondition += G4BestUnit(particleEnergy,"Energy");
    }

    // Print results for both detectors
    if (IsMaster()) {
        G4cout
         << G4endl
         << "-------- End of Global Run (Dual Detector) --------"
         << G4endl
         << " The run consists of " << nofEvents << " events"
         << G4endl
         << " Detector angle: " << detectorConstruction->GetDetector2Angle() << " degrees"
         << G4endl << G4endl;
         
        G4cout << "=== DETECTOR 1 RESULTS ===" << G4endl;
        G4cout << " Events with energy deposit: " << eventCountDet1 << G4endl;
        G4cout << " Cumulative energy deposit: "
         << G4BestUnit(energyDepositDet1,"Energy") << " rms = "
         << G4BestUnit(rmsDet1,"Energy")
         << G4endl
         << " Dose in scoring volume : "
         << G4BestUnit(doseDet1,"Dose") << " rms = "
         << G4BestUnit(rmsDoseDet1,"Dose")
         << G4endl;
         
        G4cout << "=== DETECTOR 2 RESULTS ===" << G4endl;
        G4cout << " Events with energy deposit: " << eventCountDet2 << G4endl;
        G4cout << " Cumulative energy deposit: "
         << G4BestUnit(energyDepositDet2,"Energy") << " rms = "
         << G4BestUnit(rmsDet2,"Energy")
         << G4endl
         << " Dose in scoring volume : "
         << G4BestUnit(doseDet2,"Dose") << " rms = "
         << G4BestUnit(rmsDoseDet2,"Dose")
         << G4endl
         << "------------------------------------"
         << G4endl
         << G4endl;
    }
    
    // Write and close ROOT file
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->Write();
    analysisManager->CloseFile();

    // Print final results and write spectrum files for both detectors
    Run* localRun = (Run*)run;
    if (IsMaster()) {
        localRun->PrintResults();
    }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::AddEnergyDepositDet1(G4double edep)
{
    fEnergyDepositDet1 += edep;
    if (edep > 0.) fEventCountDet1 += 1;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::AddEnergyDepositDet2(G4double edep)
{
    fEnergyDepositDet2 += edep;
    if (edep > 0.) fEventCountDet2 += 1;
}