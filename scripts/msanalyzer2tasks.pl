#!/usr/bin/perl -w

# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

=head1 NAME

msanalyzer2tasks.pl - Convert MSVC/Static analyzer warnings and errors into Qt Creator task files.

=head1 SYNOPSIS

    msanalyzer2tasks.pl < logfile > taskfile

=cut

# Sample:
# <DEFECTS>
#  <DEFECT>
#    <SFA>
#      <FILEPATH>c:\dev\qt5\qtbase\src\corelib\statemachine\</FILEPATH>
#      <FILENAME>qstatemachine.cpp</FILENAME>
#      <LINE>968</LINE>
#      <COLUMN>21</COLUMN>
#    </SFA>
#    <DEFECTCODE>6246</DEFECTCODE>
#    <DESCRIPTION>Local declaration of 'i' hides declaration of the same name in outer scope. For additional information, see previous declaration at line '953' of 'c:\dev\qt5\qtbase\src\corelib\statemachine\qstatemachine.cpp'.</DESCRIPTION>
#    <FUNCTION>QStateMachinePrivate::enterStates</FUNCTION>
#    <DECORATED>?enterStates@QStateMachinePrivate@@UAEXPAVQEvent@@ABV?$QList@PAVQAbstractState@@@@1ABV?$QSet@PAVQAbstractState@@@@AAV?$QHash@PAVQAbstractState@@V?$QVector@UQPropertyAssignment@@@@@@ABV?$QList@PAVQAbstractAnimation@@@@@Z</DECORATED>
#    <FUNCLINE>941</FUNCLINE>
#    <PATH>
#      <SFA>
#        <FILEPATH>c:\dev\qt5\qtbase\src\corelib\statemachine\</FILEPATH>
#        <FILENAME>qstatemachine.cpp</FILENAME>
#        <LINE>953</LINE>
#        <COLUMN>18</COLUMN>
#      </SFA>
#    </PATH>
#  </DEFECT>
#  <DEFECT>

use strict;

my ($description, $defectCode);
my (@filePaths, @fileNames, @lineNumbers);

# Return element contents of a line "<ELEMENT>Contents</ELEMENT>"
sub elementContents
{
    my ($line) = @_;
    return $line =~ /^ *<([^>]*)>([^<]*)<\/.*$/
        ? ($1, $2) : (undef, undef)
}

# Fix path 'c:\foo\' -> 'c:/foo'
sub fixFilePath
{
    my ($p) = @_;
    $p =~ s|\\|/|g;
    my $lastSlash = rindex($p, '/');
    chop($p) if $lastSlash  == length($p) - 1;
    return $p;
}

# Fix message (replace XML entities, backslash)
sub fixMessage
{
    my ($m) = @_;
    $m =~ s|\\|/|g;
    $m =~ s/&lt;/</g;
    $m =~ s/&gt;/>/g;
    $m =~ s/&amp;/&/g;
    return $m;
}

while (my $line = <STDIN> ) {
    chomp($line);
    my ($element, $content) = elementContents($line);
    if (defined($element)) {
        if ($element eq 'FILEPATH') {
            push(@filePaths, fixFilePath($content));
        } elsif ($element eq 'FILENAME') {
            push(@fileNames, $content);
        } elsif ($element eq 'LINE') {
            push(@lineNumbers, $content);
        } elsif ($element eq 'DESCRIPTION') {
            $description = fixMessage($content);
        } elsif ($element eq 'DEFECTCODE') {
            $defectCode = $content;
        }
    } elsif (index($line, '</DEFECT>') >= 0) {
        my $count = scalar(@filePaths);
        for (my $i = 0; $i < $count; $i++) {
            my $text = $description;
            $text .= ' (' . $defectCode . ')' if defined $defectCode;
            print $filePaths[$i], '/', $fileNames[$i], "\t", $lineNumbers[$i], "\twarn\t",  $text,"\n";
        }
        $description =  $defectCode = undef;
        @filePaths = @fileNames= @lineNumbers = ();
    }
}
