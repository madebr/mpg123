#!/usr/bin/perl

use File::Basename qw(dirname);
use File::Spec::Functions qw(abs2rel);

my @incdirs = ('src/common', 'src/compat', 'src');

sub header
{
	my ($from, $inc) = @_;
	for(dirname($from), @incdirs)
	{
		my $cand = "$_/$inc";
		return $cand if -e $cand;
	}
	return undef;
}

for my $f (@ARGV)
{
	open(my $ih, '<', $f) or die "meh: $!\n";
	open(my $oh, '>', "$f.tmp") or die "moh: $!\n";
	while(my $line = <$ih>)
	{
		if($line =~ m,^(\s*\#\s*include\s+)"([^/]+)\"(.*)$,)
		{
			my ($pre, $inc, $post) = ($1, $2, $3);
			my $h = header($f, $inc);
			if($inc ne "config.h" and defined $h)
			{
				if(dirname($h) ne dirname($f))
				{
					my $rel = abs2rel($h, dirname($f));
					print "$f: including $inc ($rel)\n";
					$line = $pre.'"'.$rel.'"'.$post."\n";
				}
			}
		}
		print $oh "$line";
	}
	rename("$f.tmp", $f);
}
