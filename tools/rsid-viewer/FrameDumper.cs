using System;
using System.IO;
using System.Windows.Media.Imaging;
using System.Linq;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Drawing.Imaging;

namespace rsid_wrapper_csharp
{
    internal class FrameDumper
    {
        private static readonly TimeSpan MaxFrameAge = TimeSpan.FromMinutes(1);
        private static readonly string Raw10Extension = ".w10";
        private static readonly string ImageExtension = ".png";
        private readonly string _dumpDir;

        public FrameDumper(string dumpDir)
        {
            // create sub folder for current session.            
            var unixMilliSeconds = ((DateTimeOffset)DateTime.Now).ToUnixTimeMilliseconds();
            _dumpDir = Path.Combine(dumpDir, $"session_{unixMilliSeconds}");

            Directory.CreateDirectory(_dumpDir);
            // delete all old frames at start of session
            CleanOldDumps(TimeSpan.Zero);
        }

        public void DumpPreviewImage(rsid.PreviewImage image)
        {
            using (var bmp = new Bitmap(image.width, image.height, image.stride, PixelFormat.Format24bppRgb, image.buffer))
            {
                var bmpSrc = ToBitmapSource(bmp);
                var timestampAvailable = image.metadata.timestamp != 0;
                var filename = timestampAvailable ? $"timestamp_{image.metadata.timestamp}" : $"frame_{image.number}";
                var fullPath = Path.Combine(_dumpDir, filename + ImageExtension);
                DumpBitmapImage(bmpSrc, fullPath);
            }
            // clean old dumps and mark algo images every 900 frames (~1/min on 15 fps)
            if (image.number % 900 == 0)
            {
                CleanOldDumps(MaxFrameAge);
            }
        }

        // Save raw debug frame to dump dir, and the following metadata:
        //    Frame timestamp in micros
        //    Face detection status
        //    sensorID , led , projector
        public void DumpRawImage(rsid.PreviewImage image)
        {
            string filename;
            var timestampAvailable = image.metadata.timestamp != 0;
            var prefix = timestampAvailable ? $"timestamp_{image.metadata.timestamp}" : $"frame_{image.number}";
            if (timestampAvailable)
            {
                uint status = image.metadata.status;
                var sensorStr = (image.metadata.sensor_id != 0) ? "right" : "left";
                var ledStr = (image.metadata.led) ? "led_on" : "led_off";
                var projectorStr = (image.metadata.projector) ? "projector_on" : "projector_off";
                filename = $"{prefix}_status_{status}_{sensorStr}_{projectorStr}_{ledStr}{Raw10Extension}";
            }
            else // metadata isn't valid
            {
                filename = $"{prefix}{Raw10Extension}";
            }

            var byteArray = new Byte[image.size];
            Marshal.Copy(image.buffer, byteArray, 0, image.size);

            var fullPath = Path.Combine(_dumpDir, filename);
            using (var fs = new FileStream(fullPath, FileMode.Create, FileAccess.Write))
            {
                fs.Write(byteArray, 0, byteArray.Length);
            }
            // clean old dumps every 100 frames 
            if (image.number % 100 == 0)
            {
                CleanOldDumps(MaxFrameAge);
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
            var encoder = new PngBitmapEncoder();
            encoder.Frames.Add(BitmapFrame.Create(bitmap));
            using (var stream = new FileStream(fullPath, FileMode.Create))
            {
                encoder.Save(stream);
            }
        }


        // Delete frames older than MaxAgeSeconds
        private void CleanOldDumps(TimeSpan maxAge)
        {
            // delete all frames
            if (maxAge == TimeSpan.Zero)
            {
                Directory.GetFiles(_dumpDir).Select(f => new FileInfo(f))
                    .Where(f => (f.Extension == ImageExtension || f.Extension == Raw10Extension))
                    .ToList().ForEach(f => f.Delete());
            }
            else
            {
                //delete old frames
                var expiredTime = DateTime.Now.Subtract(maxAge);
                Directory.GetFiles(_dumpDir).Select(f => new FileInfo(f))
                    .Where(f => f.CreationTime < expiredTime && (f.Extension == ImageExtension || f.Extension == Raw10Extension))
                    .ToList().ForEach(f => f.Delete());
            }
        }
    }
}
