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

use strict;

my $file = shift;

if (!defined $file)
{
   my $usage=<<EOF;
Usage: krazy2tasks.pl outputfile

Runs KDE's Krazy2 code scanning tool on .cpp/.h files it finds below the
the working directory, filters and converts the output into a .tasks file
that can be loaded into Qt Creator's build issues pane via 'Open'.
EOF
   print $usage;
   exit(0);
}

open(PIPE, 'find . -name "*.cpp" -o -name "*.h" | grep -v /tests/ | grep -v /3rdparty/ | xargs krazy2 --check-sets qt4 --exclude captruefalse --export textedit |') or
    die 'Could not start krazy2, please make sure it is in your PATH.';
open(FILE, ">$file") or die ('Failed to open ' . $file . ' for writing.');

while (<PIPE>) {
    my $line = $_;
    chomp $line;
    next unless $line =~ /^(.*):(\d+):(.*)$/;

    my $file = $1;
    my $lineno = $2;
    my $description = $3;
    next if $file =~ /\/3rdparty\//;
    next if $file =~ /\/tests\//;

    print FILE "$file\t$lineno\tWARN\tKrazy: $description\n";
}
