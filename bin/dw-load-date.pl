#!/usr/bin/perl
#
# Meant to be called from CPF pages with hugeass data loading problems, but you can do it too:
#
# bin/load-table-in-bg.pl dbname tablename filename.gz

use POSIX 'setsid';

use strict;

sub daemonize {
    open STDIN, '/dev/null' or die "Can't read /dev/null: $!";
    open STDOUT, '>/dev/null'
      or die "Can't write to /dev/null: $!";
    defined( my $pid = fork ) or die "Can't fork: $!";
    exit if $pid;
    setsid or die "Can't start a new session: $!";
    open STDERR, '>&STDOUT' or die "Can't dup stdout: $!";
}

daemonize();

use strict;

my $dbname    = shift @ARGV;
my $tablename = shift @ARGV;
my $delay     = shift @ARGV || 0;
sleep $delay;

use strict;
use POSIX;

open( F, "|psql $dbname" );
print F "begin;";


#1997-01-01 = 852094800
#2038-01-18 = 2147403600
my $date = 852094800;
my %qmap = (
    'Jan' => '1',
    'Feb' => '1',
    'Mar' => '1',
    'Apr' => '2',
    'May' => '2',
    'Jun' => '2',
    'Jul' => '3',
    'Aug' => '3',
    'Sep' => '3',
    'Oct' => '4',
    'Nov' => '4',
    'Dec' => '4'
);
while ( $date < 2147403600 ) {

    my @ltime = localtime($date);
#epoch started on a wednesday.  Good week numbers will start 3 days before the epoch
    my $day_num  = floor($date / (24 * 60 * 60));
    my $is_weekday = ((($day_num + 3) % 7) <5) ? 1 : 0;
    my $week_num =
      floor( ( $date - ( 3 * 24 * 60 * 60 ) ) / ( 7 * 24 * 60 * 60 ) );
      my $dom = strftime('%d', @ltime);
      my $dow = strftime('%w', @ltime);
      my $month = strftime("%m", @ltime);
      my $year = strftime("%Y", @ltime);
    my $mon_num = ( $year - 1997)*12 + $month - 1;

	my $fdom =  strftime('%w', 0,0, 0, 0, $month-1, $year);

      my $week_in_month = floor(( $dom - $dow + $fdom - 1) / 7)+1;

    my $quarter = $qmap{ strftime( "%b", @ltime ) };
    print F strftime(
"insert into $tablename(pgsql_date, mday, mon, year, dow, doverall, week_in_year, week_in_month, week_number_overall, month_number_overall, quarter, is_weekday) values  ('%F', $dom, $month, $year, $dow, $day_num, %U, $week_in_month, $week_num,  $mon_num, '$quarter', $is_weekday);\n",
        @ltime
    );

    $date += 24 * 60 * 60;    #next day
}

print F "commit;";

close F;
