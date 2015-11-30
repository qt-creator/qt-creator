#!/usr/bin/perl

=head1 NAME

qdoc2tasks.pl - Convert qdoc warnings into Qt Creator task files.

=head1 SYNOPSIS

    qdoc2tasks.pl < logfile > taskfile

=cut

use strict;
use warnings;

while (my $line = <STDIN>) {
    chomp($line);
    # --- extract file name based matching:
    # D:/.../qaxbase.cpp:3231: warning: Cannot tie this documentation to anything
    if ($line =~ /^(..[^:]*):(\d+): warning: (.*)$/) {
        my $fileName = $1;
        my $lineNumber = $2;
        my $text = $3;
        print $fileName, "\t", $lineNumber, "\twarn\t", $text,"\n";
    }
}
