#!/usr/bin/perl

use strict;
use FindBin qw($Bin);

my %v;
my $set;

if(@ARGV == 1)
{
	if($ARGV[0] =~ /^(\d+)\.(\d+)\.(\d+)(.*)$/)
	{
		$v{MAJOR} = $1;
		$v{MINOR} = $2;
		$v{PATCH} = $3;
		$v{SUFFIX} = $4;
		$set = 1;
	} else
	{
		die "$0: Give me a proper version to set!\n";
	}
}

chdir($Bin) or die "$0: cannot cd into $Bin: $!\n";
open(my $i, '<', 'version.h') or die "$0: cannot open version.h: $!\n";
(open(my $o, '>', 'version.h.tmp') or die "$0: cannot write new version.h: $!\n")
	if $set;

while(<$i>)
{
	if(/^(#define\s+MPG123_)(MAJOR|MINOR|PATCH|SUFFIX)(\s+)(.+)$/)
	{
		if($set)
		{
			my $val = $2 eq 'SUFFIX' ? '"'.$v{$2}.'"' : $v{$2};
			$_ = $1.$2.$3.$val."\n";
		} else
		{
			$v{$2} = $4;
		}
	}
	print $o $_ if $set;
}

$v{SUFFIX} =~ s:^"(.*)":$1:
	unless $set;
rename('version.h.tmp', 'version.h')
	if $set;

print "$v{MAJOR}.$v{MINOR}.$v{PATCH}$v{SUFFIX}\n";
