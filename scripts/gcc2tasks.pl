#!/usr/bin/perl -w

=head1 NAME

msvc2tasks.pl - Convert GCC warnings into Qt Creator task files.

=head1 SYNOPSIS

    gcc2tasks.pl < logfile > taskfile

=cut

use strict;

while (my $line = <STDIN> ) {
    chomp($line);
    # --- extract file name based matching:
    # file.cpp:214:37: warning: cast from pointer to integer of different size [-Wpointer-to-int-cast]
    if ($line =~ /^([^:]+):(\d+):\d*:? warning: (.*)$/) {
        my $fileName = $1;
        my $lineNumber = $2;
        my $text = $3;
        $fileName =~ s|\\|/|g;
        print $fileName, "\t", $lineNumber, "\twarn\t", $text,"\n";
    }
}
