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
my $count = 0;

while (my $line = <STDIN>) {
    chomp($line);
    # --- extract file name based matching:
    # /.../..../localizedclock-switchlang.qdoc:107: [QtLinguist] (qdoc) warning: Can't link to 'QLocale::setDefault()'
    if ($line =~ /^(..[^:]*):(\d+):.*\(qdoc\) warning: (.*)$/) {
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
            ++$count;
        }
    } elsif (defined($lastDiagnostic) && $line =~ /^    /) {
        $line =~ s/^\s+//;
        $line =~ s/\s+$//;
        $lastDiagnostic .= ' ' . $line;
    }
}

print $lastDiagnostic, "\n" if defined($lastDiagnostic);

print STDERR $count, " issue(s) found.\n" if $count;
