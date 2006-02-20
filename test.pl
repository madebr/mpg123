#!/usr/bin/perl

open(FH, "|-", "./mpg123 -R -");
select FH; $| = 1;

$e0 = 0;
$e1 = 0;

while(<STDIN>) {
  if($_ =~ /pl/) { print FH "l mp3.mp3\n"; }
  if($_ =~ /st/) { print FH "stop\n"; }
  if($_ =~ /up/) { 
	$e0 = $e0+0.1;
	print FH "eq 0 0 $e0";
	print FH "eq 1 0 $e0";
  }   
  if($_ =~ /dn/) { 
	$e0--;
	print FH "eq 0 0 $e0";
	print FH "eq 1 0 $e0";
  }   
}


