// ==============================================================================
// HPGeDual.cc - Dual HPGe Detector Simulation (Minimal Verbosity Version)
// Geant4 gamma cascade simulation with dual detectors
// ==============================================================================

#include "G4MTRunManager.hh"
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"

#include "G4SystemOfUnits.hh"
#include <iostream>
#include <string>
#include <sstream>
#include <thread>

// Headers for verbosity control
#include "G4NuclearLevelData.hh"
#include "G4DeexPrecoParameters.hh"
#include "G4ProductionCutsTable.hh"
#include "G4PhysicsListHelper.hh"
#include "G4ProcessTable.hh"
#include "G4HadronicProcessStore.hh"
#include "G4CrossSectionDataSetRegistry.hh"
#include "G4ios.hh"

// Global flag for quiet mode
bool g_quietMode = false;

void PrintUsage() {
    G4cout << "\nUsage: " << G4endl;
    G4cout << "  ./DualHPGe_NuDEX [options] [macro_file]" << G4endl;
    G4cout << "\nOptions:" << G4endl;
    G4cout << "  -angle <degrees>    : Angle for second detector (default: 180.0)" << G4endl;
    G4cout << "  -coin               : Generate Co-60 coincidences (2 gammas per event)" << G4endl;
    G4cout << "  -single             : Generate single gammas (1 gamma per event)" << G4endl;
    G4cout << "  -nudex [Z A|ZA]     : NuDEX thermal capture cascades" << G4endl;
    G4cout << "                        Z,A integers (e.g., 24 53 for Cr-53) or ZA=1000*Z+A" << G4endl;
    G4cout << "                        Default if omitted: 17 35 (Cl-35)" << G4endl;
    G4cout << "  -nudex-libdir <path>: Override NuDEX library directory (default: ../NuDEX/NuDEXlib/)" << G4endl;
    // -cascade mode removed
    // RAINIER file mode removed
    G4cout << "  -threads <N>        : Number of threads for parallel execution (default: 1)" << G4endl;
    G4cout << "                        Use 'auto' or 0 to use all available CPU cores" << G4endl;
    G4cout << "  -quiet              : Suppress all non-essential output" << G4endl;
    G4cout << "  -h, --help          : Show this help message" << G4endl;
    G4cout << "\nArguments:" << G4endl;
    G4cout << "  macro_file          : Optional Geant4 macro file (.mac extension)" << G4endl;
    G4cout << "\nExamples:" << G4endl;
    G4cout << "  ./DualHPGe_NuDEX -quiet                    # Silent mode with Co-60 test data" << G4endl;
    G4cout << "  ./DualHPGe_NuDEX -angle 45 -quiet          # Silent mode, 45° detector angle" << G4endl;
    // RAINIER examples removed
    G4cout << "  ./DualHPGe_NuDEX -coin -quiet              # Silent mode with Co-60 coincidences" << G4endl;
    G4cout << "  ./DualHPGe_NuDEX -single -quiet            # Silent mode with single gammas" << G4endl;
    // -cascade examples removed
    
    G4cout << "\n" << G4endl;
}

