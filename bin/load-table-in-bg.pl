#!/usr/bin/perl
#
# Meant to be called from CPF pages with hugeass data loading problems, but you can do it too:
#
# bin/load-table-in-bg.pl dbname tablename filename.gz

use POSIX 'setsid';

sub daemonize {
	open STDIN, '/dev/null' or die "Can't read /dev/null: $!";
	open STDOUT, '>/dev/null'
	or die "Can't write to /dev/null: $!";
	defined(my $pid = fork) or die "Can't fork: $!";
	exit if $pid;
	setsid                  or die "Can't start a new session: $!";
	open STDERR, '>&STDOUT' or die "Can't dup stdout: $!";
}

daemonize();

use strict;

my $dbname = shift @ARGV;
my $tablename = shift @ARGV;
my $filename = shift @ARGV;
my $delay = shift @ARGV || 0;

sleep $delay;

system("gunzip -dc $filename | perl -p -e 's/TABLENAME_DAMMIT/$tablename/gm' | psql $dbname");

