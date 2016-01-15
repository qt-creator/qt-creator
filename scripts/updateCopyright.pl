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

my $old_template = shift @ARGV;
my $new_template = shift @ARGV;
my $file = shift @ARGV;

# Usage: git grep "(C) " | cut -d':' -f1 | sort -u | xargs -L 1 scripts/updateCopyright.pl old.header new.header

# This will remove a verbatim copy of old.header from the file
# given as the last argument and then inserts the new.header
# instead. It may retain copyright year, copyright holder, author
# lines or "this file is part of" information.

# All lines of the old.header are compared to the lines in the file.
# Exceptions are lines matching "Copyright (C) (\d+) ", "Author: " or
# "file is part of" which will must also match both the old.template
# as well as the file-line to be considered a match.

# If there is no match between old.header and the start of file, then
# the file is skipped.

### Set which information to keep from the file here:
my $keepYear = 0;
my $keepHolder = 1;
my $keepContact = 1;
my $keepPartOf = 1;
my $keepAuthors = 1;

### Do not edit below this line...

die "Usage: $0 /path/to/oldtemplate /path/to/newtemplate /path/to/file\n" unless $file;

open TEMPLATE, "<$old_template" or die $!;
my @oldT = <TEMPLATE>;
close TEMPLATE;

open TEMPLATE, "<$new_template" or die $!;
my @newT = <TEMPLATE>;
close TEMPLATE;

open FILE, "<$file" or die $!;
my @contents = <FILE>;
close FILE;

my $currentLineNo = -1;
my $oldLineNo = -1;
my @copyrightYear;
my @copyrightHolder;
my @authors;
my $contactInfo = "";
my $partOf = "";

my $matched = 1;

my $hadCopyright = 0;
my $hadAuthors = 0;

foreach (@contents) {
    last unless $matched;

    my $currentLine = $_;
    ++$currentLineNo;
    ++$oldLineNo unless $hadCopyright or $hadAuthors;

    last if $oldLineNo == @oldT;

    my $oldLine = $oldT[$oldLineNo];

    # print "$currentLineNo: $currentLine";
    # print "old: $oldLine";

    if ($currentLine =~ / Copyright .C. (\d+) (.*)$/i) {
        $hadAuthors = 0;

        push(@copyrightYear, $1);
        my $holder = $2;
        # sanitize "Copyright (C) 2014 - 2016 whoever"
        if ($holder =~ /^- \d\d\d\d (.*)$/) { $holder = $1; }
        push(@copyrightHolder, $holder);

        ## print "  Found copyright ($1, $2)...\n";

        if (!$hadCopyright) {
            $hadCopyright = 1;
            ++$oldLineNo;
            $matched = 0 unless $oldLine =~ / Copyright .C. \d+ /i;
            ### print "    matched copyright: $matched\n";
        }
        next;
    }
    $hadCopyright = 0;

    if ($currentLine =~ / Author: (.*)$/i) {
        push(@authors, $1);

        ## print "  Found author...\n";

        if (!$hadAuthors) {
            $hadAuthors = 1;
            ++$oldLineNo;
            $matched = 0 unless $oldLine =~ / Author: /i;
            ### print "    matched author: $matched\n";
        }
        next;
    }
    $hadAuthors = 0;

    if ($currentLine =~ / Contact: (.*)$/i) {
        $contactInfo = $1;
        ## print "  Found contact...\n";
        $matched = 0 unless $oldLine =~ / Contact: /i;
        ### print "    matched contact: $matched\n";
        next;
    }
    if ($currentLine =~ / file is part of (.*)$/) {
        $partOf = $1;
        ## print "  Found part of...\n";
        $matched = 0 unless $oldLine =~ / file is part of /;
        ### print "    matched partOf: $matched\n";
        next;
    }
    $matched = 0 unless $oldLine eq $currentLine;
    ### print "    GENERIC: $matched\n";
}

if (!$matched) {
    print "$file: did not match input template. Skipping.\n";
    exit 0;
}

# remove empty lines...
++$currentLineNo while ($currentLineNo < @contents and $contents[$currentLineNo] =~ /^\s*$/);

splice @contents, 0, $currentLineNo;

print "Contents found in $file:\n";
print "Copyright:\n";
for (my $i = 0; $i < @copyrightYear; ++$i) {
    print "    " . $copyrightYear[$i] . " - " . $copyrightHolder[$i] . ".\n";
}
print "Authors:\n";
foreach (@authors) { print "    $_...\n"; }
print "Contact: $contactInfo...\n";

### Write file:
my $hasEmptyLine = 0;
open FILE, ">$file" or die "Failed to write into $file: $!.\n";

foreach (@newT) {
    my $currentLine = $_;

    $hasEmptyLine = ($currentLine =~ /^\s*$/);

    if ($currentLine =~ /^(.* Copyright .C.) (\d+) (.*)$/i) {
        my $prefix = $1;
        my $year = $2;
        my $holder = $3;
        if ($keepYear || $keepHolder) {
            for (my $i = 0; $i < @copyrightYear; ++$i) {
                my $y = $year;
                $y = $copyrightYear[$i] if $keepYear;
                my $h = $holder;
                $h = $copyrightHolder[$i] if $keepHolder;
                print FILE "$prefix $y $h\n";
            }
            next;
        }
    }

    if ($currentLine =~ /^(.* Author:) (.*)$/i) {
        my $prefix = $1;
        my $author = $2;

        if ($keepAuthors) {
            foreach (@authors) { print FILE "$prefix $_\n"; }
            next;
        }
    }

    if ($currentLine =~ /^(.* Contact:) (.*)$/i) {
        my $prefix = $1;
        $contactInfo = $2 unless $contactInfo;
        if ($keepContact) {
            print FILE "$prefix $contactInfo\n";
            next;
        }
    }

    if ($currentLine =~ /^(.* file is part of) (.*)$/) {
        my $prefix = $1;
        if ($keepPartOf) {
            $partOf = $2 unless $partOf;
            print FILE "$prefix $partOf\n";
            next;
        }
    }

    print FILE $currentLine;
}

print FILE "\n" unless ($hasEmptyLine);

foreach (@contents) { print FILE $_; }

close FILE;
