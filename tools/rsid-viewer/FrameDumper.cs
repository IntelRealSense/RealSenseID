using System;
using System.IO;
using System.Windows.Media.Imaging;
using System.Linq;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Drawing.Imaging;
using System.Diagnostics;
using System.Threading.Tasks;
using System.Threading;

namespace rsid_wrapper_csharp
{
    internal class FrameDumper
    {
        private static readonly TimeSpan MaxFrameAge = TimeSpan.FromMinutes(1);
        private static readonly string Raw10Extension = ".w10";
        private static readonly string JpgExtension = ".jpg";
        private static string _fileNamePrefix = "";
        private readonly string _dumpDir;
        private object _mutex = new object();

        public FrameDumper(string dumpDir, string title)
        {
            // create sub folder for current session.            
            var unixMilliSeconds = ((DateTimeOffset)DateTime.Now).ToUnixTimeMilliseconds();
            var t = title.Replace(' ', '_');
            _dumpDir = Path.Combine(dumpDir, $"{t}_session_{unixMilliSeconds}");

            Directory.CreateDirectory(_dumpDir);
            _fileNamePrefix = title.Contains("Enroll") ? "REG" : "AUTH";
        }

        public string DumpPreviewImage(rsid.PreviewImage image)
        {
            lock (_mutex)
            {
                using (var bmp = new Bitmap(image.width, image.height, image.stride, PixelFormat.Format24bppRgb, image.buffer))
                {
                    var bmpSrc = ToBitmapSource(bmp);
                    var filename = GetJpgFilename(image);
                    var fullPath = Path.Combine(_dumpDir, filename);
                    DumpBitmapImage(bmpSrc, fullPath);
                    return fullPath;
                }
            }
        }

        // Save raw debug frame to dump dir, and the following metadata:
        //    Frame timestamp in micros
        //    Face detection status
        //    sensorID, led
        public string DumpRawImage(rsid.PreviewImage image)
        {
            lock (_mutex)
            {
                var byteArray = new Byte[image.size];
                Marshal.Copy(image.buffer, byteArray, 0, image.size);
                var filename = GetRaw10Filename(image);
                var fullPath = Path.Combine(_dumpDir, filename);
                using (var fs = new FileStream(fullPath, FileMode.Create, FileAccess.Write))
                {
                    fs.Write(byteArray, 0, byteArray.Length);
                }
                return fullPath;
            }
        }

        
        public void Cancel()
        {
            lock (_mutex)
            {                
                var file = Path.Combine(_dumpDir, "cancelled.txt");
                try
                {
                    using (FileStream fs = File.Create(file)) { }
                }
                catch { }
            }
        }
        
        private static string GetRaw10Filename(rsid.PreviewImage image)
        {
            var timestampAvailable = image.metadata.timestamp != 0;
            var prefix = timestampAvailable ? $"timestamp_{image.metadata.timestamp}" : $"frame_{image.number}";            
            if (timestampAvailable)
            {
                uint exposure = image.metadata.exposure;
                uint gain = image.metadata.gain;
                var ledStr = (image.metadata.led != 0) ? "led_on" : "led_off";
                var sensorStr = (image.metadata.sensor_id != 0) ? "right" : "left";
                uint status = image.metadata.status;
                return $"{_fileNamePrefix}_{prefix}_exp_{exposure}_gain_{gain}_{ledStr}_sensor_{sensorStr}_status_{status}{Raw10Extension}";
            }
            else // metadata isn't valid
            {
                return $"{prefix}{Raw10Extension}";
            }            
        }

        // get filename for jpg snapshots
        private static string GetJpgFilename(rsid.PreviewImage image)
        {
            var timestampAvailable = image.metadata.timestamp != 0;
            var prefix = timestampAvailable ? $"timestamp_{image.metadata.timestamp}" : $"frame_{image.number}";            
            if (timestampAvailable)
            {
                uint exposure = image.metadata.exposure;
                uint gain = image.metadata.gain;                                                
                return $"{_fileNamePrefix}_{prefix}_exp_{exposure}_gain_{gain}{JpgExtension}";

            }
            else // metadata isn't valid
            {
                return $"{prefix}{JpgExtension}";
            }            
        }

        // needed since the orig image is bgr
        private BitmapSource ToBitmapSource(Bitmap bitmap)
        {
            var bitmapData = bitmap.LockBits(
                new Rectangle(0, 0, bitmap.Width, bitmap.Height),
                ImageLockMode.ReadOnly, bitmap.PixelFormat);

            var bitmapSource = BitmapSource.Create(
                bitmapData.Width, bitmapData.Height,
                bitmap.HorizontalResolution, bitmap.VerticalResolution,
                System.Windows.Media.PixelFormats.Rgb24, null,
                bitmapData.Scan0, bitmapData.Stride * bitmapData.Height, bitmapData.Stride);

            bitmap.UnlockBits(bitmapData);

            return bitmapSource;
        }


        private void DumpBitmapImage(BitmapSource bitmap, string fullPath)
        {
            var encoder = new JpegBitmapEncoder();
            encoder.QualityLevel = 100;
            encoder.Frames.Add(BitmapFrame.Create(bitmap));
            using (var stream = new FileStream(fullPath, FileMode.Create))
            {
                encoder.Save(stream);
            }            
        }


    }
}
