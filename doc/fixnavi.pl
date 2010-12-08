#! /usr/bin/perl -w

use strict;

my @files = ();
my %defines = ();
for (@ARGV) {
    if (/^-D(.*)$/) {
        $defines{$1} = 1;
    } elsif (/^-/) {
        printf STDERR "Unknown option '".$_."'\n";
        exit 1;
    } else {
        push @files, $_;
    }
}

int(@files) or die "usage: $0 [-D<define>]... <qdoc-file>\n";

my @toc = ();
my %title2page = ();
my $doctitle = "";
my $curpage = "";
my $intoc = 0;
my %prev_skips = ();
my %next_skips = ();
my %define_skips = ();
my %polarity_skips = ();
my $prev_skip = "";
my $next_skip = "";
my $define_skip = "";
my $polarity_skip = 0;
for my $file (@files) {
    my $skipping = 0; # no nested ifs!
    open FILE, $file or die "File $file cannot be opened.\n";
    while (<FILE>) {
        if (/^\h*\\if\h+defined\h*\(\h*(\H+)\h*\)/) {
            $skipping = !defined($defines{$1});
            if (!$intoc) {
                $define_skip = $1;
                $polarity_skip = $skipping;
            }
        } elsif (/^\h*\\else/) {
            $skipping = 1 - $skipping;
        } elsif (/^\h*\\endif/) {
            $skipping = 0;
        } elsif (keys(%title2page) == 1 && /^\h*\\list/) {
            $intoc++;
        } elsif (!$intoc) {
            if ($skipping && /^\h*\\previouspage\h+(\H+)/) {
                $prev_skip = $1;
            } elsif ($skipping && /^\h*\\nextpage\h+(\H+)/) {
                $next_skip = $1;
            } elsif (/^\h*\\page\h+(\H+)/) {
                $curpage = $1;
            } elsif (/^\h*\\title\h+(.+)$/) {
                if ($curpage eq "") {
                    die "Title '$1' appears in no \\page.\n";
                }
                if (length($define_skip)) {
                     $define_skips{$1} = $define_skip;
                     $polarity_skips{$1} = $polarity_skip;
                     $prev_skips{$1} = $prev_skip;
                     $next_skips{$1} = $next_skip;
                     $define_skip = $prev_skip = $next_skip = "";
                }
                $title2page{$1} = $curpage;
                $doctitle = $1 if (!$doctitle);
                $curpage = "";
            }
        } else {
            if (/^\h*\\endlist/) {
                $intoc--;
            } elsif (!$skipping && /^\h*\\o\h+\\l\h*{(.*)}$/) {
                push @toc, $1;
            }
        }
    }
    close FILE;
}

my %prev = ();
my %next = ();
my $last = $doctitle;
for my $title (@toc) {
    $next{$last} = $title2page{$title};
    $prev{$title} = $title2page{$last};
    $last = $title;
}

for my $file (@files) {
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
                if (defined($define_skips{$1})) {
                    print OUT "    \\if defined(".$define_skips{$1}.")\n";
                    if ($polarity_skips{$1}) {
                        print OUT "    \\previouspage ".$prev_skips{$1} if ($prev_skips{$1});
                        print OUT "    \\else\n";
                    }
                }
                print OUT "    \\previouspage ".$prev{$1} if ($prev{$1});
                if (defined($define_skips{$1})) {
                    if (!$polarity_skips{$1}) {
                        print OUT "    \\else\n";
                        print OUT "    \\previouspage ".$prev_skips{$1} if ($prev_skips{$1});
                    }
                    print OUT "    \\endif\n";
                }
                print OUT "    \\page ".$title2page{$1};
                if (defined($define_skips{$1})) {
                    print OUT "    \\if defined(".$define_skips{$1}.")\n";
                    if ($polarity_skips{$1}) {
                        print OUT "    \\nextpage ".$next_skips{$1} if ($next_skips{$1});
                        print OUT "    \\else\n";
                    }
                }
                print OUT "    \\nextpage ".$next{$1} if ($next{$1});
                if (defined($define_skips{$1})) {
                    if (!$polarity_skips{$1}) {
                        print OUT "    \\else\n";
                        print OUT "    \\nextpage ".$next_skips{$1} if ($next_skips{$1});
                    }
                    print OUT "    \\endif\n";
                }
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
}
