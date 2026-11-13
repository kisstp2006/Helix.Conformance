using HelixEngine;

namespace Conformance
{
    /// <summary>
    /// The lifecycle order of the reference: Awake, OnEnable and Start before the first Update; FixedUpdate, Update and
    /// LateUpdate in that order within a frame; an inactive GameObject's scripts and a disabled script wait; activating,
    /// enabling and AddComponent call Awake and OnEnable at once and Start before the next Update; Destroy acts at the end
    /// of the frame, DestroyImmediate at once; DefaultExecutionOrder orders the scripts' Updates.
    /// </summary>
    public class LifecycleSuite : GameBehaviour
    {
        public GameObject inactive;
        public Probe disabled;
        public Probe destroyed;
        public Probe immediate;

        int m_Updates;
        int m_FirstFrame;
        Probe m_Added;

        void Update()
        {
            m_Updates++;
            if (m_Updates == 1)
            {
                m_FirstFrame = Time.frameCount;
                Report.Case("lifecycle.inactive-no-awake", Report.Of("CsInactive:") == "", Report.Of("CsInactive:"));
                Report.Case("lifecycle.disabled-awake-only", Report.Of("CsDisabled:") == "CsDisabled:Awake", Report.Of("CsDisabled:"));
                inactive.SetActive(true);
                Report.Case("lifecycle.activate-awakes-at-once", Report.Of("CsInactive:") == "CsInactive:Awake,CsInactive:OnEnable", Report.Of("CsInactive:"));
                disabled.enabled = true;
                Report.Case("lifecycle.enable-at-once", Report.Of("CsDisabled:") == "CsDisabled:Awake,CsDisabled:OnEnable", Report.Of("CsDisabled:"));
                m_Added = new GameObject("CsAdded").AddComponent<Probe>();
                Report.Case("lifecycle.addcomponent-at-once", Report.Of("CsAdded:") == "CsAdded:Awake,CsAdded:OnEnable", Report.Of("CsAdded:"));
            }
            else if (m_Updates == 2)
            {
                string f = "@" + m_FirstFrame;
                int fixedAt = Report.IndexOf("CsActive:FixedUpdate" + f), updateAt = Report.IndexOf("CsActive:Update" + f),
                    lateAt = Report.IndexOf("CsActive:LateUpdate" + f);
                Report.Case("lifecycle.phase-order", updateAt >= 0 && lateAt > updateAt && (fixedAt < 0 || fixedAt < updateAt), Report.Of("CsActive:"));
                Destroy(destroyed);
                Report.Case("lifecycle.destroy-waits-for-the-end-of-the-frame", destroyed != null && Report.IndexOf("CsDestroyed:OnDestroy") < 0,
                            Report.Of("CsDestroyed:"));
                DestroyImmediate(immediate.gameObject);
                Report.Case("lifecycle.destroyimmediate-at-once", immediate == null && Report.InOrder("CsImmediate:OnDisable", "CsImmediate:OnDestroy"),
                            Report.Of("CsImmediate:"));
            }
            else if (m_Updates == 3)
            {
                bool started = true;
                foreach (string name in new[] { "CsActive", "CsInactive", "CsDisabled", "CsAdded" })
                {
                    int start = Report.IndexOf(name + ":Start");
                    int update = Report.Events.FindIndex(e => e.StartsWith(name + ":Update@", System.StringComparison.Ordinal));
                    started &= start >= 0 && update > start && Report.InOrder(name + ":Awake", name + ":OnEnable", name + ":Start");
                }
                Report.Case("lifecycle.start-before-first-update", started, Report.Of("Cs"));
                Report.Case("lifecycle.destroyed-at-the-end-of-the-frame", destroyed == null && Report.InOrder("CsDestroyed:OnDisable", "CsDestroyed:OnDestroy"),
                            Report.Of("CsDestroyed:"));
                string f = "@" + m_FirstFrame;
                Report.Case("lifecycle.execution-order", Report.InOrder("order:early" + f, "order:late" + f), Report.Of("order:"));
                Destroy(m_Added.gameObject);
            }
        }
    }
}
