using HelixEngine;

namespace Conformance
{
    /// <summary>A C# base script: its Awake is the message of scripts deriving from it.</summary>
    public class CrossBase : GameBehaviour
    {
        protected bool baseAwake;

        void Awake() => baseAwake = true;
    }

    /// <summary>A C# class deriving from the C++ module's CppShape: C++ calls its Area.</summary>
    public class CsSquare : CppShape
    {
        readonly float m_Side;

        public CsSquare(float side) : base("square") => m_Side = side;

        public override float Area() => m_Side * m_Side;
    }

    /// <summary>
    /// The C# half of the cross language suite, beside CrossCpp on one GameObject: its field refers to the C++ script
    /// from the scene file; SendMessage to C++ answered by C++ with SendMessage; C# disables and enables the C++ script
    /// (C++ tells its OnDisable and OnEnable by message); the messages of a base script.
    /// </summary>
    public class CrossCs : CrossBase
    {
        public GameBehaviour partner;

        int m_Updates;
        int m_Ack = -1;
        int m_FromCpp = -1;
        int m_CppDisabled;
        int m_CppEnabled;
        CsSquare m_Square;

        void AckFromCpp(int value) => m_Ack = value;
        void CppDisabled() => m_CppDisabled++;
        void CppEnabled() => m_CppEnabled++;
        void OnDisable()
        {
            if (partner)
                partner.SendMessage("CsDisabled", SendMessageOptions.DontRequireReceiver);
        }

        void FromCpp(int value)
        {
            m_FromCpp = value;
            partner.SendMessage("AckFromCs", true);
        }

        void Update()
        {
            m_Updates++;
            if (m_Updates == 1)
            {
                Report.Case("cross.field-refers-to-cpp", partner != null && partner.gameObject == gameObject && !ReferenceEquals(partner, this) &&
                            partner.GetType() != typeof(CrossCs));
                Report.Case("cross.base-script-message", baseAwake);
                partner.SendMessage("FromCs");
                Report.Case("cross.sendmessage-both-ways", m_Ack == 42, m_Ack.ToString());
                // The C++ script's own class through the module's C# bindings, and a C# class deriving from a C++ one.
                Report.Case("cross.cpp-script-class-in-cs", GetComponent<CrossCpp>() != null && GetComponent<CrossCpp>() == partner);
                m_Square = new CsSquare(3);
                Report.Case("cross.derives-from-a-cpp-class", CppShape.Count() == 1 && CppShape.TotalArea() == 9f && m_Square.label == "square",
                            CppShape.Count() + " " + CppShape.TotalArea());
            }
            else if (m_Updates == 2)
            {
                Report.Case("cross.cpp-sent-a-value", m_FromCpp == 7, m_FromCpp.ToString());
                // CrossCpp called the override in its second Update (before this one).
                m_Square.Destroy();
                int disabledBefore = m_CppDisabled, enabledBefore = m_CppEnabled;
                partner.enabled = false;
                Report.Case("cross.disable-cpp", m_CppDisabled == disabledBefore + 1 && !partner.enabled, m_CppDisabled.ToString());
                partner.enabled = true;
                Report.Case("cross.enable-cpp", m_CppEnabled == enabledBefore + 1 && partner.enabled, m_CppEnabled.ToString());
            }
        }
    }
}
