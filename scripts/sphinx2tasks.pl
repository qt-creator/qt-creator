#!/usr/bin/perl

# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

=head1 NAME

sphinx2tasks.pl - Convert sphinx (Python documentation) warnings into Qt Creator task files.

=head1 SYNOPSIS

    sphinx2tasks.pl < logfile > taskfile

=cut

use strict;
use warnings;

while (my $line = <STDIN>) {
    chomp($line);
    # Strip terminal control characters
    my $len = length($line);
    $line = substr($line, 5, $len - 17) if ($len > 0 && ord(substr($line, 0, 1)) == 0x1B);
    # --- extract file name based matching:
    # file.rst:698: WARNING: undefined label: xquer (if the link....)
    if ($line =~ /^[^\/]*([^:]+\.rst):(\d*): WARNING: (.*)$/) {
        my $fileName = $1;
        my $lineNumber = $2 eq '' ? '1' : $2;
        my $text = $3;
        print $fileName, "\t", $lineNumber, "\twarn\t", $text,"\n";
    }
}
