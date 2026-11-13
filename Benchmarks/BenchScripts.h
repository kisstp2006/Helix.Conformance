#pragma once

#include <HelixEngine/HelixEngineApi.h>

// The C++ script of the dispatch benchmark (the benchmark program's own game module, ConformanceBench).
namespace ConformanceBench
{
    HX_CLASS(Namespace = "ConformanceBench")
    class CppCounter : public ::HelixEngine::GameBehaviour
    {
    public:
        ::helix::i32 count = 0;

        void Update() { count++; }
    };
}
