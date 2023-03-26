#!/usr/bin/perl -w

# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

=head1 NAME

msvc2tasks.pl - Convert MSVC/Clang-cl warnings and errors into Qt Creator task files.

=head1 SYNOPSIS

    msvc2tasks.pl < logfile > taskfile

=cut

use strict;

sub filterLine
{
    my ($line) = @_;

    my ($fileName, $lineNumber, $category, $text);

    # --- MSVC:
    # c:\foo.cpp(395) : warning C4800: 'BOOL' : forcing value to bool 'true' or 'false' (performance warning)
    if ($line =~ /^([^(]+)\((\d+)\) ?: (warning|error) (C\d+:.*)$/) {
        $fileName = $1;
        $lineNumber = $2;
        $category = $3;
        $text = $4;
    # --- Clang-cl:
    #  ..\gui\text\qfontengine_ft.cpp(1743,5) :  warning: variable 'bytesPerLine' is used uninitialized whenever switch default is taken [-Wsometimes-uninitialized]
    } elsif ($line =~ /^([^(]+)\((\d+),\d+\) ?: +(warning|error):\s+(.*)$/) {
        $fileName = $1;
        $lineNumber = $2;
        $category = $3;
        $text = $4;
    }

    if (defined $fileName) {
        $fileName =~ s|\\|/|g;
        $text =~ s|\\|/|g;  # Fix file names mentioned in text since tasks file have backslash-escaping.
        $category = $category eq 'warning' ? 'warn' : 'err';
    }

    return ($fileName, $lineNumber, $category, $text);
}

while (my $line = <STDIN> ) {
    chomp($line);
    my ($fileName, $lineNumber, $category, $text) = filterLine($line);
    print $fileName, "\t", $lineNumber, "\t", $category, "\t", $text,"\n" if defined $text;
}
