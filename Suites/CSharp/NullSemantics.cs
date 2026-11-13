using HelixEngine;

namespace Conformance
{
    /// <summary>
    /// The null semantics of the reference: a destroyed object equals null (after the frame for Destroy, at once for
    /// DestroyImmediate) and converts to false while the C# reference stays; its members throw MissingReferenceException;
    /// Find skips it; a missing component and an unset field are null; destroying a component leaves its GameObject.
    /// </summary>
    public class NullSuite : GameBehaviour
    {
        public GameObject unset;

        int m_Updates;
        GameObject m_Target;
        Probe m_TargetProbe;

        void Update()
        {
            m_Updates++;
            if (m_Updates == 1)
            {
                m_Target = new GameObject("CsNullTarget");
                m_TargetProbe = m_Target.AddComponent<Probe>();
                Destroy(m_Target);
                Report.Case("null.destroy-keeps-the-object-for-the-frame", m_Target != null && (bool)m_Target && GameObject.Find("CsNullTarget") == m_Target);
                var now = new GameObject("CsNullImmediate");
                DestroyImmediate(now);
                Report.Case("null.destroyimmediate-at-once", now == null && !now);
                Report.Case("null.missing-component", GetComponent<Camera>() == null && !TryGetComponent<Camera>(out _));
                Report.Case("null.unset-field", unset == null && !unset);
                var host = new GameObject("CsNullHost");
                Probe probe = host.AddComponent<Probe>();
                DestroyImmediate(probe);
                Report.Case("null.destroying-a-component-keeps-its-gameobject", probe == null && host != null && host.GetComponent<Probe>() == null);
                DestroyImmediate(host);
            }
            else if (m_Updates == 2)
            {
                Report.Case("null.destroyed-equals-null", m_Target == null && !m_Target && m_Target.Equals(null) && m_TargetProbe == null);
                Report.Case("null.the-reference-stays", !ReferenceEquals(m_Target, null));
                Report.Case("null.find-skips-destroyed", GameObject.Find("CsNullTarget") == null);
                string thrown = "none";
                try
                {
                    thrown = m_Target.name;
                }
                catch (MissingReferenceException)
                {
                    thrown = "MissingReferenceException";
                }
                Report.Case("null.members-of-destroyed-throw", thrown == "MissingReferenceException", thrown);
            }
        }
    }
}
