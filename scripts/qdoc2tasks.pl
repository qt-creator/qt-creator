#!/usr/bin/perl

# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

=head1 NAME

qdoc2tasks.pl - Convert qdoc warnings into Qt Creator task files.

=head1 SYNOPSIS

    qdoc2tasks.pl < logfile > taskfile

=cut

use strict;
use warnings;

my $lastDiagnostic;

while (my $line = <STDIN>) {
    chomp($line);
    # --- extract file name based matching:
    # Qt 5.10: D:/.../qaxbase.cpp:3231: warning: Cannot tie this documentation to anything
    # Qt 5.11: D:/.../qaxbase.cpp:3231: (qdoc) warning: Cannot tie this documentation to anything
    if ($line =~ /^(..[^:]*):(\d+): (?:\(qdoc\) )?warning: (.*)$/) {
        if (defined($lastDiagnostic)) {
            print $lastDiagnostic, "\n";
            $lastDiagnostic = undef;
        }
        my $fileName = $1;
        my $lineNumber = $2;
        my $text = $3;
        my $message = $fileName . "\t" . $lineNumber . "\twarn\t" . $text;
        if (index($message, 'clang found diagnostics parsing') >= 0) {
            $lastDiagnostic = $message;
        } else {
            print $message, "\n";
        }
    } elsif (defined($lastDiagnostic) && $line =~ /^    /) {
        $line =~ s/^\s+//;
        $line =~ s/\s+$//;
        $lastDiagnostic .= ' ' . $line;
    }
}

print $lastDiagnostic, "\n" if defined($lastDiagnostic);
