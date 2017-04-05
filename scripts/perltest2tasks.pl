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

perltest2tasks.pl - Convert messages of the Perl module Test::More into Qt Creator task files.

=head1 SYNOPSIS

    perltest2tasks.pl < logfile > taskfile

=cut

use strict;

my $lineNumber = 1;
my $type = 'err';
my $slash = index($^O, 'MSWin') < 0 ? '/' : "\\";

my $text;

# The format is:
# "ok <number> - message"
# "not ok <number> - message"
# "# message line 2" (Continuation line)
# The message is free format, the script tries to find a file name in it by
# checking for a slash.

sub printPendingMessage
{
    return unless defined $text;
    # Try to extract the file name from the message with some heuristics
    my $fileName;
    for my $word (split(/\s+/,  $text)) {
        if (index($word, $slash) >= 0) {
            $fileName = $word;
            last;
        }
    }
    print $fileName, "\t", $lineNumber, "\t", $type, "\t", $text,"\n" if defined $fileName;
    $text = undef;
}

while (my $line = <STDIN> ) {
    chomp($line);
    if (index($line, '#') == 0) { # Continuation line
        $line =~ s/^#\s+//;
        $text .= ' ' . $line if defined($text);
    } elsif ($line =~ /^ok\s/) { # Passed test, flush message
        printPendingMessage();
    } elsif ($line =~ /^not ok\s+\d+\s-\s+(.*)$/) { # Failed test, flush message and start over
        printPendingMessage();
        $text = $1;
    }
}

printPendingMessage();


