# Helix.Conformance

The conformance suites of the Helix engine: documented behavior of the script API, as C# scripts and as C++ scripts,
run by the player itself in batch mode without graphics.

## Contents

| Path | Content |
|---|---|
| `Suites/CSharp/` | The C# suites (ConformanceSuites.dll): `LifecycleSuite`, `NullSuite`, `CrossCs` with the probes they watch |
| `Suites/Cpp/` | The C++ suites (the game module ConformanceModule, on the C++ script API): `CppLifecycleSuite`, `CppNullSuite`, `CrossCpp` |
| `Suites/Expected.txt` | Every case a run must report ok |
| `Data/` | The data folder the player runs, as a build would leave it: the settings, the scene with the suites' GameObjects, the script assets, the index (written by `Tools/GenerateData.py`) |
| `Tests/RunConformance.cmake` | Runs the player on the data folder and checks its log |
| `Benchmarks/` | One Update phase over 10 000 C# scripts and over 10 000 C++ scripts |

## Suites

- Lifecycle: Awake, OnEnable and Start before the first Update; FixedUpdate, Update and LateUpdate in order; inactive
  GameObjects and disabled scripts wait; SetActive, enabling and AddComponent call Awake and OnEnable at once and
  Start before the next Update; Destroy at the end of the frame, DestroyImmediate at once; the execution order.
- Null semantics: a destroyed object equals null (after the frame for Destroy, at once for DestroyImmediate) and is
  false; C# keeps the reference and its members throw MissingReferenceException; Find skips it; missing components
  and unset fields are null; destroying a component keeps its GameObject.
- Across the languages, on one GameObject: each script's field refers to the other's from the scene file; SendMessage
  both ways with values; each disables and enables the other and hears it; messages of base scripts; C# reaches the
  C++ script through its own class and derives a class (`CsSquare`) from the module's C++ class `CppShape`, whose
  override C++ calls. The C# suites compile the module's C# bindings (Helix.Bindgen's output for ConformanceModule).

Each case logs `conformance: <cs|cpp>.<suite>.<case> ok` (or `FAIL` with what it saw). The tests run the player
executable on Windows and Linux (with the hostfxr and the coreclr runtime starters) and the player's web app in node
on the Web (the data folder mounted from the disk); Android builds the APK with the suites.

## Build

```
git clone https://github.com/kisstp2006/Helix.Conformance
git -C Helix.Conformance submodule update --init
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
```

The .NET 11 SDK is needed (`HELIX_DOTNET_ROOT` or `DOTNET_ROOT`); the Web needs its `wasm-tools` workload.

## License

GPL-3.0, see `LICENSE`.
