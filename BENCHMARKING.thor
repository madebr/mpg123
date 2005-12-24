I (Thomas Orgis) made some tests back then when I decided if there is a point in using another "modern" mp3 decoder for my mixing daemon. This was around Dec. 2004 till Feb. 2005 with current versions (0.59r-thorX in the case of mpg123). What follows is a copy of the DECODERS file in the dermixd distribution:

What decoder should one use?

-----
 MP3
-----

mpg123 is the old-fashioned way, uncertain license, may have some problems with extraordinary files (huge id3v2 tags, other specialities?), but fast. Has EQ control; interactive frontend interface (in parts hacked by me...).

mpg321: is popular these days... interface? speed?

madplay: is becoming popular these days, handles RVA2 by itself... can't provide an interface nor EQ


speed:

decoding Dirty Guitar with NULL output, pentium-optimized (not more):

decoder	user time/s
mpg123	23
madplay(hq)	46
madplay(speed)	34
mpg321(hq)	62
mpg321(speed)	49


So, there is still a strong technical point in using mpg123... even when my 366MHz-Laptop can easily handle several decoders at once with either of the programs, every percent cpu usage drags on the battery... and takes the cpu time from the real work

-----
 OGG
-----

7,9M	/tmp/dirty_guitar-q3.ogg
11M	/tmp/dirty_guitar-q5.ogg
13M	/tmp/dirty_guitar-q6.ogg
15M	/tmp/dirty_guitar-q7.ogg
22M	/tmp/dirty_guitar-q9.ogg
29M	/tmp/dirty_guitar-q10.ogg
14M	/tmp/dirty_guitar-std.mp3
101M	/tmp/dirty_guitar.wav

speed of ogg123:

quality	user time/s
3	31
5	36
6	38
7	39
10	55

So, with similar care as mpg123 concerning some (assembler?) optimization, ogg could well come close to "fast" mp3 decoding.
Does ist sound better, then?
