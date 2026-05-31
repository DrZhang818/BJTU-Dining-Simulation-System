# 快速运行指南

```bash
rm -rf build
cmake -S . -B build -DBDSS_BUILD_GUI=OFF
cmake --build build
./build/BDSS --headless --output simulation_log.csv
./build/BDSS_CoreTest
ctest --test-dir build --output-on-failure
```

若在 Windows PowerShell 中运行，可将 `./build/BDSS` 替换为 `.\build\Debug\BDSS.exe` 或 `.\build\BDSS.exe`，具体取决于所使用的 CMake 生成器。
