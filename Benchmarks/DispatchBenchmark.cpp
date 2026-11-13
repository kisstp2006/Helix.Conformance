#include <BenchScripts.h>

#include <Helix/Engine/GameObject/Lifecycle.h>
#include <Helix/Engine/Script/ScriptRuntime.h>
#include <Helix/Scripting/Bindings/Interop.h>
#include <Helix/Scripting/Host/DotnetHost.h>
#include <Helix/Testing/Bench.h>

#include <bit>
#include <cstdio>

HX_BINDINGS_EXPORT void HxBindings_ConformanceBench_Publish();

using namespace helix;
using namespace helix::engine;

// The M6 exit's dispatch measure: one Update phase over 10 000 scripts of one language (the scheduler calls C++
// scripts through function pointers of dense lists, C# ones in one call into C#).
namespace
{
    bool StartCSharp()
    {
        static int state = 0;
        if (state != 0)
            return state > 0;
        state = -1;
        using namespace scripting;
        String error;
        DotnetHostDesc desc;
        desc.runtimeAssembly = HX_RUNTIME_ASSEMBLY;
        if (!DotnetHost::Start(desc, error))
        {
            std::printf("no .NET host: %.*s\n", int(error.Size()), error.Data());
            return false;
        }
        (void)FindType("HelixEngine.Object");
        AssemblyHandle core = DotnetHost::LoadAssembly(HX_CORE_MODULE, error);
        auto start = std::bit_cast<i32 (*)()>(DotnetHost::GetFunction(core, "HelixEngine.Internal.ScriptBridge", "Start", error));
        auto load = std::bit_cast<i32 (*)(const char*, i32)>(DotnetHost::GetFunction(core, "HelixEngine.Internal.ScriptBridge", "LoadScripts", error));
        StringView suites = HX_SUITES;
        if (!start || !load || start() != 0 || load(suites.Data(), i32(suites.Size())) <= 0)
            return false;
        state = 1;
        return true;
    }

    void Run(const ScriptClass& script, testing::BenchState& state)
    {
        Array<GameObject*> objects;
        for (u32 i = 0; i < 10000; i++)
        {
            GameObject* g = GameObject::Create("Counter");
            AddScript(g, script);
            objects.Push(g);
        }
        RunScriptPhase(ScriptMessage::Update);
        while (state.KeepRunning())
            RunScriptPhase(ScriptMessage::Update);
        for (GameObject* g : objects)
            DestroyImmediate(g);
    }
}

HX_BENCH(Update10kCSharpScripts)
{
    if (!StartCSharp())
        return;
    if (const ScriptClass* counter = FindScriptClass("ConformanceSuites", "Conformance", "Counter"))
        Run(*counter, state);
}

HX_BENCH(Update10kCppScripts)
{
    static bool registered = false;
    if (!registered)
    {
        String error;
        HxBindings_ConformanceBench_Publish();
        registered = RegisterLinkedGameModule("ConformanceBench", error);
        if (!registered)
        {
            std::printf("%.*s\n", int(error.Size()), error.Data());
            return;
        }
    }
    if (const ScriptClass* counter = FindScriptClass("ConformanceBench", "ConformanceBench", "CppCounter"))
        Run(*counter, state);
}
