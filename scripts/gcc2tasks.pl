#!/usr/bin/perl -w

# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    if ($line =~ /^([^:]+):(\d+):\d*:? (warning|error): (.*)$/) {
        my $fileName = $1;
        my $lineNumber = $2;
        my $type = $3 eq 'warning' ? 'warn' : 'err';
        my $text = $4;
        $fileName =~ s|\\|/|g;
        print $fileName, "\t", $lineNumber, "\t", $type, "\t", $text,"\n";
    }
}
