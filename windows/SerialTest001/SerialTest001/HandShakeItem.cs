using System.IO.Ports;

namespace SerialTest001
{
    internal class HandShakeItem
    {
        public string NAME = "";
        public Handshake HANDSHAKE;

        public override string ToString()
        {
            return NAME;
        }
    }
}