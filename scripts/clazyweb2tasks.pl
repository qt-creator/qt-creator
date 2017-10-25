#!/usr/bin/perl -w

############################################################################
#
# Copyright (C) 2017 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

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
