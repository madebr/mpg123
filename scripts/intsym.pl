#!/usr/bin/perl

use strict;

print STDERR "Do not forget to handle renaming of symbols from a statically linked output module!\n";

my @instances =
(
{	name => 'intsym.h'
,	guard => 'MPG123_INTSYM_H'
,	dir => 'src'
,	asmdir => 'src/libmpg123'
,	headers => [qw(
		compat/compat
		libmpg123/decode
		libmpg123/dither
		libmpg123/frame
		libmpg123/getbits
		libmpg123/getcpuflags 
		libmpg123/huffman
		libmpg123/icy2utf8
		libmpg123/icy
		libmpg123/id3
		libmpg123/index
		libmpg123/mpg123lib_intern
		libmpg123/optimize
		libmpg123/parse
		libmpg123/reader
		libout123/module
		libout123/buffer
		libout123/xfermem
		libout123/wav
		libout123/out123_int
		libout123/stringlists
	)]
,	prefix => 'INT123_'
,	apiprefix => 'mpg123_|out123_'
,	conditional => { strerror=>'HAVE_STRERROR' }
,	symbols => [qw(COS9 tfcos36 pnts catchsignal)] # extra symbols
}
);

for my $i (@instances)
{
	my $dir = $i->{dir};
	print STDERR "dir: $dir\n";
	my $outfile = "$dir/$i->{name}";
	print STDERR "generating $outfile\n";
	open(my $out, '>', $outfile) or die "Meh.\n";

	my %ident;
	my @symbols = @{$i->{symbols}};
	my $apiex = qr/^$i->{apiprefix}/;

	foreach my $header (@{$i->{headers}})
	{
		print STDERR "==== working on header $header\n";
		open(DAT, '<', $dir.'/'.$header.'.h') or die "Cannot open $header.\n";
		while(<DAT>)
		{ # Only simple declarations parsed here, more configured manually.
			if(/^([^\s\(#][^\(]*)\s\*?([a-z][a-z_0-9]+)\s*\(/)
			{
				# Skip preprocessing/comment stuff and official API.
				unless($1 =~ '^#' or $1 =~ '/\*' or $2 =~ $apiex)
				{
					push(@symbols, $2) unless grep {$_ eq $2} (keys %{$i->{conditional}});
				}
			}
		}
		close(DAT);
	}

	print STDERR join("\n", glob("$i->{asmdir}/*.S"))."\n";
	foreach my $asm (glob("$i->{asmdir}/*.S"))
	{
		print STDERR "==== working on asm file $asm\n";
		open(DAT, '<', $asm) or die "Cannot open $asm.\n";
		while(<DAT>)
		{
			if(/^\s*\.globl\s+ASM_NAME\((\S+)\)$/)
			{
				print STDERR;
				push(@symbols, $1) unless grep {$_ eq $1} @symbols;
			}
		}
		close(DAT);
	}

	print $out "#ifndef $i->{guard}\n";
	print $out "#define $i->{guard}\n";
	print $out <<EOT;
/* Mapping of internal mpg123 symbols to something that is less likely to
   conflict in case of static linking. */
#include "config.h"
EOT

	foreach my $sym (@symbols)
	{
		my $name = $i->{prefix}.$sym;
		my $signi = substr($name,0,31);
		#print STDERR "$name / $signi\n";
		if(++$ident{$signi} > 1)
		{
			print STDERR "WARNING: That symbol is not unique in 31 chars: $name\n";
		}
		print $out "#define $sym $name\n";
	}
	foreach my $key (keys  %{$i->{conditional}})
	{
		my ($sym, $guard) = ($key, $i->{conditional}{$key});
		my $name = $i->{prefix}.$sym;
		my $signi = substr($name,0,31);
		if(++$ident{$signi} > 1)
		{
			print STDERR "WARNING: That symbol is not unique in 31 chars: $name\n";
		}
		print $out "#ifndef $guard\n";
		print $out "#define $sym $name\n";
		print $out "#endif\n";
	}

	print $out "#endif\n";
}
