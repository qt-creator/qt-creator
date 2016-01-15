#!/usr/bin/perl -w

############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
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

msvc2tasks.pl - Convert MSVC warnings into Qt Creator task files.

=head1 SYNOPSIS

    msvc2tasks.pl < logfile > taskfile

=cut

use strict;

while (my $line = <STDIN> ) {
    chomp($line);
    # --- extract file name based matching:
    # c:\foo.cpp(395) : warning C4800: 'BOOL' : forcing value to bool 'true' or 'false' (performance warning)
    if ($line =~ /^([^(]+)\((\d+)\) ?: warning (C\d+:.*)$/) {
        my $fileName = $1;
        my $lineNumber = $2;
        my $text = $3;
        $fileName =~ s|\\|/|g;
        $text =~ s|\\|/|g;  # Fix file names mentioned in text since tasks file have backslash-escaping.
        print $fileName, "\t", $lineNumber, "\twarn\t", $text,"\n";
    }
}
