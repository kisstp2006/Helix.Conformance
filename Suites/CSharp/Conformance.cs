using System;
using System.Collections.Generic;
using HelixEngine;

namespace Conformance
{
    /// <summary>The cases' results (one log line each) and the events the probes record.</summary>
    public static class Report
    {
        public static readonly List<string> Events = new();

        public static void Case(string name, bool ok, string detail = "")
        {
            Debug.Log("conformance: cs." + name + (ok ? " ok" : " FAIL " + detail));
        }

        public static void Record(string text) => Events.Add(text);

        /// <summary>The index of the first event, -1 when there is none.</summary>
        public static int IndexOf(string text) => Events.IndexOf(text);

        /// <summary>Whether the events come in this order (each found after the one before).</summary>
        public static bool InOrder(params string[] events)
        {
            int at = -1;
            foreach (string e in events)
            {
                int next = Events.IndexOf(e, at + 1);
                if (next < 0)
                    return false;
                at = next;
            }
            return true;
        }

        public static string Of(string prefix) => string.Join(",", Events.FindAll(e => e.StartsWith(prefix, StringComparison.Ordinal)));
    }

    /// <summary>Records its messages as "name:Message" (Update and LateUpdate with the frame).</summary>
    public class Probe : GameBehaviour
    {
        void Awake() => Report.Record(gameObject.name + ":Awake");
        void OnEnable() => Report.Record(gameObject.name + ":OnEnable");
        void Start() => Report.Record(gameObject.name + ":Start");
        void FixedUpdate() => Report.Record(gameObject.name + ":FixedUpdate@" + Time.frameCount);
        void Update() => Report.Record(gameObject.name + ":Update@" + Time.frameCount);
        void LateUpdate() => Report.Record(gameObject.name + ":LateUpdate@" + Time.frameCount);
        void OnDisable() => Report.Record(gameObject.name + ":OnDisable");
        void OnDestroy() => Report.Record(gameObject.name + ":OnDestroy");
    }

    [DefaultExecutionOrder(-50)]
    public class Early : GameBehaviour
    {
        void Update() => Report.Record("order:early@" + Time.frameCount);
    }

    [DefaultExecutionOrder(50)]
    public class Late : GameBehaviour
    {
        void Update() => Report.Record("order:late@" + Time.frameCount);
    }
}
