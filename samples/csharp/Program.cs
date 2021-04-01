using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using rsid;
namespace ConsoleApp1
{
    class Program
    {

        // Authentication callbacks
        static void OnAuthHint(rsid.AuthStatus hint, IntPtr ctx)
        {
            Console.WriteLine("OnHint " + hint);
        }

        static void OnAuthResult(rsid.AuthStatus status, string userId, IntPtr ctx)
        {
            Console.WriteLine("OnResults " + status);
            if (status == AuthStatus.Success)
            {
                Console.WriteLine("Authenticated " + userId);
            }
        }

        static void Main(string[] args)
        {
            var auth = new rsid.Authenticator();
            if (auth.Connect(new SerialConfig { port = "COM9" }) != Status.Ok)
            {
                System.Console.WriteLine("Error connecting to device:");
            }
            var authArgs = new AuthArgs { hintClbk = OnAuthHint, resultClbk = OnAuthResult };
            auth.Authenticate(authArgs);
        }
    }
}
