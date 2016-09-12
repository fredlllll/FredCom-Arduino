using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Runtime.InteropServices;
using System.Threading;

namespace FredCom
{
    public class FredCom
    {
        enum VarType : byte
        {
            uint8_t = 1,
            uint16_t = 2,
            uint32_t = 3,
            uint64_t = 4,
            int8_t = 5,
            int16_t = 6,
            int32_t = 7,
            int64_t = 8,
            float32_t = 9,
            float64_t = 10,
        }

        [StructLayout(LayoutKind.Explicit)]
        struct VarSendPackage
        {
            [FieldOffset(0)]
            public VarType type;
            [FieldOffset(1)]
            public byte var_uint8_t;
            [FieldOffset(1)]
            public UInt16 var_uint16_t;
            [FieldOffset(1)]
            public UInt32 var_uint32_t;
            [FieldOffset(1)]
            public UInt64 var_uint64_t;
            [FieldOffset(1)]
            public sbyte var_int8_t;
            [FieldOffset(1)]
            public Int16 var_int16_t;
            [FieldOffset(1)]
            public Int32 var_int32_t;
            [FieldOffset(1)]
            public Int64 var_int64_t;
            [FieldOffset(1)]
            public float var_float32_t;
            [FieldOffset(1)]
            public double var_float64_t;
        }

        SerialPort sp;
        BinaryWriter bw;
        BinaryReader br;
        Thread ReaderThread;

        public delegate void receiveMessageDelegate(byte op, List<byte> data);
        public delegate void receiveUInt8Delegate(byte val);
        public delegate void receiveUInt16Delegate(UInt16 val);
        public delegate void receiveUInt32Delegate(UInt32 val);
        public delegate void receiveUInt64Delegate(UInt64 val);
        public delegate void receiveInt8Delegate(sbyte val);
        public delegate void receiveInt16Delegate(Int16 val);
        public delegate void receiveInt32Delegate(Int32 val);
        public delegate void receiveInt64Delegate(Int64 val);
        public delegate void receiveFloat32Delegate(Single val);
        public delegate void receiveFloat64Delegate(Double val);
        public event receiveMessageDelegate receiveMessage;
        public event receiveUInt8Delegate receiveUInt8;
        public event receiveUInt16Delegate receiveUInt16;
        public event receiveUInt32Delegate receiveUInt32;
        public event receiveUInt64Delegate receiveUInt64;
        public event receiveInt8Delegate receiveInt8;
        public event receiveInt16Delegate receiveInt16;
        public event receiveInt32Delegate receiveInt32;
        public event receiveInt64Delegate receiveInt64;
        public event receiveFloat32Delegate receiveFloat32;
        public event receiveFloat64Delegate receiveFloat64;

        public FredCom(String portname, int baudrate)
        {
            sp = new SerialPort(portname, baudrate, Parity.None, 8, StopBits.One);
            sp.Handshake = Handshake.None;
            sp.Open();
            try
            {
                DateTime start = DateTime.Now;
                int to = sp.BaseStream.ReadTimeout;
                sp.BaseStream.ReadTimeout = 1000;
                while((DateTime.Now - start).TotalSeconds < 1)
                {
                    //try to empty buffer
                }
            }
            catch(InvalidOperationException)
            {
                Console.WriteLine("stream doesnt support readtimeout. this is unfortunate, but might still work");
            }
            catch(TimeoutException)
            {

            }
            br = new BinaryReader(sp.BaseStream);
            bw = new BinaryWriter(sp.BaseStream);
            bw.Write((byte)0); //start new frame
            ReaderThread = new Thread(read);
            ReaderThread.Start();
        }

        public void Close()
        {
            ReaderThread.Interrupt();
            sp.Close();
        }

