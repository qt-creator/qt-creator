#!/usr/bin/perl

($second, $minute, $hour, $dayOfMonth, $month, $yearOffset, $dayOfWeek, $dayOfYear, $daylightSavings) = localtime();
$filename = "$yearOffset-$month-$dayOfMonth.log";

system("git rev-parse HEAD >$filename");
system("./tst_bauhaus >$filename");
