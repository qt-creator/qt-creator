#!/usr/bin/perl -w

=head1 NAME

msvc2tasks.pl - Convert MSVC warnings into Qt Creator task files.

=head1 SYNOPSIS

    msvc2tasks.pl < logfile > taskfile

=cut

use strict;

while (my $line = <STDIN> ) {
    chomp($line);
    # --- extract file name based matching:
    # c:\foo.cpp(395) : warning C4800: 'BOOL' : forcing value to bool 'true' or 'false' (performance warning)
    if ($line =~ /^([^(]+)\((\d+)\) : warning (C\d+:.*)$/) {
        my $fileName = $1;
        my $lineNumber = $2;
        my $text = $3;
        $fileName =~ s|\\|/|g;
        $text =~ s|\\|/|g;  # Fix file names mentioned in text since tasks file have backslash-escaping.
        print $fileName, "\t", $lineNumber, "\twarn\t", $text,"\n";
    }
}
