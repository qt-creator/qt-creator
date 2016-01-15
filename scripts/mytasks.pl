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

use strict;

my $date = `date`;
print "# $date";

sub escape {
    my $text = shift;
    $text =~ s/\\/\\\\/g;
    $text =~ s/\n/\\n/g;
    $text =~ s/\t/\\t/g;
    return $text;
}
sub checkLine {
    my $line = shift;
    my $file = shift;
    my $row = shift;

    $file = escape($file);
    $row = escape($row);

    if ($file =~ /(.cpp|.c|.cxx|.h)$/ && $line =~ /(FIXME|TODO)(.*)$/) {
        print "$file\t$row\tWARNING\t$1$2\n";
    }
    if ($file =~ /(.cpp|.c|.cxx|.h)$/ && $line =~ /(qDebug|QDebug>|QtDebug>|<debug>)/) {
        print "$file\t$row\tWARNING\tRemove debug code.\n";
    }
    if ($line =~ /^(<<<<|====|>>>>)/) {
        print "$file\n$row\tERROR\tResolved conflict.\n";
    }
    if ($file =~ /(.cpp|.c|.cxx|.h)$/ && $line =~ /(Q_SIGNALS|Q_SLOTS)/) {
        print "$file\t$row\tWARNING\tPrefer signals and slots over Q_SIGNALS and Q_SLOTS.\n";
    }
    if ($line =~ /^#\s*(warning|error) (.*)$/) {
        print "$file\t$row\t$1\tClean up preprocessor $1:$2.\n";
    }
    if ($file =~ /(.cpp|.c|.cxx|.h)$/ && $line =~ /\s+$/) {
        print "$file\t$row\tWARNING\tTrailing whitespaces found.\n";
    }
    if ($file =~ /(.cpp|.c|.cxx|.h)$/ && $line =~ /^ *\t */) {
        print "$file\t$row\tWARNING\tTabs used to indent.\n";
    }
    if ($file =~ /(.cpp|.c|.cxx|.h)$/ && $line =~ /^\s+{$/) {
        print "$file\t$row\tWARNING\tOpening bracket found all alone on a line.\n";
    }
    if ($file =~ /(.cpp|.c|.cxx|.h)$/ && $line =~ /{.*[^\s].*$/) {
        if ($line !~ /\{\s+\/\*/ && $line !~ /^\s*\{ \}$/ && $line !~ /^namespace .* \}$/)
        { print "$file\t$row\tWARNING\tText found after opening bracket.\n"; }
    }
    if ($file =~ /(.cpp|.c|.cxx|.h)$/ && $line =~ /[a-zA-Z0-9_]\*[^\/]/) {
        next if $line =~ /SIGNAL\(/;
        next if $line =~ /SLOT\(/;
        print "$file\t$row\tWARNING\tUse \"TYPE *NAME\" (currently the space between TYPE and * is missing).\n";
    }
    if ($file =~ /(.cpp|.c|.cxx|.h)$/ && $line =~ /[a-zA-Z0-9_]&[^&]/) {
        print "$file\t$row\tWARNING\tUse \"TYPE &NAME\" (currently the space between TYPE and & is missing).\n";
    }
    if ($file =~ /(.cpp|.c|.cxx|.h)$/ && $line =~ /\*\s+/) {
        next if ($line =~ /^\s*\*\*?\s+/);
        next if ($line =~ /\/\*/);
        print "$file\t$row\tWARNING\tUse \"TYPE *NAME\" (currently there is a space between * and NAME).\n";
    }
    if ($file =~ /(.cpp|.c|.cxx|.h)$/ && $line =~ /[^&]&\s+/) {
        print "$file\t$row\tWARNING\tUse \"TYPE &NAME\" (currently there is a space between & and NAME).\n";
    }
}

my $filename = "";
my $pos = 0;

sub getDiffOrigin {
   my $currentBranch = `git branch | grep "^* "`;
   chop $currentBranch;
   $currentBranch =~ s/^\s*\* //;

   my $remoteRepo = `git config --local --get branch.$currentBranch.remote`;
   chop $remoteRepo;

   return "HEAD" if (!$remoteRepo);

   my $remoteBranch = `git config --local --get branch.$currentBranch.merge`;
   chop $remoteBranch;
   $remoteBranch =~ s!^refs/heads/!!;

   return "HEAD" if (!$remoteBranch);

   return "$remoteRepo/$remoteBranch";
}

my $origin = shift;
$origin = getDiffOrigin unless $origin;
print "# running: git diff $origin ...\n";

open(PIPE, "git diff $origin|");
while (<PIPE>) {
    chomp;
    my $line = $_;
    print "#$line\n";
    next if ($line =~ /^-/);
    if ($line =~ /^\+\+\+ (.*)$/) {
        $filename = $1;
        $filename =~ s/^b\///;
        $pos = 0;
        next;
    }
    next if $filename =~ /^\/dev\/null$/;
    if ($line =~ /^@@ -\d+,\d+\s\+(\d+),\d+ @@$/) {
        $pos = $1 - 1;
        next;
    }
    $pos = $pos + 1;
    if ($line =~ /^\+(.*)/) {
        checkLine($1, $filename, $pos);
    }
}
close(PIPE);
