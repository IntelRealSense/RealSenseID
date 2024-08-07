// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Management;
using System.Runtime.InteropServices;
using System.Web.Script.Serialization;
using System.Windows.Media.Imaging;

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

                        // extract com port
                        var serialPort = query[DeviceIdField].ToString();

                        var vidStart = deviceId.Substring(vidIndex + VidField.Length);
                        var vid = vidStart.Substring(0, VidLength);

                        string pidStart = deviceId.Substring(pidIndex + PidField.Length);
                        var pid = pidStart.Substring(0, PidLength);

                        // use vid and pid to decide if it is connected to device's usb or device's uart 
                        foreach (var id in DeviceIds)
                        {
                            if (vid == id.Vid && pid == id.Pid)
                                results.Add(new rsid.SerialConfig { port = serialPort });
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
            public string Vid;
            public string Pid;
        };

        private static readonly string VidField = "VID_";
        private static readonly string PidField = "PID_";
        private static readonly string DeviceIdField = "DeviceID";
        private static readonly string PnpDeviceIdField = "PNPDeviceID";
        private static readonly int VidLength = 4;
        private static readonly int PidLength = 4;
        private static readonly List<AnnotatedSerialPort> DeviceIds = new List<AnnotatedSerialPort> {
                new AnnotatedSerialPort{ Vid = "04D8", Pid = "00DD" },
                new AnnotatedSerialPort{ Vid = "2AAD", Pid = "6373" },
            };
    }

    class ImageHelper
    {
        // return (result 1d array, width, height, result bitmap)
        // if the results exceeds maxSize, rescale and return the rescaled byte array
        public static Tuple<byte[], int, int, Bitmap> ToBgr(string filename, int maxSize)
        {
            var bmp = new Bitmap(filename);
            FixOrientation(bmp);
            bmp = ResizeToDim(bmp, 320);
            var arr = ToBgr(bmp);
            return Tuple.Create(arr, bmp.Width, bmp.Height, bmp);
        }

        // Resize to dimension while perserving oroginal aspect ratio.
        // Note: If original image is smaller than required dimenstion, return original without scaling
        private static Bitmap ResizeToDim(Bitmap img, int dimenstion)
        {
            if (img.Width <= dimenstion && img.Height <= dimenstion)
                return img;

            int newWidth = 0;
            int newHeight = 0;
            if (img.Width > img.Height)
            {
                newWidth = dimenstion;
                var scaleH = (double)newWidth / (double)img.Width;
                newHeight = (int)Math.Round(img.Height * scaleH);
            }
            else
            {
                newHeight = dimenstion;
                var scaleW = (double)newHeight / (double)img.Height;
                newWidth = (int)Math.Round(img.Width * scaleW);
            }

            Logger.Log($"Resizing image to {newWidth}x{newHeight}");

            var resultBitmap = new Bitmap(newWidth, newHeight);
            using (var g = Graphics.FromImage(resultBitmap))
            {
                g.InterpolationMode = InterpolationMode.HighQualityBicubic;
                g.DrawImage(img, 0, 0, newWidth, newHeight);
            }

            return resultBitmap;
        }

        private static byte[] ToBgr(Bitmap image)
        {
            var bpp = 3;
            var outStride = image.Width * bpp;
            var rgbArray = new byte[image.Height * outStride];
            var data = image.LockBits(new Rectangle(0, 0, image.Width, image.Height),
                ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
            try
            {
                for (var lineNumber = 0; lineNumber < data.Height; lineNumber++)
                {
                    var scanOffset = lineNumber * data.Stride;
                    var outOffset = lineNumber * outStride;
                    Marshal.Copy(data.Scan0 + scanOffset, rgbArray, outOffset, outStride);
                }
            }
            finally
            {
                image.UnlockBits(data);
            }
            return rgbArray;
        }

        public static BitmapImage BitmapToImageSource(Bitmap bitmap)
        {
            using (MemoryStream memoryStream = new MemoryStream())
            {
                bitmap.Save(memoryStream, ImageFormat.Bmp);
                memoryStream.Position = 0;
                var bi = new BitmapImage();
                bi.BeginInit();
                bi.StreamSource = memoryStream;
                bi.CacheOption = BitmapCacheOption.OnLoad;
                bi.EndInit();
                return bi;
            }
        }

        public static System.Windows.Controls.Image CreateImageControl(string fileName)
        {
            var bitmap = new BitmapImage();
            using (var stream = File.OpenRead(fileName))
            {
                bitmap.BeginInit();
                bitmap.CacheOption = BitmapCacheOption.OnLoad;
                bitmap.StreamSource = stream;
                bitmap.EndInit();
            }

            return new System.Windows.Controls.Image
            {
                Source = bitmap,
                Width = bitmap.Width,
                Height = bitmap.Height
            };
        }

        // rotate the image to be top-left orientation if the exif is in different orientation
        public static void FixOrientation(Bitmap bmp)
        {
            const int exifOrientationId = 0x112;
            var props = bmp.PropertyIdList.ToArray();
            if (!props.Contains(exifOrientationId)) return;
            var prop = bmp.GetPropertyItem(exifOrientationId);
            var orientation = BitConverter.ToInt16(prop.Value, 0);
            if (orientation == 1) return; //top left

            // Set Orientation top left
            prop.Value = BitConverter.GetBytes((short)1);
            bmp.SetPropertyItem(prop);

            switch (orientation)
            {
                case 2:
                    bmp.RotateFlip(RotateFlipType.RotateNoneFlipX);
                    break;
                case 3:
                    bmp.RotateFlip(RotateFlipType.Rotate180FlipNone);
                    break;
                case 4:
                    bmp.RotateFlip(RotateFlipType.Rotate180FlipX);
                    break;
                case 5:
                    bmp.RotateFlip(RotateFlipType.Rotate90FlipX);
                    break;
                case 6:
                    bmp.RotateFlip(RotateFlipType.Rotate90FlipNone);
                    break;
                case 7:
                    bmp.RotateFlip(RotateFlipType.Rotate270FlipX);
                    break;
                case 8:
                    bmp.RotateFlip(RotateFlipType.Rotate270FlipNone);
                    break;
            }
        }
    }

    public struct EnrollImageRecord
    {
        public string UserId { get; set; }
        public string Filename { get; set; }
    }

    class JsonHelper
    {
        public static List<EnrollImageRecord> LoadImagesToEnroll(string filename)
        {
            using (var reader = new StreamReader(filename))
            {
                var js = new JavaScriptSerializer();
                return js.Deserialize<List<EnrollImageRecord>>(reader.ReadToEnd());
            }
        }
    }


    class Logger
    {
        private static readonly object ConsoleLock = new object();
        // Log to console with color around the "Info" text to match spdlog console format 
        public static void Log(string message)
        {
            string timestamp = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss.fff");
            lock (ConsoleLock)
            {
                Console.Write($"[{timestamp}] [");
                Console.ForegroundColor = ConsoleColor.Green;
                Console.Write("Info");
                Console.ResetColor();
                Console.WriteLine($"] [Viewer] {message}");
            }
        }
    }
}
