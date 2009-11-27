/*
	replacereaderclr: test program for mpg123clr, showing how to use ReplaceReader in a CLR enviro.
	copyright 2009 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org	

    initially written by Malcolm Boczek
  
    not to be used as an example of good coding practices, note the total absence of error handling!!!  
*/

/*
	1.9.0.0 24-Sep-09	Function names harmonized with libmpg123 (mb)
*/

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using mpg123clr;

namespace ReplaceReaderclr
{
    class Program
    {
        private unsafe static int MyReadFunc(int a, void* b, uint c)
        {
            // need to call posix read function here...

            // PosixRead is an example, substitute your replacement function here.
            int ret = mpg123.PosixRead(a, b, c);

            return ret;
        }

        private static int MySeekFunc(int a, int b, int c)
        {
            // need to call posix lseek function here...

            // PosixSeek is an example, substitute your replacement function here.
            int ret = mpg123.PosixSeek(a, b, c);

            return ret;
        }

        static unsafe void Main(string[] args)
        {
            if (args.Length == 0)
            {
                Console.WriteLine("I need a file to work on:\n\nPress any key to exit.");
                while (Console.Read() == 0) ;
                return;
            }
            mpg123clr.mpg.ErrorCode err;

            err = mpg123.mpg123_init();
            Console.WriteLine("Init:");

            mpg123 mp = new mpg123();
            err = mp.mpg123_new();

            // ReplaceReader example
            mpg123clr.mpg123.ReadDelegate rdel = MyReadFunc;
            mpg123clr.mpg123.SeekDelegate sdel = MySeekFunc;
            err = mp.mpg123_replace_reader(rdel, sdel);

            err = mp.mpg123_open(args[0]);

            if (err != mpg123clr.mpg.ErrorCode.ok)
            {
                Console.WriteLine("Error: " + mp.mpg123_strerror());
            }
            else
            {
                Console.WriteLine("Open:");

                // Show available decoders
                string[] Decoders = mp.mpg123_decoders();

                if (Decoders.Length > 0)
                {
                    Console.WriteLine("\nDecoders:");
                    foreach (string str in Decoders) Console.WriteLine(str);
                }

                // Show supported decoders
                string[] supDecoders = mp.mpg123_supported_decoders();

                if (supDecoders.Length > 0)
                {
                    Console.WriteLine("\nSupported Decoders:");
                    foreach (string str in supDecoders) Console.WriteLine(str);
                }

                // Show actual decoder
                Console.WriteLine("\nDecoder: " + mp.mpg123_current_decoder());

                // Show estimated file length
                Console.WriteLine("\nLength Estimate: " + mp.mpg123_length().ToString());

                // Scan - gets actual details including ID3v2 and Frame offsets
                err = mp.mpg123_scan();

                // Show actual file length
                if (err == mpg123clr.mpg.ErrorCode.ok) Console.WriteLine("Length Actual  : " + mp.mpg123_length().ToString());

                // Get ID3 data
                mpg123clr.id3.mpg123id3v1 iv1;
                mpg123clr.id3.mpg123id3v2 iv2;
                err = mp.mpg123_id3(out iv1, out iv2);

                // Show ID3v2 data
                Console.WriteLine("\nTitle  : " + iv2.title);
                Console.WriteLine("Artist : " + iv2.artist);
                Console.WriteLine("Album  : " + iv2.album);
                Console.WriteLine("Comment: " + iv2.comment);
                Console.WriteLine("Year   : " + iv2.year);

                // Demo seek (back to start of file - note: scan should already have done this)
                long pos = mp.mpg123_seek(0, System.IO.SeekOrigin.Begin);

                long[] frameindex;
                long step;
                err = mp.mpg123_index(out frameindex, out step);

                if (err == mpg123clr.mpg.ErrorCode.ok)
                {
                    Console.WriteLine("\nFrameIndex:");
                    foreach (long idx in frameindex)
                    {
                        // Console.WriteLine(idx.ToString());
                    }
                }

                int num;
                uint cnt;
                IntPtr audio;

                // Walk the file - effectively decode the data without using it...
                Console.WriteLine("\nWalking  : " + iv2.title);
                DateTime dte, dts = DateTime.Now;

                while (err == mpg123clr.mpg.ErrorCode.ok || err == mpg123clr.mpg.ErrorCode.new_format)
                {
                    err = mp.mpg123_decode_frame(out num, out audio, out cnt);

                    // do something with "audio" here....
                }

                dte = DateTime.Now;

                TimeSpan ts = dte - dts;
                Console.WriteLine("Duration:  " + ts.ToString());

            }

            Console.WriteLine("\nPress any key to exit:");
            while (Console.Read() == 0) ;

            mp.mpg123_close();
            mp.Dispose();

            mpg123.mpg123_exit();
        }
    }
}
