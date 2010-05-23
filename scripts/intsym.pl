#!/usr/bin/perl

my $dir = 'src/libmpg123';
my @headers = qw(compat decode dither frame getbits getcpuflags huffman icy2utf8 icy id3 index mpg123lib_intern optimize parse reader);
my $prefix = 'INT123_';

my %ident;

# Extra symbols.
my @symbols = qw(COS9);

foreach my $header (@headers)
{
	print STDERR "==== working on header $header\n";
	open(DAT, '<', $dir.'/'.$header.'.h') or die "Cannot open $header.\n";
	while(<DAT>)
	{
		if(/^([^\s\(#][^\(]*)\s([a-z][a-z_0-9]+)\s*\(/)
		{
			# Skip preprocessing/comment stuff and official API.
			unless($1 =~ '^#' or $1 =~ '/\*' or $2 =~ /^mpg123_/)
			{
				push(@symbols, $2);
			}
		}
	}
	close(DAT);
}

print STDERR join("\n", glob("$dir/*.S"))."\n";
foreach my $asm (glob("$dir/*.S"))
{
	print STDERR "==== working on asm file $asm\n";
	open(DAT, '<', $asm) or die "Cannot open $header.\n";
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

print "#ifndef MPG123_INTMAP_H\n";
print "#define MPG123_INTMAP_H\n";
print "/* Mapping of internal mpg123 symbols to something that is less likely to conflict in case of static linking. */\n";

foreach my $sym (@symbols)
{
	my $name = $prefix.$sym;
	my $signi = substr($name,0,31);
	#print STDERR "$name / $signi\n";
	if(++$ident{$signi} > 1)
	{
		die "That symbol is not unique in 31 chars: $name\n";
	}
	print "#define $sym $name\n";
}

print "#endif\n";
