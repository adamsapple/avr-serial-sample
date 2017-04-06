
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NotAmp.Protocol
{
    //class Message
    //{
    //}
    // http://schima.hatenablog.com/entry/20090620/1245425357

    [StructLayout(LayoutKind.Explicit)]
    public struct Message
    {
        //[FieldOffset(0)]
        //[MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
        //public byte[] op;

        [FieldOffset(0)]
        public byte op0;

        [FieldOffset(1)]
        public byte op1;

        [FieldOffset(2)]
        public byte op2;

        //[FieldOffset(3)]
        //[MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        //public byte[]   val_c;

        [FieldOffset(3)]
        public Int16 val_i_a;

        [FieldOffset(5)]
        public byte     val_i_b;

        [FieldOffset(6)]
        public byte     val_i_c;

        [FieldOffset(7)]
        //[MarshalAs(UnmanagedType.ByValArray, SizeConst = 1)]
        public byte reserve;



        public Message(string _op)
        {
            op0     = 0;
            op1     = 0;
            op2     = 0;
            val_i_a = 0;
            val_i_b = 0;
            val_i_c = 0;
            reserve = 0;
            op      = _op;
        }

        public Message(string _op,Int16 a, byte b, byte c) : this(_op)
        {
            val_i_a = a;
            val_i_b = b;
            val_i_c = c;
        }

        public string val_c
        {
            get
            {
                var bytes = new byte[]{ (byte)(val_i_a>>8), (byte)(val_i_a&0xff), val_i_b, val_i_c };
                Encoding ascii = Encoding.ASCII;
                return ascii.GetString(bytes);
            }
            set
            {
                Encoding ascii = Encoding.ASCII;
                var bytes = ascii.GetBytes(value);
                //val_i_a = (Int16)((bytes[0]<<8) | bytes[1]);
                val_i_a = (Int16)((bytes[1] << 8) | bytes[0]);
                val_i_b = bytes[2];
                val_i_c = bytes[3];
            }
        }

        public string op
        {
            get
            {
                var bytes = new byte[] { op0, op1, op2 };
                Encoding ascii = Encoding.ASCII;
                return ascii.GetString(bytes);
            }
            set
            {
                Encoding ascii = Encoding.ASCII;
                var bytes = ascii.GetBytes(value);
                op0 = bytes[0];
                op1 = bytes[1];
                op2 = bytes[2];
            }
        }

        public static Message ToStruct(byte[] bytes)
        {
            GCHandle hGC = GCHandle.Alloc(bytes, GCHandleType.Pinned);
            var result = (Message)Marshal.PtrToStructure(hGC.AddrOfPinnedObject(), typeof(Message));
            hGC.Free();


            //result.val_i_a = (Int16)(((result.val_i_a & 0xff)<<8) | ((result.val_i_a>>8)&0xff));

            return result;
        }

        public static byte[] ToBytes(Message obj)
        {
            int size = Marshal.SizeOf(typeof(Message));
            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(obj, ptr, false);

            byte[] bytes = new byte[size];
            Marshal.Copy(ptr, bytes, 0, size);
            Marshal.FreeHGlobal(ptr);

            //byte swap = bytes[3];
            //bytes[3] = bytes[4];
            //bytes[4] = swap;

            return bytes;
        }

        public override string ToString()
        {
            return $"{base.ToString()} op={op} ({val_i_a}, {val_i_b}, {val_i_c})";
        }
    }

    static class SerialPortNapExtensions
    {
        public static void Write(this System.IO.Ports.SerialPort serial, Message msg)
        {
            byte[] data = Message.ToBytes(msg);

            if (serial.BytesToWrite != 0) return;

            serial.Write(data, 0, data.Length);

            Debug.WriteLine(msg);
        }
    }
}

