# GUI parameter panel fixes

This patch addresses Linux/Qt desktop-theme rendering issues seen on Ubuntu 24.04/VMware:

- The parameter page no longer inherits the OS dark palette on white cards.
- The scroll area and card gaps are explicitly painted with the light app background, removing black/gray vertical bands.
- Native QSpinBox/QDoubleSpinBox/QTimeEdit arrow buttons are replaced by explicit `−` and `+` step buttons, avoiding the Ubuntu theme issue where both buttons looked like `−` and the upper area still incremented.
- The parameter grid is changed from four cramped columns to a clearer two-column layout.
- Runtime parameter editing is allowed: while the simulation is running, users can adjust values and click **应用参数** to synchronize the engine.

Rebuild GUI on Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential cmake qt6-base-dev
rm -rf build
cmake -S . -B build -DBDSS_BUILD_GUI=ON
cmake --build build --parallel
./build/BDSS --gui --config resources/default_config.json
```
