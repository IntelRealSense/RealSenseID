// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

using System;
using System.Text;

namespace rsid_wrapper_csharp
{

    // We use this small helper class, and not a proper JSON serializer
    // in order to simplify project dependencies and set-up.
    // For now, this is sufficient, if future needs will dictate so, 
    // a proper serializer will be added.
    internal class DatabaseSerializer
    {
        public static String[] Serialize(rsid.UserFeatures user)
        {
            String[] ret = new string[8];
            ret[0] = "{";
            ret[1] = "\"userID\":\"" + user.userID + "\",";
            ret[2] = "\"version\":" + user.faceprints.version + ",";
            ret[3] = "\"num of descriptors\":" + user.faceprints.numberOfDescriptors + ",";
            ret[4] = "\"features type\":" + user.faceprints.featuresType + ",";
            StringBuilder sb = new StringBuilder();
            foreach (short f in user.faceprints.avgDescriptor)
                sb.Append(f + ",");
            ret[5] = "\"avgDescriptor\": [" + sb.ToString() + "],";
            sb = new StringBuilder();
            foreach (short f in user.faceprints.origDescriptor)
                sb.Append(f + ",");
            ret[6] = "\"origDescriptor\": [" + sb.ToString() + "],";
            ret[7] = "}";

            return ret;
        }


        public static rsid.UserFeatures Deserialize(string[] user_string)
        {
            try
            {
                rsid.UserFeatures user_features = new rsid.UserFeatures();
                user_features.faceprints = new rsid.Faceprints();
                for (int i = 0; i < user_string.Length; i++)
                {
                    var line = user_string[i].Replace("\"", "");
                    if (line.Contains("userID"))
                        user_features.userID = line.Replace("userID:", "").Replace(",", "");
                    else if (line.Contains("version"))
                        user_features.faceprints.version = Int32.Parse(line.Replace("version:", "").Replace(",", ""));
                    else if (line.Contains("num of descriptors"))
                        user_features.faceprints.numberOfDescriptors = Int32.Parse(line.Replace("num of descriptors:", "").Replace(",", ""));
                    else if (line.Contains("features type"))
                        user_features.faceprints.featuresType = ushort.Parse(line.Replace("features type:", "").Replace(",", ""));
                    else if (line.Contains("avgDescriptor"))
                    {
                        var features_s = line.Replace("avgDescriptor:", "").Replace("[", "").Replace("]", "").Replace(",,", "");
                        var features = features_s.Split(',');
                        user_features.faceprints.avgDescriptor = new short[features.Length];
                        for (int j = 0; j < features.Length; j++)
                        {
                            user_features.faceprints.avgDescriptor[j] = Int16.Parse(features[j]);
                        }
                        if (user_features.faceprints.avgDescriptor.Length != 256)
                            throw new Exception("Descriptor length is invalid");
                    }
                    else if (line.Contains("origDescriptor"))
                    {
                        var features_s = line.Replace("origDescriptor:", "").Replace("[", "").Replace("]", "").Replace(",,", "");
                        var features = features_s.Split(',');
                        user_features.faceprints.origDescriptor = new short[features.Length];
                        for (int j = 0; j < features.Length; j++)
                        {
                            user_features.faceprints.origDescriptor[j] = Int16.Parse(features[j]);
                            if (user_features.faceprints.origDescriptor.Length != 256)
                                throw new Exception("Descriptor length is invalid");
                        }
                    }
                }
                return user_features;
            }
            catch (Exception ex)
            {
                Console.WriteLine("DB file is in incorrect format");
            }
            return new rsid.UserFeatures();
        }
    }
}