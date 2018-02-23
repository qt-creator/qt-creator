#!/usr/bin/perl

############################################################################
#
# Copyright (C) 2018 The Qt Company Ltd.
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
