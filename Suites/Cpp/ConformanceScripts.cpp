#include <ConformanceScripts.h>

#include <Helix/Engine/GameObject/GameObject.h>
#include <Helix/Engine/Object/Object.h>

namespace Conformance
{
    Array<String>& Events()
    {
        static Array<String> events;
        return events;
    }

    void Case(StringView name, bool ok, StringView detail)
    {
        Debug::Log(ok ? ::helix::Format("conformance: cpp.{} ok", name) : ::helix::Format("conformance: cpp.{} FAIL {}", name, detail));
    }

    i32 IndexOf(StringView event)
    {
        const Array<String>& e = Events();
        for (::helix::usize i = 0; i < e.Size(); i++)
            if (StringView(e[i]) == event)
                return i32(i);
        return -1;
    }

    bool InOrder(std::initializer_list<StringView> events)
    {
        const Array<String>& e = Events();
        ::helix::usize from = 0;
        for (StringView wanted : events)
        {
            bool found = false;
            for (; from < e.Size() && !found; from++)
                found = StringView(e[from]) == wanted;
            if (!found)
                return false;
        }
        return true;
    }

    String Of(StringView prefix)
    {
        String out;
        for (const String& e : Events())
            if (StringView(e).StartsWith(prefix))
            {
                if (!out.Empty())
                    out.Append(",");
                out.Append(e);
            }
        return out;
    }

    ::helix::engine::GameObject* Native(const ::HelixEngine::GameObject& gameObject)
    {
        return ::helix::engine::EntityIdToObject<::helix::engine::GameObject>(gameObject.GetEntityId());
    }

    void CppProbe::Record(StringView message)
    {
        String owner = gameObject.Get().name;
        Events().Push(::helix::Format("{}:{}", owner, message));
    }

    namespace
    {
        i32 FirstUpdate(StringView name)
        {
            String prefix = ::helix::Format("{}:Update@", name);
            const Array<String>& e = Events();
            for (::helix::usize i = 0; i < e.Size(); i++)
                if (StringView(e[i]).StartsWith(prefix))
                    return i32(i);
            return -1;
        }
    }

    void CppLifecycleSuite::Update()
    {
        using ::helix::Format;
        updates_++;
        if (updates_ == 1)
        {
            firstFrame_ = Time::frameCount;
            Case("lifecycle.inactive-no-awake", Of("CppInactive:").Empty(), Of("CppInactive:"));
            Case("lifecycle.disabled-awake-only", Of("CppDisabled:").View() == StringView("CppDisabled:Awake"), Of("CppDisabled:"));
            inactive.SetActive(true);
            Case("lifecycle.activate-awakes-at-once", Of("CppInactive:").View() == StringView("CppInactive:Awake,CppInactive:OnEnable"), Of("CppInactive:"));
            ::HelixEngine::Component probe = disabled.GetComponent("CppProbe");
            ::HelixEngine::Behaviour::HxFromId(probe.HxId()).enabled = true;
            Case("lifecycle.enable-at-once", Of("CppDisabled:").View() == StringView("CppDisabled:Awake,CppDisabled:OnEnable"), Of("CppDisabled:"));
            added_ = ::HelixEngine::GameObject::New("CppAdded");
            const ::helix::engine::ScriptClass* script = ::helix::engine::FindScriptClass("ConformanceModule", "Conformance", "CppProbe");
            if (script)
                ::helix::engine::AddScript(Native(added_), *script);
            Case("lifecycle.addcomponent-at-once", Of("CppAdded:").View() == StringView("CppAdded:Awake,CppAdded:OnEnable"), Of("CppAdded:"));
        }
        else if (updates_ == 2)
        {
            i32 fixedAt = IndexOf(Format("CppActive:FixedUpdate@{}", firstFrame_)), updateAt = IndexOf(Format("CppActive:Update@{}", firstFrame_)),
                lateAt = IndexOf(Format("CppActive:LateUpdate@{}", firstFrame_));
            Case("lifecycle.phase-order", updateAt >= 0 && lateAt > updateAt && (fixedAt < 0 || fixedAt < updateAt), Of("CppActive:"));
            Object::Destroy(destroyed);
            Case("lifecycle.destroy-waits-for-the-end-of-the-frame", destroyed != nullptr && IndexOf("CppDestroyed:OnDestroy") < 0, Of("CppDestroyed:"));
            Object::DestroyImmediate(immediate);
            Case("lifecycle.destroyimmediate-at-once", immediate == nullptr && InOrder({"CppImmediate:OnDisable", "CppImmediate:OnDestroy"}), Of("CppImmediate:"));
        }
        else if (updates_ == 3)
        {
            bool started = true;
            for (StringView probe : {StringView("CppActive"), StringView("CppInactive"), StringView("CppDisabled"), StringView("CppAdded")})
            {
                i32 start = IndexOf(Format("{}:Start", probe));
                i32 update = FirstUpdate(probe);
                started = started && start >= 0 && update > start &&
                          InOrder({Format("{}:Awake", probe), Format("{}:OnEnable", probe), Format("{}:Start", probe)});
            }
            Case("lifecycle.start-before-first-update", started, Of("Cpp"));
            Case("lifecycle.destroyed-at-the-end-of-the-frame", destroyed == nullptr && InOrder({"CppDestroyed:OnDisable", "CppDestroyed:OnDestroy"}),
                 Of("CppDestroyed:"));
            Case("lifecycle.execution-order", InOrder({Format("order:early@{}", firstFrame_), Format("order:late@{}", firstFrame_)}), Of("order:"));
            Object::Destroy(added_);
        }
    }

