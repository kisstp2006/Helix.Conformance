using HelixEngine;

namespace Conformance
{
    /// <summary>The script of the dispatch benchmark: one Update that counts.</summary>
    public class Counter : GameBehaviour
    {
        public int count;

        void Update() => count++;
    }
}
