#!/usr/bin/perl -w

# Copyright (C) 2017 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

=head1 NAME

clazyweb2tasks.pl - Convert Clazy logs as displayed by the Web frontend into
Qt Creator task files.

Expected format:

Explanation for clazy-strict-iterators
./qtbase/src/tools/moc/preprocessor.cpp
line 995: for (Symbols::const_iterator j = mergeSymbol + 1; j != i; ++j)
=> Mixing iterators with const_iterators

=head1 SYNOPSIS

    clazyweb2tasks.pl < logfile > taskfile

=cut

use strict;

my $message = '';
my $fileName = '';

while (my $line = <STDIN> ) {
    chomp($line);
    if ($line =~ /\s*Explanation for (.*)$/) {
         $message = $1;
    } elsif (index($line, './') == 0) {
        $fileName = substr($line, 2);
    } elsif ($line =~ /\s*line (\d+):/) {
        my $lineNumber = $1;
        print $fileName, "\t", $lineNumber, "\tclazy\t", $message,"\n";
    }
}
