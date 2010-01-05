#! /usr/bin/perl -w

use strict;

@ARGV == 1 or die "usage: $0 <qdoc-file>\n";
my $file = $ARGV[0];
open FILE, $file or die "File $file cannot be opened.\n";
my @toc = ();
my %title2page = ();
my $doctitle = "";
my $curpage = "";
my $intoc = 0;
while (<FILE>) {
    if (!$intoc) {
        if (keys(%title2page) == 1 && /^\h*\\list/) {
            $intoc = 1;
        } elsif (/^\h*\\page\h+(\H+)/) {
            $curpage = $1;
        } elsif (/^\h*\\title\h+(.+)$/) {
            if ($curpage eq "") {
                die "Title '$1' appears in no \\page.\n";
            }
            $title2page{$1} = $curpage;
            $doctitle = $1 if (!$doctitle);
            $curpage = "";
        }
    } else {
        if (/^\h*\\endlist/) {
            $intoc = 0;
        } elsif (/^\h*\\o\h+\\l{(.*)}$/) {
            push @toc, $1;
        }
    }
}
close FILE;

my %prev = ();
my %next = ();
my $last = $doctitle;
for my $title (@toc) {
    $next{$last} = $title2page{$title};
    $prev{$title} = $title2page{$last};
    $last = $title;
}

open IN, $file or die "File $file cannot be opened a second time?!\n";
open OUT, '>'.$file.".out" or die "File $file.out cannot be created.\n";
my $cutting = 0;
while (<IN>) {
    if (!$cutting) {
        if (/^\h*\\contentspage/) {
            $cutting = 1;
        }
    } else {
        if (/^\h*\\title\h+(.+)$/) {
            print OUT "    \\previouspage ".$prev{$1} if ($prev{$1});
            print OUT "    \\page ".$title2page{$1};
            print OUT "    \\nextpage ".$next{$1} if ($next{$1});
            print OUT "\n";
            $cutting = 0;
        } else {
            next;
        }
    }
    print OUT $_;
}
close OUT;
close IN;

rename($file.".out", $file) or die "Cannot replace $file with new version.\n";
