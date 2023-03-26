#!/usr/bin/perl -w

# Copyright (C) 2017 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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


