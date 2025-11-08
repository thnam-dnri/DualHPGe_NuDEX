# Agent Workflow

1. Develop and test changes locally on the workstation (`DualHPGe_local`).
2. Commit and push updates to `git@github.com:thnam-dnri/Double_HPGe_Geant4.git`.
3. On the remote server, pull the latest changes from the repository.
4. Configure and build on the remote server (e.g., `cmake .. && make`) and run the simulations.
5. For machine-specific build flags, create `CMakeLists.local.cmake` (ignored) to override defaults such as `HPGE_CXX_STANDARD`.
