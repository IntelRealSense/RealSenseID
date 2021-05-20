using System;
using System.IO;
using System.Windows.Media.Imaging;
using System.Linq;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text.RegularExpressions;
using System.Collections.Generic;

namespace rsid_wrapper_csharp
{
    internal class FrameDumper
    {
        private static readonly TimeSpan MaxFrameAge = TimeSpan.FromMinutes(1);
        private static readonly UInt32 MaxTimestampDiffMilli = 40;
        private static readonly string Raw10Extension = ".w10";
        private static readonly string ImageExtension = ".png";
        private static readonly string SelectedStr = "selected";
        private string _dumpDir;

        public FrameDumper(string dumpDir)
        {
            // create sub folder for current session.            
            var unixSeconds = ((DateTimeOffset)DateTime.Now).ToUnixTimeSeconds();
            _dumpDir = Path.Combine(dumpDir, $"session_{unixSeconds}");

            Directory.CreateDirectory(_dumpDir);
            // delete all old frames at start of session
            CleanOldDumps(TimeSpan.Zero);
        }

        private bool IsCloseTimestamp(UInt32 ts_a, UInt32 ts_b)
        {
            return Math.Abs(ts_b - ts_a) < MaxTimestampDiffMilli;
        }

        private UInt32 TimeStampFromFileName(string fileName)
        {
            fileName = Path.GetFileNameWithoutExtension(fileName);
            var parts = fileName.Split('_');
            return UInt32.Parse(parts[1]);
        }


        // replace selected files (timestamp_xxx.png -> timestamp_xxx_selected.png)
        // selected file is any file with timestamp close (40ms) to given face timestamps
        public void MarkSelectedPreviewImage(List<UInt32> facesTs)
        {
            var allFrames = Directory.GetFiles(_dumpDir).Select(f => new FileInfo(f))
                              .Where(f => f.Name.StartsWith("timestamp_") && (f.Extension == ImageExtension));

            foreach (var ts in facesTs)
            {
                var selectedFrames = allFrames.Where(f => !f.Name.Contains(SelectedStr) && IsCloseTimestamp(TimeStampFromFileName(f.Name), ts));
                foreach (var f in selectedFrames)
                {
                    var newName = $"{Path.GetFileNameWithoutExtension(f.Name)}_{SelectedStr}{f.Extension}";
                    var newPath = Path.Combine(f.DirectoryName, newName);
                    f.MoveTo(newPath);
                }
            }
        }

        public void DumpPreviewImage(rsid.PreviewImage image)
        {
            using (var bmp = new Bitmap(image.width, image.height, image.stride, System.Drawing.Imaging.PixelFormat.Format24bppRgb, image.buffer))
            {
                var bmpSrc = ToBitmapSource(bmp);
                var timestamp_available = image.metadata.timestamp != 0;
                var filename = timestamp_available ? $"timestamp_{image.metadata.timestamp}" : $"frame_{image.number}";
                var fullPath = Path.Combine(_dumpDir, filename + ImageExtension);
                DumpBitmapImage(bmpSrc, image.metadata, fullPath);
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
            var timestamp_available = image.metadata.timestamp != 0;
            var prefix = timestamp_available ? $"timestamp_{image.metadata.timestamp}" : $"frame_{image.number}";
            if (timestamp_available)
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


        private void DumpBitmapImage(BitmapSource bitmap, rsid.PreviewImageMetadata metadata, string fullPath)
        {
            var encoder = new PngBitmapEncoder();
            encoder.Frames.Add(BitmapFrame.Create(bitmap));
            using (FileStream stream = new FileStream(fullPath, FileMode.Create))
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
