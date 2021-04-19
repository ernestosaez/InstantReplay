#!/usr/bin/perl

use Time::HiRes qw(usleep);
use File::Copy;

sub newCurrent {

	(my $current, my $speed)=@_;

	if ($current=~/^(.+)\/(\d+)\/(\d+)$/ && -f $current) {
		$prefix=$1;
		$seconds=$2;
		$frame=$3;
		if ($speed>0) {
			$frame++;
			if (!-f "$prefix/$seconds/$frame") {
				$seconds++;
				$frame=1;
			}
		}
		elsif ($speed<0) {
			$frame--;
			if ($frame<=0) {
				$seconds--;
				if (-d "$prefix/$seconds") {
					$frame=1503;
					while (!-f "$prefix/$seconds/$frame" && $frame > 0) {
						$frame--;
					}
				}
			}
		}
		if (-f "$prefix/$seconds/$frame") {
			$current="$prefix/$seconds/$frame";
			open FO,">/mnt/current";
			print FO $current;
			close FO;
		}
	}
}

$name="stream1";
$file="/mnt/screen1";

while (1) {
	open FI,"/mnt/speed";
	$speed=<FI>;
	chomp $speed;
	close FI;
	if ($speed eq "live") {
		print "copy /mnt/live /mnt/current\n";
		copy ("/mnt/live","/mnt/current");
	}
	open FI,"/mnt/current";
	$current=<FI>;
	chomp $current;
	close FI;
	if (-f $current) {
		system ("ln -sf $current /mnt/screen1");
	}
	if ($peed eq "live" || $speed == 0) {
		usleep (40000);
		next;
	}
	elsif ($speed > 0) {
		usleep (40000/$speed);
	}
	elsif ($speed < 0) {
		usleep (40000/($speed*-1));
	}
	newCurrent($current,$speed);
}
