#pragma once

#include <Helix/Core/Strings/Format.h>
#include <Helix/Engine/Script/ScriptClass.h>
#include <Helix/Engine/Script/ScriptRuntime.h>
#include <HelixEngine/HelixEngineApi.h>

// The C++ conformance suites (the game module ConformanceModule): the same cases as the C# suites, on the C++ script
// API, beside them in the data folder's scene.
namespace Conformance
{
    using namespace ::HelixEngine;
    using ::helix::Array;
    using ::helix::String;
    using ::helix::StringView;
    using ::helix::i32;

    // The events the probes record and the cases' results (one log line each).
    Array<String>& Events();
    void Case(StringView name, bool ok, StringView detail = StringView());
    i32 IndexOf(StringView event);
    bool InOrder(std::initializer_list<StringView> events);
    String Of(StringView prefix);
    // The engine's GameObject of a handle (for what the script API leaves to the engine: SendMessage, AddScript).
    ::helix::engine::GameObject* Native(const ::HelixEngine::GameObject& gameObject);

    // Records its messages as "name:Message" (Update and LateUpdate with the frame).
    HX_CLASS(Namespace = "Conformance")
    class CppProbe : public ::HelixEngine::GameBehaviour
    {
    public:
        void Awake() { Record("Awake"); }
        void OnEnable() { Record("OnEnable"); }
        void Start() { Record("Start"); }
        void FixedUpdate() { Record(::helix::Format("FixedUpdate@{}", i32(Time::frameCount))); }
        void Update() { Record(::helix::Format("Update@{}", i32(Time::frameCount))); }
        void LateUpdate() { Record(::helix::Format("LateUpdate@{}", i32(Time::frameCount))); }
        void OnDisable() { Record("OnDisable"); }
        void OnDestroy() { Record("OnDestroy"); }

    private:
        void Record(StringView message);
    };

    HX_CLASS(Namespace = "Conformance", Order = -50)
    class CppEarly : public ::HelixEngine::GameBehaviour
    {
    public:
        void Update() { Events().Push(::helix::Format("order:early@{}", i32(Time::frameCount))); }
    };

    HX_CLASS(Namespace = "Conformance", Order = 50)
    class CppLate : public ::HelixEngine::GameBehaviour
    {
    public:
        void Update() { Events().Push(::helix::Format("order:late@{}", i32(Time::frameCount))); }
    };

    // The lifecycle order (see the C# LifecycleSuite).
    HX_CLASS(Namespace = "Conformance")
    class CppLifecycleSuite : public ::HelixEngine::GameBehaviour
    {
    public:
        HX_FIELD() ::HelixEngine::GameObject inactive;
        HX_FIELD() ::HelixEngine::GameObject disabled;
        HX_FIELD() ::HelixEngine::GameObject destroyed;
        HX_FIELD() ::HelixEngine::GameObject immediate;

        void Update();

    private:
        i32 updates_ = 0;
        i32 firstFrame_ = 0;
        ::HelixEngine::GameObject added_;
    };

    // The null semantics (see the C# NullSuite).
    HX_CLASS(Namespace = "Conformance")
    class CppNullSuite : public ::HelixEngine::GameBehaviour
    {
    public:
        HX_FIELD() ::HelixEngine::GameObject unset;

        void Update();

    private:
        i32 updates_ = 0;
        ::HelixEngine::GameObject target_;
    };

    // A C++ base script: its Awake is the message of scripts deriving from it.
    HX_CLASS(Namespace = "Conformance")
    class CppCrossBase : public ::HelixEngine::GameBehaviour
    {
    public:
        void Awake() { baseAwake = true; }

    protected:
        bool baseAwake = false;
    };

    // The C++ half of the cross language suite (see the C# CrossCs).
    HX_CLASS(Namespace = "Conformance")
    class CrossCpp : public CppCrossBase
    {
    public:
        HX_FIELD() ::HelixEngine::Behaviour partner;

        void Update();
        void OnEnable();
        void OnDisable();
        // Messages from C#.
        void FromCs();
        void AckFromCs(bool value) { ack_ = value; }
        void CsDisabled() { csDisabled_++; }

    private:
        i32 updates_ = 0;
        bool ack_ = false;
        i32 csDisabled_ = 0;
        i32 enables_ = 0;
        i32 disables_ = 0;
    };
}
