#!/usr/bin/perl

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
