"""Writes the conformance data folder (Data/) and the list of expected cases (Suites/Expected.txt).

The data folder is what a build would leave: settings.asset, assets.index, the script assets (MonoScript) of both
languages' suites and the scene Scenes/Conformance.helix whose GameObjects carry them, with the scripts' fields
referring to other objects of the scene. Run it after changing the suites' scene: python Tools/GenerateData.py
"""

import hashlib
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, "Data")
HEADER = "%YAML 1.1\n%TAG !h! tag:helixengine,2026:\n"

CS = "ConformanceSuites"
CPP = "ConformanceModule"
SCRIPTS = [(CS, n) for n in ("Probe", "Early", "Late", "LifecycleSuite", "NullSuite", "CrossCs")] + \
          [(CPP, n) for n in ("CppProbe", "CppEarly", "CppLate", "CppLifecycleSuite", "CppNullSuite", "CrossCpp")]


def guid(text):
    return hashlib.md5(("helix-conformance:" + text).encode()).hexdigest()


def write(path, text):
    full = os.path.join(DATA, path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


class Scene:
    def __init__(self):
        self.docs = []
        self.next = 100
        self.ids = {}

    def object(self, name, scripts, active=True):
        """A GameObject with a Transform and scripts: (script name, enabled, {field: reference key}). Keys of the
        references are "<object>" for a GameObject and "<object>/<script>" for a script."""
        go = self.next
        self.next += 10
        self.ids[name] = go
        components = [go + 1] + [go + 2 + i for i in range(len(scripts))]
        for i, (script, _, _) in enumerate(scripts):
            self.ids[name + "/" + script] = go + 2 + i
        self.docs.append((go, name, active, scripts, components))

    def text(self):
        out = [HEADER]
        for go, name, active, scripts, components in self.docs:
            out.append(f"--- !h!1 &{go}\nGameObject:\n  m_Component:\n")
            out.extend(f"  - component: {{fileID: {c}}}\n" for c in components)
            out.append(f"  m_Name: {name}\n  m_IsActive: {1 if active else 0}\n")
            out.append(f"--- !h!4 &{go + 1}\nTransform:\n  m_GameObject: {{fileID: {go}}}\n  m_Children: []\n  m_Father: {{fileID: 0}}\n")
            for i, (script, enabled, fields) in enumerate(scripts):
                out.append(f"--- !h!114 &{go + 2 + i}\nGameBehaviour:\n  m_GameObject: {{fileID: {go}}}\n  m_Enabled: {1 if enabled else 0}\n")
                out.append(f"  m_Script: {{fileID: 11500000, guid: {guid(script)}, type: 3}}\n")
                for field, target in fields.items():
                    out.append(f"  {field}: {{fileID: {self.ids[target] if target else 0}}}\n")
        return "".join(out)


def main():
    for assembly, name in SCRIPTS:
        write(f"Scripts/{name}.asset", HEADER + f"--- !h!115 &11500000\nMonoScript:\n  m_Name: {name}\n  m_ExecutionOrder: 0\n"
              f"  m_ClassName: {name}\n  m_Namespace: Conformance\n  m_AssemblyName: {assembly}\n")

    s = Scene()
    for lang, probe, early, late, life, null in (("Cs", "Probe", "Early", "Late", "LifecycleSuite", "NullSuite"),
                                                 ("Cpp", "CppProbe", "CppEarly", "CppLate", "CppLifecycleSuite", "CppNullSuite")):
        s.object(f"{lang}Active", [(probe, True, {})])
        s.object(f"{lang}Inactive", [(probe, True, {})], active=False)
        s.object(f"{lang}Disabled", [(probe, False, {})])
        s.object(f"{lang}Destroyed", [(probe, True, {})])
        s.object(f"{lang}Immediate", [(probe, True, {})])
        s.object(f"{lang}Order", [(early, True, {}), (late, True, {})])
        if lang == "Cs":
            refs = {"inactive": "CsInactive", "disabled": "CsDisabled/Probe", "destroyed": "CsDestroyed/Probe", "immediate": "CsImmediate/Probe"}
        else:
            refs = {"inactive": "CppInactive", "disabled": "CppDisabled", "destroyed": "CppDestroyed", "immediate": "CppImmediate"}
        s.object(f"{lang}Lifecycle", [(life, True, refs)])
        s.object(f"{lang}Null", [(null, True, {"unset": None})])
    # One GameObject with a script of each language, each referring to the other.
    s.object("Cross", [("CrossCs", True, {}), ("CrossCpp", True, {})])
    s.docs[-1][3][0][2]["partner"] = "Cross/CrossCpp"
    s.docs[-1][3][1][2]["partner"] = "Cross/CrossCs"
    write("Scenes/Conformance.helix", s.text())

    write("settings.asset", HEADER + "--- !h!129 &1\nPlayerSettings:\n  companyName: Helix\n  productName: Helix Conformance\n"
          "  bundleVersion: 1.0\n  applicationIdentifier: org.helix.conformance\n  runInBackground: 1\n"
          "--- !h!141 &2\nBuildSettings:\n  scenes:\n  - Scenes/Conformance.helix\n")
    lines = ["# The conformance data folder: every file, its guid and its path (Tools/GenerateData.py)."]
    lines += [f"{guid(name)} Scripts/{name}.asset" for _, name in SCRIPTS]
    lines.append(f"{guid('scene')} Scenes/Conformance.helix")
    write("assets.index", "\n".join(lines) + "\n")
    # The data folder's own assemblies, for the Web's loader.
    write("Managed/assemblies.index", f"Managed/{CS}.dll\n")

    lifecycle = ["inactive-no-awake", "disabled-awake-only", "activate-awakes-at-once", "enable-at-once", "addcomponent-at-once",
                 "start-before-first-update", "phase-order", "destroy-waits-for-the-end-of-the-frame", "destroyimmediate-at-once",
                 "destroyed-at-the-end-of-the-frame", "execution-order"]
    null_cs = ["destroy-keeps-the-object-for-the-frame", "destroyimmediate-at-once", "missing-component", "unset-field",
               "destroying-a-component-keeps-its-gameobject", "destroyed-equals-null", "the-reference-stays", "find-skips-destroyed",
               "members-of-destroyed-throw"]
    null_cpp = ["destroy-keeps-the-object-for-the-frame", "destroyimmediate-at-once", "missing-component", "unset-field",
                "destroyed-equals-null", "find-skips-destroyed"]
    cross_cs = ["field-refers-to-cpp", "base-script-message", "sendmessage-both-ways", "cpp-sent-a-value", "disable-cpp", "enable-cpp"]
    cross_cpp = ["field-refers-to-cs", "base-script-message", "sendmessage-both-ways", "disabled-and-enabled-by-cs", "disable-cs"]
    cases = [f"cs.lifecycle.{c}" for c in lifecycle] + [f"cs.null.{c}" for c in null_cs] + [f"cs.cross.{c}" for c in cross_cs]
    cases += [f"cpp.lifecycle.{c}" for c in lifecycle] + [f"cpp.null.{c}" for c in null_cpp] + [f"cpp.cross.{c}" for c in cross_cpp]
    with open(os.path.join(ROOT, "Suites", "Expected.txt"), "w", encoding="utf-8", newline="\n") as f:
        f.write("# The cases a run must report ok (Tools/GenerateData.py).\n" + "\n".join(cases) + "\n")


if __name__ == "__main__":
    main()
