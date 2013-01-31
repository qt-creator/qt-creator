#!/usr/bin/perl -w

# Report possible problems with copy right headers
#
# Sample usage:
#     find . -type f | xargs ./scripts/hasCopyright.pl

use strict;

shift; # remove script

sub canIgnoreNoCopyright {
    my $file = shift;
    return 1 if ($file =~ /\.png$/ or
        $file =~ /\.ico$/ or
        $file =~ /\.svg$/ or
        $file =~ /\.xpm$/ or
        $file =~ /\.dia$/ or
        $file =~ /\/Doxyfile$/ or
        $file =~ /\.qmlproject$/ or
        $file =~ /\.pr[oi]$/ or
        $file =~ /\.qbs$/ or
        $file =~ /\.qrc$/ or
        $file =~ /\.txt$/i or
        $file =~ /\/README[^\/]*$/i or
        $file =~ /\/LICENSE.LGPL$/i or
        $file =~ /\.ui$/i or
        $file =~ /\.xml$/ or
        $file =~ /\.css$/ or
        $file =~ /\.metainfo$/ or
        $file =~ /\.json$/ or
        $file =~ /\.pl$/ or
        $file =~ /\.py$/ or
        $file =~ /\.sh$/ or
        $file =~ /\.bat$/ or
        $file =~ /\.patch$/ or
        $file =~ /\.sed$/ or
        $file =~ /\.pro\.user$/ or
        $file =~ /\.plist$/ or
        $file =~ /\.qdocconf$/i or
        $file =~ /\.qdocinc/);
    return 0;
}

while (1) {
    my $file = shift;
    last unless $file;

    my $hasCopyright = 0;
    my $hasCurrent = 0;
    my $hasContact = 0;
    my $hasCommercial = 0;
    my $hasLGPL = 0;
    my $hasGPL = 0;
    my $hasDigia = 0;
    my $linecount = 0;

    if ($file !~ /\.png$/) {
        open(my $fh, "<", $file) or die "Could not open $file.\n";

        while (<$fh>) {
            $linecount++;
            last if ($linecount > 50);

            $hasCopyright = 1 if $_ =~ /Copyright/i;
            $hasCurrent = 1 if $_ =~ /\(c\).*\s2013/i;

            $hasContact = 1 if $_ =~ /Contact: http:\/\/www.qt-project.org\/legal/;
            $hasCommercial = 1 if $_ =~ /Commercial (License )?Usage/;
            $hasDigia = 1 if $_ =~ /Digia Plc/;
            $hasLGPL = 1 if $_ =~ /GNU Lesser General Public License Usage/;
            $hasGPL = 1 if $_ =~ /GNU General Public License Usage/;
        }
        close $fh;
    }

    unless ($hasCopyright) {
        print "$file\t";
        if (canIgnoreNoCopyright($file)) {
            print "Warning\t";
        } else {
            print "ERROR\t";
        }
        print "No copyright\n";
        next;
    }

    unless ($hasCurrent) {
        print "$file\tERROR\tcopyright outdated\n";
        next;
    }

    unless ($hasDigia) {
        print "$file\tERROR\tNo digia\n";
        next;
    }

    if (!$hasContact && $file !~ /\.pluginspec\.in$/) {
        print "$file\tERROR\tWrong contact\n";
        next;
    }

    unless ($hasCommercial) {
        print "$file\tERROR\tNo commercial license\n";
        next;
    }

    unless ($hasLGPL) {
        print "$file\tERROR\tNo LGPL license\n";
        next;
    }

    if ($hasGPL) {
        print "$file\tERROR\tHas GPL license\n";
        next;
    }

    print "$file\tinfo\tCopyright OK\n";

} # loop over files

exit 0;
