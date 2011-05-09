#!/usr/bin/perl -w

use strict;

my $file = shift;
die "No .tasks file given to save data into." unless ($file);

open(PIPE, "find . -name \*.cpp -o -name \*.h | grep -v /tests/ | grep -v /3rdparty/ | xargs krazy2 --check-sets qt4 --exclude captruefalse --export textedit |") or
    die "Could not start krazy2all, please make sure it is in your PATH.";  
open(FILE, ">$file") or die "Failed to open \"$file\" for writing.";

while (<PIPE>) {
    my $line = $_;
    chomp $line;
    next unless $line =~ /^(.*):(\d+):(.*)$/;

    my $file = $1;
    my $lineno = $2;
    my $description = $3;
    next if $file =~ /\/3rdparty\//;
    next if $file =~ /\/tests\//;

    print FILE "$file\t$lineno\tWARN\tKrazy: $description\n";
}