        void read()
        {
            List<byte> payloadList = new List<byte>();
            List<byte> frame = new List<byte>();
            while(sp.IsOpen)
            {
                try
                {
                    byte inp = 0;
                    while((inp = br.ReadByte()) == 0)
                    { }
                    frame.Clear();
                    frame.Add(inp);
                    while((inp = br.ReadByte()) != 0)
                    {
                        frame.Add(inp);
                    }
                    byte[] frameArray = frame.ToArray();
                    frameArray = COBSUnstuffData(frameArray);
                    if(frameArray != null) //cobs cant decode if this is null
                    {
                        payloadList.Clear();
                        byte tid = frameArray[0];
                        byte len = frameArray[1];
                        byte op = frameArray[2];
                        byte checksum = frameArray[frameArray.Length - 1];
                        for(int i = 3; i < frameArray.Length - 1; i++)
                        {
                            payloadList.Add(frameArray[i]);
                        }
                        bool frameValid = true;
                        if(frameValid && len != payloadList.Count)
                        {
                            frameValid = false;
                        }
                        if(frameValid)
                        {
                            byte testChecksum = 127;
                            testChecksum *= tid;
                            testChecksum *= len;
                            testChecksum *= op;
                            foreach(byte b in payloadList)
                            {
                                testChecksum *= b;
                            }
                            if(testChecksum != checksum)
                            {
                                frameValid = false;
                            }
                        }
                        if(frameValid)
                        {
                            switch(op)
                            {
                                case 250:
                                    byte id = payloadList[0];
                                    Console.WriteLine("ACK: " + id);
                                    pastTransmissions.Remove(id);
                                    break;
                                case 251:
                                    id = payloadList[0];
                                    byte reason = payloadList[1];
                                    byte r1 = payloadList[2], r2 = payloadList[3];
                                    Console.WriteLine("NACK: " + id + " Reason: " + reason + " R1: " + r1 + " R2: " + r2);
                                    TransmissionInfo ti = null;
                                    if(pastTransmissions.TryGetValue(id, out ti))
                                    {
                                        SendTransmission(ti);
                                    }
                                    break;
                                case 252:
                                    break;
                                case 253:
                                    break;
                                case 254:
                                    break;
                                case 255:
                                    byte[] arr = payloadList.ToArray();
                                    VarSendPackage pckg = StructFromBytes<VarSendPackage>(arr);
                                    switch(pckg.type)
                                    {
                                        case VarType.uint8_t:
                                            receiveUInt8?.Invoke(pckg.var_uint8_t);
                                            break;
                                        case VarType.uint16_t:
                                            receiveUInt16?.Invoke(pckg.var_uint16_t);
                                            break;
                                        case VarType.uint32_t:
                                            receiveUInt32?.Invoke(pckg.var_uint32_t);
                                            break;
                                        case VarType.uint64_t:
                                            receiveUInt64?.Invoke(pckg.var_uint64_t);
                                            break;
                                        case VarType.int8_t:
                                            receiveInt8?.Invoke(pckg.var_int8_t);
                                            break;
                                        case VarType.int16_t:
                                            receiveInt16?.Invoke(pckg.var_int16_t);
                                            break;
                                        case VarType.int32_t:
                                            receiveInt32?.Invoke(pckg.var_int32_t);
                                            break;
                                        case VarType.int64_t:
                                            receiveInt64?.Invoke(pckg.var_int64_t);
                                            break;
                                        case VarType.float32_t:
                                            receiveFloat32?.Invoke(pckg.var_float32_t);
                                            break;
                                        case VarType.float64_t:
                                            receiveFloat64?.Invoke(pckg.var_float64_t);
                                            break;
                                    }
                                    break;
                                default:
                                    receiveMessage?.Invoke(op, payloadList);
                                    break;
                            }
                        }
                    }
                }
                catch(TimeoutException)
                {

                }
                catch(ThreadInterruptedException)
                {
                    break;
                }
                catch(Exception e)
                {
                    Console.WriteLine("leaving read loop unexpectedly:");
                    Console.WriteLine(e);
                    break;
                }
            }
        }

        Dictionary<byte, TransmissionInfo> pastTransmissions = new Dictionary<byte, TransmissionInfo>();
        class TransmissionInfo
        {
            public byte transmissionID;
            public byte op;
            public byte[] data;
            public int offset;
            public byte len;
            public byte checksum;
        }

        byte transmissionID;
        public void SendMessage(byte op, byte[] data, int offset, byte len)
        {
            TransmissionInfo ti = new TransmissionInfo();
            ti.transmissionID = transmissionID++;
            ti.op = op;
            ti.data = data;
            ti.offset = offset;
            ti.len = len;
            byte checksum = 127;
            checksum *= ti.transmissionID;
            checksum *= ti.len;
            checksum *= ti.op;
            for(int i = offset; i < offset + len; i++)
            {
                checksum *= data[i];
            }
            ti.checksum = checksum;
            pastTransmissions[ti.transmissionID] = ti;
            SendTransmission(ti);
        }

        public void Send(byte val)
        {
            VarSendPackage pckg = new VarSendPackage();
            pckg.type = VarType.uint8_t;
            pckg.var_uint8_t = val;
            Send(pckg);
        }

