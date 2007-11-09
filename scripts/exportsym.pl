#!/usr/bin/perl
print STDERR "I am filtering the EXPORT symbols out of some header you give me to STDIN.\n";
while(<STDIN>)
{
	if(/^EXPORT\s+([^\(\[]+)/)
	{
		my $sym = $1;
		# some type symname 
		$sym =~ s/^.*\s(\S+)\s*$/$1/;
		$sym =~ s/^\*+//;
		print $sym,"\n";
	}
}
