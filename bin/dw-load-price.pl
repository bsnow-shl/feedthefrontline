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

#daemonize();

use strict;

my $dbname = shift @ARGV;
my $tablename = shift @ARGV;
my $delay = shift @ARGV || 0;

sleep $delay;
my $tmp = `mktemp`;
chop $tmp;
print $tmp."...";
`echo 'drop table integers; create table integers (x int); copy integers from stdin;' >$tmp`;
`seq 0 30000 >>$tmp`;

system("psql $dbname -f $tmp");

open (F, "|psql $dbname");
print F "insert into $tablename(price,price_band) select x/100.0 , to_char(floor(x/500)*5.0, 'FM990.00') || '-'|| to_char(floor((x+500)/500)*5.0-.01, 'FM990.00') from integers";
close F;



