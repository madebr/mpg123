This is the README for the mpg123-thor package, which ceased to exist with the new mpg123.
This file is still here for historic reference and perhaps for some info that is not yet in another proper place.
+++++++++++++++++++++++++++++++++++++++++++++++

This is mpg123-0.59r with various patches from others and some work of me, Thomas Orgis (<thomas@orgis.org>).
Please see the file NEWS for what I did and whose patches I used.

The intend of my work on mpg123 was another project of mine: DerMixD, a tcp-controlled audio playing demon similar to and inspired by mixplayd (http://mixplayd.sf.net), but multi-threaded and written in C++ with classes that make extension more easy. This daemon includes a decoder backend for mp3s using mpg123 in remote mode. Thus, it is a "generic" front-end for mpg123.

Another point in working on this package came up with the desire to listen to music on my Compaq Alpha machine at work (sorry, so working Output for MME, yet - raw decoding works, though); some specific bugs needed fixing. 

Mpg123 needed some changes in its beaviour to be really useful for that (and maybe for someone else, too...).
The control interface (the "API") has been modified and extended a bit; thus the id string on starting is not 

@R MPG123

but

@R MPG123 (ThOr)

so that DerMixD (or who else) can see if there is a mpg123 that will work as expected.

I did not limit the functionality on the package in any way (I hope). Plus, I included bug fixes from Debian (see NEWS). 

So, there is no reason to not consider this package for general (not only for DerMixD backend) use. One exception may be a frontend that relies too heavily on the exact behaviour of the control_generic interface.

Summary of changes to interface
+++++++++++++++++++++++++++++++

- command must be ended by line break (of any fashion), otherwise:

@E Unfinished command: line-without-end

- failed load does not exit() program, see two load attemps (load commands are the input, of course)

load fg
fg: No such file or directory
@E Error opening stream: fg
@P 0
load http://hhh
Unknown host "hhh".
@E Error opening stream: http://hhh
@P 0

- added SILENCE command; suppresses playback info
- EQ command is there (see README.cfa, associated test.pl slightly modified to have line ends on every command)
- also a SEQ (simple eq) command is there:

seq 1.2 3 4
@bass: 1.200000 mid: 3.000000 treble: 4.000000

- JUMP returns the actual reached position, not just the desired
- slightly different "unknown command" messages:

sdfsdf
@E Unknown command or no arguments: sdfsdf
df ff
@E Unknown command: df
seq r 4.3
@E invalid arguments for SEQ: r 4.3