int main(int argc, char** argv)
{
    // Parse command line arguments first to check for quiet mode
    bool quietMode = false;
    bool cascadeMode = true;  // Default to cascade mode for coincidence analysis
    SourceMode sourceMode = CO60_CASCADE;  // Default source mode
    G4double detector2Angle = 180.0;
    std::string macroFile = "";
    // NuDEX configuration
    int nudexZA = 17035; // Default: Cl-35 target
    std::string nudexLibDir = "../NuDEX/NuDEXlib/";

    // -cascade parameters removed

    // Multi-threading parameter
    G4int nThreads = 1;  // Default: single-threaded

    // RAINIER mode removed

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            PrintUsage();
            return 0;
        }
        else if (arg == "-quiet" || arg == "-q") {
            quietMode = true;
        }
        else if (arg == "-coin") {
            cascadeMode = true;
            sourceMode = CO60_CASCADE;
        }
        else if (arg == "-single") {
            cascadeMode = false;
            sourceMode = SINGLE_GAMMA;
        }
        else if (arg == "-nudex-libdir") {
            if (i + 1 < argc) {
                nudexLibDir = argv[i + 1];
                i++;
            } else {
                if (!quietMode) {
                    G4cout << "Error: -nudex-libdir requires a path argument" << G4endl;
                }
                return 1;
            }
        }
        else if (arg == "-nudex") {
            sourceMode = NUDEX_CAPTURE;
            // Optional Z A or ZA argument
            auto parseInt = [](const std::string& s, int& out) -> bool {
                if (s.empty()) return false;
                char* endp = nullptr;
                long v = strtol(s.c_str(), &endp, 10);
                if (endp && *endp == '\0') { out = static_cast<int>(v); return true; }
                return false;
            };
            if (i + 2 < argc) {
                int z = -1, a = -1;
                if (parseInt(argv[i + 1], z) && parseInt(argv[i + 2], a) && z > 0 && a > 0) {
                    nudexZA = z * 1000 + a;
                    i += 2;
                } else if (i + 1 < argc) {
                    int tmpZA = -1;
                    if (parseInt(argv[i + 1], tmpZA) && tmpZA > 0) {
                        nudexZA = tmpZA;
                        i += 1;
                    }
                }
            } else if (i + 1 < argc) {
                int tmpZA = -1;
                if (parseInt(argv[i + 1], tmpZA) && tmpZA > 0) {
                    nudexZA = tmpZA;
                    i += 1;
                }
            }
        }
        // -cascade mode removed
        // RAINIER mode removed
        else if (arg == "-angle") {
            if (i + 1 < argc) {
                std::stringstream ss(argv[i + 1]);
                if (!(ss >> detector2Angle)) {
                    if (!quietMode) G4cout << "Error: Invalid angle value '" << argv[i + 1] << "'" << G4endl;
                    detector2Angle = 180.0;
                }
                i++;
            }
        }
        else if (arg == "-threads") {
            if (i + 1 < argc) {
                std::string threadArg = argv[i + 1];
                if (threadArg == "auto" || threadArg == "0") {
                    // Use all available CPU cores
                    nThreads = std::thread::hardware_concurrency();
                } else {
                    std::stringstream ss(threadArg);
                    if (!(ss >> nThreads) || nThreads < 1) {
                        if (!quietMode) G4cout << "Error: Invalid threads value '" << threadArg << "'" << G4endl;
                        nThreads = 1;
                    }
                }
                i++;
            }
        }
        // RAINIER filter removed
        else if (arg.find(".mac") != std::string::npos) {
            macroFile = arg;
        }
        else if (macroFile.empty()) {
            macroFile = arg;
        }
        else {
            if (!quietMode) G4cout << "Warning: Ignoring unrecognized argument: " << arg << G4endl;
        }
    }

    // Set the global quiet mode flag
    g_quietMode = quietMode;

    // NuDEX currently runs safest in single-threaded mode
    if (sourceMode == NUDEX_CAPTURE && nThreads > 1) {
        if (!quietMode) {
            G4cout << "NuDEX mode selected: forcing single-thread execution for stability." << G4endl;
        }
        nThreads = 1;
    }

    // Print startup info only if not in quiet mode
    if (!quietMode) {
        G4cout << "\n========================================" << G4endl;
        G4cout << "  Dual HPGe Detector Simulation" << G4endl;
        G4cout << "========================================\n" << G4endl;

        G4cout << "Configuration:" << G4endl;
        G4cout << "  Detector 2 angle: " << detector2Angle << " degrees" << G4endl;

        std::string modeStr;
        if (sourceMode == CO60_CASCADE) {
            modeStr = "Co-60 Cascade (2 gammas/event)";
        } else if (sourceMode == SINGLE_GAMMA) {
            modeStr = "Single gamma (1 gamma/event)";
        } else {
            int zDisp = nudexZA / 1000; int aDisp = nudexZA % 1000;
            modeStr = "NuDEX thermal capture (Z=" + std::to_string(zDisp) + ", A=" + std::to_string(aDisp) + ")";
            G4cout << "  NuDEX libdir: " << nudexLibDir << G4endl;
        }
        G4cout << "  Generation mode: " << modeStr << G4endl;
        if (!macroFile.empty()) {
            G4cout << "  Macro file: " << macroFile << G4endl;
        }
        G4cout << G4endl;
    }

    // Create run manager (MT or ST depending on nThreads)
    G4RunManager* runManager = nullptr;
    if (nThreads > 1) {
        G4MTRunManager* mtRunManager = new G4MTRunManager();
        mtRunManager->SetNumberOfThreads(nThreads);
        runManager = mtRunManager;
        if (!quietMode) {
            G4cout << "Multi-threading enabled with " << nThreads << " threads" << G4endl;
        }
    } else {
        runManager = new G4RunManager();
        if (!quietMode) {
            G4cout << "Single-threaded mode" << G4endl;
        }
    }

    // COMPREHENSIVE VERBOSITY SUPPRESSION
    // ===================================
    
    // 1. Nuclear de-excitation parameters
    G4DeexPrecoParameters* deex = G4NuclearLevelData::GetInstance()->GetParameters();
    deex->SetVerbose(0);
    
    // 2. Production cuts table
    G4ProductionCutsTable::GetProductionCutsTable()->SetVerboseLevel(0);
    
    // 3. Physics list helper
    G4PhysicsListHelper::GetPhysicsListHelper()->SetVerboseLevel(0);
    
    // 4. Hadronic process store
    G4HadronicProcessStore::Instance()->SetVerbose(0);
    
    // 5. Cross section registry (method may not exist in all Geant4 versions)
    // G4CrossSectionDataSetRegistry::Instance()->SetVerboseLevel(0);
    
    // 6. Process table
    G4ProcessTable::GetProcessTable()->SetVerboseLevel(0);

    // Set mandatory initialization classes
    DetectorConstruction* detConstruction = new DetectorConstruction(detector2Angle);
    runManager->SetUserInitialization(detConstruction);

    PhysicsList* physicsList = new PhysicsList();
    runManager->SetUserInitialization(physicsList);

    // Use ActionInitialization for MT-safe action setup
    ActionInitialization* actionInitialization =
        new ActionInitialization(cascadeMode, sourceMode, nudexZA, nudexLibDir);
    runManager->SetUserInitialization(actionInitialization);

    // Initialize visualization (only if not quiet mode)
    G4VisManager* visManager = nullptr;
    if (!quietMode) {
        visManager = new G4VisExecutive("Quiet"); // Use quiet visualization
        visManager->Initialize();
    }

    // Get the pointer to the User Interface manager
    G4UImanager* UImanager = G4UImanager::GetUIpointer();

    // COMPREHENSIVE UI COMMAND SUPPRESSION
    // ===================================
    UImanager->ApplyCommand("/process/verbose 0");
    UImanager->ApplyCommand("/geometry/navigator/verbose 0");
    UImanager->ApplyCommand("/particle/verbose 0");
    UImanager->ApplyCommand("/run/verbose 0");
    UImanager->ApplyCommand("/event/verbose 0");
    UImanager->ApplyCommand("/tracking/verbose 0");
    UImanager->ApplyCommand("/process/em/verbose 0");
    UImanager->ApplyCommand("/process/had/verbose 0");
    UImanager->ApplyCommand("/cuts/verbose 0");
    UImanager->ApplyCommand("/material/verbose 0");
    UImanager->ApplyCommand("/physics_lists/verbose 0");
    
    // Additional quiet mode suppressions
    if (quietMode) {
        UImanager->ApplyCommand("/run/printProgress 0");
        UImanager->ApplyCommand("/control/verbose 0");
        UImanager->ApplyCommand("/vis/verbose 0");
        UImanager->ApplyCommand("/vis/scene/verbose 0");
        UImanager->ApplyCommand("/vis/sceneHandler/verbose 0");
        UImanager->ApplyCommand("/vis/viewer/verbose 0");
    }

    // Batch vs interactive
    bool batchMode = !macroFile.empty();

    if (batchMode) {
        G4String command = "/control/execute ";
        UImanager->ApplyCommand(command + macroFile);
    } else {
        if (!quietMode) {
            G4UIExecutive* ui = new G4UIExecutive(argc, argv);
            UImanager->ApplyCommand("/control/execute init_vis.mac");
            if (ui->IsGUI()) {
                UImanager->ApplyCommand("/control/execute gui.mac");
            }
            ui->SessionStart();
            delete ui;
        } else {
            // In quiet mode without macro, just show final message
            if (!quietMode) {
                G4cout << "Quiet mode: Use -h for help or provide a macro file for batch execution." << G4endl;
            }
        }
    }

    // Clean up
    if (visManager) delete visManager;
    delete runManager;

    // Final message (always shown unless completely silent)
    if (!quietMode) {
        G4cout << "\nDual detector simulation completed successfully!" << G4endl;
        G4cout << "Output files:" << G4endl;
        G4cout << "  - gamma_spectrum_det1.dat (Detector 1 at +Z axis)" << G4endl;
        G4cout << "  - gamma_spectrum_det2.dat (Detector 2 at " << detector2Angle << "°)" << G4endl;
    }
    
    return 0;
}
