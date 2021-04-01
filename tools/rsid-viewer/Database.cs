// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;

namespace rsid_wrapper_csharp
{
    internal class Database
    {
        public Database()
        {
            faceprintsArray = new List<(rsid.Faceprints, string)>();
            lastIndex = 0;
            isDone = (faceprintsArray.Count == 0);

            var baseDir = System.AppDomain.CurrentDomain.BaseDirectory;
            dbPath = Path.Combine(baseDir, "db");
        }

        public bool Push(rsid.Faceprints faceprints, string userId)
        {
            if (DoesUserExist(userId))
                return false;
            else
            {
                faceprintsArray.Add((faceprints, userId));
                isDone = false;
                return true;
            }
        }

        public bool DoesUserExist(string userId)
        {
            return (faceprintsArray.Any(item => item.Item2 == (userId + '\0')));
        }

        public bool Remove(string userId)
        {
            int removedItems = faceprintsArray.RemoveAll(r => r.Item2 == userId);
            return (removedItems > 0);
        }

        public bool RemoveAll()
        {
            faceprintsArray.Clear();
            return (faceprintsArray.Count == 0);
        }

        public (rsid.Faceprints, string, bool) GetNext()
        {
            if (isDone)
            {
                Console.WriteLine("Scanned all faceprints in db");
                return (new rsid.Faceprints(), "done", true);
            }
            if (faceprintsArray.Count == 0)
            {
                Console.WriteLine("Db is empty, can't get next faceprints");
                return (new rsid.Faceprints(), "empty", true);
            }

            if (lastIndex >= faceprintsArray.Count)
                lastIndex = 0;

            var nextUser = faceprintsArray[lastIndex];
            var nextFaceprints = nextUser.Item1;
            var nextUserId = nextUser.Item2;

            lastIndex++;
            if (lastIndex >= faceprintsArray.Count)
            {
                lastIndex = 0;
                isDone = true;
            }

            return (nextFaceprints, nextUserId, false);
        }

        public void ResetIndex()
        {
            lastIndex = 0;
            isDone = isDone = (faceprintsArray.Count == 0);
        }

        public void GetUserIds(out string[] userIds)
        {
            int arrayLength = faceprintsArray.Count;
            userIds = new string[arrayLength];
            for (var i = 0; i < arrayLength; i++)
                userIds[i] = faceprintsArray[i].Item2;
        }

        public void Save()
        {
            try
            {
                FileStream stream = new FileStream(dbPath, FileMode.Create);
                BinaryFormatter formatter = new BinaryFormatter();
                formatter.Serialize(stream, faceprintsArray);
                stream.Close();
            }
            catch (System.Exception e)
            {
                System.Console.WriteLine($"Failed saving database: {e.Message}");
            }
        }

        public void Load()
        {
            try
            {
                Console.WriteLine("Loading database ...");
                if (!File.Exists(dbPath))
                {
                    Console.WriteLine("Database file is missing, using an empty database.");
                    return;
                }
                FileStream inStr = new FileStream(dbPath, FileMode.Open);
                BinaryFormatter bf = new BinaryFormatter();
                faceprintsArray = bf.Deserialize(inStr) as List<(rsid.Faceprints, string)>;
            }
            catch (System.Exception e)
            {
                System.Console.WriteLine($"Failed loading database: {e.Message}");
            }
        }

        public List<(rsid.Faceprints, string)> faceprintsArray;
        public int lastIndex;
        public bool isDone;
        public string dbPath;
    }

}