        public void Send(UInt16 val)
        {
            VarSendPackage pckg = new VarSendPackage();
            pckg.type = VarType.uint16_t;
            pckg.var_uint16_t = val;
            Send(pckg);
        }

        public void Send(UInt32 val)
        {
            VarSendPackage pckg = new VarSendPackage();
            pckg.type = VarType.uint32_t;
            pckg.var_uint32_t = val;
            Send(pckg);
        }

        public void Send(UInt64 val)
        {
            VarSendPackage pckg = new VarSendPackage();
            pckg.type = VarType.uint64_t;
            pckg.var_uint64_t = val;
            Send(pckg);
        }

        public void Send(sbyte val)
        {
            VarSendPackage pckg = new VarSendPackage();
            pckg.type = VarType.int8_t;
            pckg.var_int8_t = val;
            Send(pckg);
        }

        public void Send(Int16 val)
        {
            VarSendPackage pckg = new VarSendPackage();
            pckg.type = VarType.int16_t;
            pckg.var_int16_t = val;
            Send(pckg);
        }

        public void Send(Int32 val)
        {
            VarSendPackage pckg = new VarSendPackage();
            pckg.type = VarType.int32_t;
            pckg.var_int32_t = val;
            Send(pckg);
        }

        public void Send(Int64 val)
        {
            VarSendPackage pckg = new VarSendPackage();
            pckg.type = VarType.int64_t;
            pckg.var_int64_t = val;
            Send(pckg);
        }

        public void Send(float val)
        {
            VarSendPackage pckg = new VarSendPackage();
            pckg.type = VarType.float32_t;
            pckg.var_float32_t = val;
            Send(pckg);
        }

        public void Send(double val)
        {
            VarSendPackage pckg = new VarSendPackage();
            pckg.type = VarType.float64_t;
            pckg.var_float64_t = val;
            Send(pckg);
        }

        void Send(VarSendPackage pckg)
        {
            byte[] bytes = StructToBytes(pckg);
            SendMessage(255, bytes, 0, (byte)bytes.Length);
        }

        void SendTransmission(TransmissionInfo ti)
        {
            List<byte> buffer = new List<byte>();
            buffer.Add(ti.transmissionID);
            buffer.Add(ti.len);
            buffer.Add(ti.op);
            for(int i = ti.offset; i < ti.offset + ti.len; i++)
            {
                buffer.Add(ti.data[i]);
            }
            buffer.Add(ti.checksum);
            byte[] encoded = COBSStuffData(buffer.ToArray());

            bw.Write(encoded, 0, encoded.Length);
            bw.Write((byte)0);
        }

        static byte[] StructToBytes<T>(T str) where T : struct
        {
            int size = Marshal.SizeOf(str);
            byte[] arr = new byte[size];

            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(str, ptr, true);
            Marshal.Copy(ptr, arr, 0, size);
            Marshal.FreeHGlobal(ptr);
            return arr;
        }

        static T StructFromBytes<T>(byte[] arr) where T : struct
        {
            T str = new T();

            int size = Marshal.SizeOf(str);
            IntPtr ptr = Marshal.AllocHGlobal(size);

            Marshal.Copy(arr, 0, ptr, size);

            str = (T)Marshal.PtrToStructure(ptr, str.GetType());
            Marshal.FreeHGlobal(ptr);

            return str;
        }

        static byte[] COBSStuffData(byte[] data)
        {
            List<byte> output = new List<byte>();
            output.Add(1);
            int codePos = 0;

            for(int i = 0; i < data.Length; i++)
            {
                if(data[i] == 0)
                {
                    codePos = output.Count;
                    output.Add(1);
                }
                else
                {
                    output.Add(data[i]);
                    output[codePos]++;
                    if(output[codePos] == 0xFF)
                    {
                        codePos = output.Count;
                        output.Add(1);
                    }
                }
            }
            return output.ToArray();
        }

        static byte[] COBSUnstuffData(byte[] data)
        {
            try
            {
                List<byte> output = new List<byte>();
                byte lastcode = 0;
                for(int i = 0; i < data.Length;)
                {
                    byte code = data[i];
                    if(i != 0 && lastcode != 0xFF) // dont put 0 when lastcode == FF or end of message
                    {
                        output.Add(0);
                    }
                    lastcode = code;
                    i++;
                    for(int j = 1; j < code && i < data.Length; j++) // just check i here too so we dont run out of bounds
                    {
                        output.Add(data[i++]);
                    }
                }
                return output.ToArray();
            }
            catch
            {
                return null;
            }
        }
    }
}
