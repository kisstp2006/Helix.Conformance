#pragma once

#include <Helix/Core/Strings/Format.h>
#include <Helix/Engine/Script/ScriptClass.h>
#include <Helix/Engine/Script/ScriptRuntime.h>
#include <Helix/Scripting/Bindings/Interop.h>
#include <Helix/Scripting/Types/Type.h>
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

    // A class of the module C# classes derive from (not a script): C++ calls its virtual Area, which a C# class
    // overrides. Its objects live until Destroy; C# reaches them through the module's C# bindings.
    HX_CLASS(Root, Namespace = "Conformance")
    class CppShape
    {
    public:
        HX_FUNCTION(Constructor) explicit CppShape(StringView label);
        virtual ~CppShape();
        CppShape(const CppShape&) = delete;
        CppShape& operator=(const CppShape&) = delete;

        HX_PROPERTY() StringView label() const { return label_; }
        HX_FUNCTION() virtual ::helix::f32 Area() const { return 0; }
        HX_FUNCTION() void Destroy();
        // The sum of every live shape's Area (C++ calling the C# overrides), and their count.
        HX_FUNCTION() static ::helix::f32 TotalArea();
        HX_FUNCTION() static i32 Count();

        ::helix::u64 Id() const { return id_; }
        ::helix::u64& Wrapper() { return wrapper_; }
        virtual ::helix::scripting::TypeId HelixType() const;
        static CppShape* FromId(::helix::u64 id);

    private:
        String label_;
        ::helix::u64 id_ = 0;
        ::helix::u64 wrapper_ = 0;
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

// Shapes cross to C# as their id, with their class's type as dynamic type.
template <>
struct helix::scripting::ObjectHandles<Conformance::CppShape>
{
    static u64 ToId(const Conformance::CppShape* shape) { return shape->Id(); }
    static Conformance::CppShape* FromId(u64 id) { return Conformance::CppShape::FromId(id); }
    static TypeId TypeOf(const Conformance::CppShape* shape) { return shape->HelixType(); }
    static u64& WrapperOf(Conformance::CppShape* shape) { return shape->Wrapper(); }
};
