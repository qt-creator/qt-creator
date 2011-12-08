#!/usr/bin/perl -w

=head1 NAME

test2tasks.pl - Convert QTest logs into Qt Creator task files.

=head1 SYNOPSIS

    test2tasks.pl < logfile > taskfile

The script needs to be run in the working directory from which the test log was
obtained as it attempts to perform a mapping from the source file base names of
the test log to relative path names by searching the files.

=cut

use strict;

use File::Find;

# -- Build a hash from source file base name to relative paths.

my %fileHash;

sub handleFile
{
    my $file = $_;
    return unless index($file, '.cpp') != -1 && -f $file;
#   './file' -> 'file'
    $fileHash{$file} = substr($File::Find::name, 0, 1) eq '.' ? substr($File::Find::name, 2) : $File::Find::name;
}

find({ wanted => \& handleFile}, '.');

# --- Find file locations and format them with the cause of the error
#     from the above lines as task entry.

my $lastLine = '';
while (my $line = <STDIN> ) {
    chomp($line);
    # --- Continuation line?
    if (substr($line, 0, 1) eq ' ') {
       $lastLine .= $line;
       next;
    }
    # --- extract file name based matching '[..\].\tst_lancelot.cpp(258) : failure location'
    if ($line =~ /^([^(]+)\((\d+)\) : failure location$/) {
        my $slashPos = rindex($1, '/');
        $slashPos = rindex($1, "\\") if $slashPos < 0;
        my $fileName = $slashPos > 0 ? substr($1, $slashPos + 1) : $1;
        my $line = $2;
        my $fullFileName = $fileHash{$fileName};
        $fullFileName = $fileName unless defined $fullFileName;
        my $type = index($lastLine, 'FAIL') == 0 ? 'err' : 'unknown';
        print $fullFileName, "\t", $line, "\t", $type, "\t", $lastLine,"\n";
    }
    $lastLine = $line;
}