    void CppNullSuite::Update()
    {
        updates_++;
        if (updates_ == 1)
        {
            target_ = ::HelixEngine::GameObject::New("CppNullTarget");
            Object::Destroy(target_);
            Case("null.destroy-keeps-the-object-for-the-frame", target_ != nullptr && static_cast<bool>(target_) &&
                                                                    ::HelixEngine::GameObject::Find("CppNullTarget") == target_);
            ::HelixEngine::GameObject now = ::HelixEngine::GameObject::New("CppNullImmediate");
            Object::DestroyImmediate(now);
            Case("null.destroyimmediate-at-once", now == nullptr && !static_cast<bool>(now));
            Case("null.missing-component", gameObject.Get().GetComponent<Camera>() == nullptr);
            Case("null.unset-field", unset == nullptr && !static_cast<bool>(unset));
        }
        else if (updates_ == 2)
        {
            Case("null.destroyed-equals-null", target_ == nullptr && !static_cast<bool>(target_));
            Case("null.find-skips-destroyed", ::HelixEngine::GameObject::Find("CppNullTarget") == nullptr);
        }
    }

    void CrossCpp::OnEnable()
    {
        enables_++;
        ::helix::engine::SendScriptMessage(Native(gameObject.Get()), "CppEnabled", ::helix::scripting::Variant(), ::helix::engine::SendMessageOptions::DontRequireReceiver);
    }

    void CrossCpp::OnDisable()
    {
        disables_++;
        if (gameObject.Get() != nullptr)
            ::helix::engine::SendScriptMessage(Native(gameObject.Get()), "CppDisabled", ::helix::scripting::Variant(),
                                               ::helix::engine::SendMessageOptions::DontRequireReceiver);
    }

    void CrossCpp::FromCs()
    {
        ::helix::engine::SendScriptMessage(Native(gameObject.Get()), "AckFromCpp", ::helix::scripting::Variant::From(i32(42)));
    }

    void CrossCpp::Update()
    {
        updates_++;
        if (updates_ == 1)
        {
            Case("cross.field-refers-to-cs", partner != nullptr && partner.gameObject.Get() == gameObject.Get());
            Case("cross.base-script-message", baseAwake);
            ::helix::engine::SendScriptMessage(Native(gameObject.Get()), "FromCpp", ::helix::scripting::Variant::From(i32(7)));
            Case("cross.sendmessage-both-ways", ack_);
        }
        else if (updates_ == 3)
        {
            // C# disabled and enabled this script in its second Update.
            Case("cross.disabled-and-enabled-by-cs", disables_ == 1 && enables_ == 2, ::helix::Format("{}/{}", disables_, enables_));
            i32 before = csDisabled_;
            partner.enabled = false;
            Case("cross.disable-cs", csDisabled_ == before + 1 && !static_cast<bool>(partner.enabled), ::helix::Format("{}", csDisabled_));
        }
    }
}
