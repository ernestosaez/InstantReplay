#!/usr/bin/perl

use Time::HiRes qw(usleep);
use File::Copy;

$name="stream1";

$liveFile="/mnt/$name/live/$name-";

$lastDir=0;
$count=0;
$frame=1;
while (1) {
	$count++;
	$thisFile=$liveFile.$count.".jpg";
	while (!-f $thisFile) {
		usleep(1000);
	}
	print $thisFile."\n";
	if (!-f $thisFile) {
		next;
	}
	@stat=stat($thisFile);
	$seconds=time();
	if ($stat[9] < $seconds - 1) {
		print "unlink $thisFile\n";
		unlink ($thisFile);
	}
	$nextFile=$liveFile.($count+1).".jpg";
	while (!-f $nextFile) {
		usleep(1000);
	}
	if (-f $nextFile) {
		$dir=int($seconds/60);
		if ($dir != $lastDir) {
			mkdir("/opt/CloudPowder/streams/$name/$dir");
			$frame=1;
		}
		$newFile="/opt/CloudPowder/streams/$name/$dir/$frame";
		if (-f $thisFile) {
			print "rename $thisFile $newFile\n";
			move ($thisFile,$newFile);
			open FO,">/mnt/$ndame/live";
			print FO $newFile;
			close FO;
		}
		$lastDir=$dir;
		$frame++;
	}
	Time::HiRes::usleep(100);
}
