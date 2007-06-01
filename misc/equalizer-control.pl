#!/usr/bin/perl
# test.pl: a little demonstration code for equalizer control over generic control interface

# copyright by the mpg123 project - free software under the terms of the LGPL 2.1
# see COPYING and AUTHORS files in distribution or http://mpg123.org

open(FH, "|-", "./mpg123 -R -");
select FH; $| = 1;

$e0 = 0;
$e1 = 0;

while(<STDIN>)
{
	if($_ =~ /pl/) { print FH "l mp3.mp3\n"; }
	if($_ =~ /st/) { print FH "stop\n"; }
	if($_ =~ /up/)
	{
		$e0 = $e0+0.1;
		print FH "eq 0 0 $e0\n";
		print FH "eq 1 0 $e0\n";
	}
	if($_ =~ /dn/)
	{
		$e0--;
		print FH "eq 0 0 $e0\n";
		print FH "eq 1 0 $e0\n";
	}
}


