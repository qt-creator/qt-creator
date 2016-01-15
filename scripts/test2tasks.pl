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

test2tasks.pl - Convert QTest logs into Qt Creator task files.

=head1 SYNOPSIS

    test2tasks.pl [OPTIONS] < logfile > taskfile

Options:

    -a              Use absolute file paths

    -r <some_path>  Prefix all file names by <some_path> (Used for
                    creating a summarized log of a submodule repository)


The script needs to be run in the working directory from which the test log was
obtained as it attempts to perform a mapping from the source file base names of
the test log to relative path names by searching the files.

=cut

use strict;

use File::Find;
use Getopt::Long;
use File::Spec;
use Cwd;

my $optAbsolute = 0;
my $optRelativeTo;

# --------------- Detect OS

my ($OS_LINUX, $OS_WINDOWS, $OS_MAC)  = (0, 1, 2);
my $os = $OS_LINUX;
if (index($^O, 'MSWin') >= 0) {
    $os = $OS_WINDOWS;
} elsif (index($^O, 'darwin') >= 0) {
   $os = $OS_MAC;
}

# -- Build a hash from source file base name to relative paths.

my %fileHash;
my $workingDirectory = getcwd();

sub handleFile
{
    my $file = $_;
    return unless -f $file;
    return unless index($file, '.cpp') != -1 || index($file, '.h') != -1 || index($file, '.qml');
#   './file' -> 'file'
    my $name = substr($File::Find::name, 0, 1) eq '.' ?
               substr($File::Find::name, 2) : $File::Find::name;
     my $fullName = $name;
    if (defined $optRelativeTo) {
    $fullName = File::Spec->catfile($optRelativeTo, $File::Find::name);
    } else {
    $fullName = File::Spec->catfile($workingDirectory, $File::Find::name) if ($optAbsolute);
    }
    $fullName =~ s|\\|/|g; # The task pane wants forward slashes on Windows, also.
    $fileHash{$file} = $fullName;
}

#   Main
if (!GetOptions("absolute" => \$optAbsolute,
                "relative=s" => \$optRelativeTo)) {
    print "Invalid option\n";
    exit (0);
}

find({ wanted => \& handleFile}, '.');

# --- Find file locations and format them with the cause of the error
#     from the above lines as task entry.

my $lastLine = '';
my ($failCount, $fatalCount) = (0, 0);

sub isAbsolute
{
    my ($f) = @_;
    return $f =~ /^[a-zA-Z]:/ ? 1 : 0 if $os eq $OS_WINDOWS;
    return index($f, '/') == 0 ? 1 : 0;
}

while (my $line = <STDIN> ) {
    chomp($line);
    # --- Continuation line?
    if (substr($line, 0, 1) eq ' ' && index($line, 'Loc: [') < 0) {
       $lastLine .= $line;
       next;
    }
    # --- extract file name based matching:
    #     Windows: '[..\].\tst_lancelot.cpp(258) : failure location'
    #     Unix:    '  Loc: [file(1596)]'
    if ($line =~ /^([^(]+)\((\d+)\) : failure location$/
        || $line =~ /^\s*Loc:\s*\[([^(]+)\((\d+)\).*$/) {
        my $fullFileName = $1;
        my $line = $2;
        #  -- Fix '/C:/bla' which is sometimes reported for QML errors.
        $fullFileName = substr($fullFileName, 1) if ($os eq $OS_WINDOWS && $fullFileName =~ /^\/[a-zA-Z]:\//);
        if (!isAbsolute($fullFileName)) { # Unix has absolute file names, Windows may not
            my $slashPos = rindex($fullFileName, '/');
            $slashPos = rindex($fullFileName, "\\") if $slashPos < 0;
            my $fileName = $slashPos > 0 ? substr($fullFileName, $slashPos + 1) : $fullFileName;
            $fullFileName = $fileHash{$fileName};
            $fullFileName = $fileName unless defined $fullFileName;
        }
        my $type = index($lastLine, 'FAIL') == 0 || index($lastLine, 'XPASS') == 0 ?
                   'err' : 'unknown';
        print $fullFileName, "\t", $line, "\t", $type, "\t", $lastLine,"\n";
        $failCount++;
    } else {
        if (index($line, 'QFATAL') == 0 || index($line, 'Received a fatal error.') >= 0) {
            print STDERR $line,"\n";
            $fatalCount++;
        }
    }
    $lastLine = $line;
}

print STDERR 'Done, ISSUES: ',$failCount, ', FATAL: ',$fatalCount, "\n";
