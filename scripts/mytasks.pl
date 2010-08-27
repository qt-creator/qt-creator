#!/usr/bin/perl

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

    if ($line =~ /(FIXME|TODO)(.*)$/) {
        print "$file\t$row\tWARNING\t$1$2\n";
    }
    if ($line =~ /(qDebug|QDebug>|QtDebug>|<debug>)/) {
        print "$file\t$row\tWARNING\tRemove debug code\n";
    }
    if ($line =~ /^(<<<<|====|>>>>)/) {
        print "$file\n$row\tERROR\tResolve conflict!\n";
    }
    if ($line =~ /(Q_SIGNALS|Q_SLOTS)/) {
        print "$file\t$row\tWARNING\tPrefer signals and slots over Q_SIGNALS and Q_SLOTS\n";
    }
    if ($line =~ /^#\s*(warning|error) (.*)$/) {
        print "$file\t$row\t$1\tClean up preprocessor $1:$2\n";
    }
}

my $filename = "";
my $pos = 0;

open(PIPE, "git diff origin/master|");  
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
