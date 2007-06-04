#!/usr/bin/perl
#
# benchmark-cpu.pl: benchmark CPU optimisations of mpg123
#
# written by Nicholas J Humfrey <njh@aelius.com>, placed in the public domain
#

use strict;
use Time::HiRes qw/time/;

my $MPG123_CMD = '../src/mpg123';
my $TEST_FILE = $ARGV[0];

die "Please specify a test MP3 file to decode" if (scalar(@ARGV) < 1);
die "mpg123 command does not exist" unless (-e $MPG123_CMD);
die "mpg123 command is not executable" unless (-x $MPG123_CMD);
die "test MP3 file does not exist" unless (-e $TEST_FILE);


# Force unbuffed output on STDOUT
$|=1;

# Check the CPUs available
my $cpulist = `$MPG123_CMD --list-cpu`;
chomp( $cpulist );
die "Failed to get list of available CPU optimisations" unless ($cpulist =~ /^CPU options: /);

my @cpus = split( / /, substr( $cpulist, 13 ) );
printf("Found %d CPU optimisations to test...\n\n", scalar(@cpus) );

foreach my $cpu (@cpus) {
	print "Checking speed of $cpu optimisation: ";
	
	my $start_time = time();
	system( $MPG123_CMD, '-q', '--cpu', $cpu, '-t', $TEST_FILE );
	my $end_time = time();
	
	printf("%4.4f seconds\n", $end_time - $start_time );
	
}

print "\n";

