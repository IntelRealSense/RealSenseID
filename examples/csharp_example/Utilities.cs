// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
using System.Collections.Generic;
using System.Management;

using rsid;

namespace rsid_wrapper_csharp
{
    // Find serial com ports that connected to f45x devices    
    class DeviceEnumerator
    {
        public List<rsid.SerialConfig> Enumerate()
        {
            var results = new List<rsid.SerialConfig>();

            using (var searcher = new ManagementObjectSearcher("Select * From Win32_SerialPort"))
            {
                foreach (ManagementObject query in searcher.Get())
                {                    
                    try
                    {
                        var deviceId = query[PnpDeviceIdField].ToString();

                        var vidIndex = deviceId.IndexOf(VidField);
                        var pidIndex = deviceId.IndexOf(PidField);

                        if (vidIndex == -1 || pidIndex == -1)
                            continue;

                        var vid = string.Empty;
                        var pid = string.Empty;

                        // extract com port
                        var serialPort = query[DeviceIdField].ToString();
                                
                        var vidStart = deviceId.Substring(vidIndex + VidField.Length);
                        vid = vidStart.Substring(0, VidLength);

                        string pidStart = deviceId.Substring(pidIndex + PidField.Length);
                        pid = pidStart.Substring(0, PidLength);

                        // use vid and pid to decide if it is connected to device's usb or device's uart 
                        foreach (var id in DeviceIds)
                        {
                            if (vid == id.vid && pid == id.pid)
                                results.Add(new rsid.SerialConfig { port = serialPort, serialType = id.serialType });
                        }
                    }
                    catch (ManagementException)
                    {
                    }
                }
            }

            return results;
        }

        private struct AnnotatedSerialPort
        {
            public string vid;
            public string pid;
            public SerialType serialType;
        };

        private static readonly string VidField = "VID_";
        private static readonly string PidField = "PID_";
        private static readonly string DeviceIdField = "DeviceID";
        private static readonly string PnpDeviceIdField = "PNPDeviceID";
        private static readonly int VidLength = 4;
        private static readonly int PidLength = 4;
        private static readonly List<AnnotatedSerialPort> DeviceIds = new List<AnnotatedSerialPort> {
                new AnnotatedSerialPort{ vid = "04D8", pid = "00DD", serialType = rsid.SerialType.UART },
                new AnnotatedSerialPort{ vid = "2AAD", pid = "6373", serialType = rsid.SerialType.USB },
            };
    }
}
