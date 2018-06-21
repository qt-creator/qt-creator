#! /usr/bin/perl

use warnings;
use strict;

sub visitDir($)
{
    my ($dir) = @_;
    my @ret = ();
    my @subret = ();
    opendir DIR, $dir or die "$dir: $!\n";
    my @ents = readdir DIR;
    closedir DIR;
    for my $ent (grep !/^\./, @ents) {
        my $ret = $dir."/".$ent;
        if (-d $ret) {
            push @subret, &visitDir($ret);
        } elsif ($ret =~ /\.qdoc$/) {
            push @ret, $ret;
        }
    }
    return @ret, @subret;
}

my @files = ();
my %defines = ();
for (@ARGV) {
    if (/^-D(.*)$/) {
        $defines{$1} = 1;
    } elsif (/^-/) {
        printf STDERR "Unknown option '".$_."'\n";
        exit 1;
    } else {
        if (-d $_) {
            push @files, visitDir($_);
        } else {
            push @files, $_;
        }
    }
}

int(@files) or die "usage: $0 [-D<define>]... <qdoc-file>...\n";

my @toc = ();
my %title2type = ();
my %title2page = ();
my $doctitle = "";
my %prev_skips = ();
my %next_skips = ();
my %prev_define_skips = ();
my %next_define_skips = ();
my %prev_polarity_skips = ();
my %next_polarity_skips = ();
for my $file (@files) {
    my ($curtype, $curpage, $inhdr, $havetoc, $intoc, $inif) = ("", 0, 0, 0, 0);
    my ($define_skip, $polarity_skip, $skipping) = ("", 0, 0);
    my ($prev_define_skip, $prev_polarity_skip, $prev_skip,
        $next_define_skip, $next_polarity_skip, $next_skip) = ("", 0, "", "", 0, "");
    open FILE, $file or die "File $file cannot be opened.\n";
    while (<FILE>) {
        if (/^\h*\\if\h+defined\h*\(\h*(\H+)\h*\)/) {
            die "Nested \\if at $file:$.\n" if ($inif);
            $inif = 1;
            $skipping = !defined($defines{$1});
            if ($inhdr) {
                $define_skip = $1;
                $polarity_skip = $skipping;
            }
        } elsif (/^\h*\\else/) {
            die "Unmatched \\else in $file:$.\n" if (!$inif);
            $skipping = 1 - $skipping;
        } elsif (/^\h*\\endif/) {
            die "Unmatched \\endif in $file:$.\n" if (!$inif);
            $inif = 0;
            $skipping = 0;
            $define_skip = "";
        } elsif ($havetoc && /^\h*\\list/) {
            $intoc++;
        } elsif ($intoc) {
            if (/^\h*\\endlist/) {
                $intoc--;
            } elsif (!$skipping && /^\h*\\li\h+\\l\h*{(.*)}$/) {
                push @toc, $1;
            }
        } elsif ($inhdr) {
            if (/^\h*\\previouspage\h+(\H+)/) {
                $prev_skip = $1 if ($skipping);
                ($prev_define_skip, $prev_polarity_skip) = ($define_skip, $polarity_skip);
            } elsif (/^\h*\\nextpage\h+(\H+)/) {
                $next_skip = $1 if ($skipping);
                ($next_define_skip, $next_polarity_skip) = ($define_skip, $polarity_skip);
            } elsif (/^\h*\\(page|example)\h+(\H+)/) {
                $curtype = $1;
                $curpage = $2;
            } elsif (/^\h*\\title\h+(.+)$/) {
                if ($curpage eq "") {
                    die "Title '$1' appears in no \\page or \\example.\n";
                }
                if (length($prev_define_skip)) {
                    ($prev_define_skips{$1}, $prev_polarity_skips{$1}, $prev_skips{$1}) =
                            ($prev_define_skip, $prev_polarity_skip, $prev_skip);
                    $prev_define_skip = $prev_skip = "";
                }
                if (length($next_define_skip)) {
                    ($next_define_skips{$1}, $next_polarity_skips{$1}, $next_skips{$1}) =
                            ($next_define_skip, $next_polarity_skip, $next_skip);
                    $next_define_skip = $next_skip = "";
                }
                $title2type{$1} = $curtype;
                $title2page{$1} = $curpage;
                if ($1 eq "Qt Creator TOC") {
                    $havetoc = 1;
                } elsif (!$doctitle) {
                    $doctitle = $1;
                }
                $curpage = "";
                $inhdr = 0;
            }
        } else {
            if (/^\h*\\contentspage\b/) {
                $havetoc = 0;
                $inhdr = 1;
            }
        }
    }
    die "Missing \\title in $file\n" if ($inhdr);
    die "Unclosed TOC in $file\n" if ($intoc);
    close FILE;
}

my %prev = ();
my %next = ();
my $last = $doctitle;
my $lastpage = $title2page{$last};
for my $title (@toc) {
    my $type = $title2type{$title};
    my $page = ($type eq "page") ? $title2page{$title} : "{$title}\n";
    defined($page) or die "TOC refers to unknown page/example '$title'.\n";
    $next{$last} = $page;
    $prev{$title} = $lastpage;
    $last = $title;
    $lastpage = $page;
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
                if (defined($prev_define_skips{$1})) {
                    print OUT "    \\if defined(".$prev_define_skips{$1}.")\n";
                    if ($prev_polarity_skips{$1}) {
                        print OUT "    \\previouspage ".$prev_skips{$1} if ($prev_skips{$1});
                        print OUT "    \\else\n";
                    }
                }
                print OUT "    \\previouspage ".$prev{$1} if ($prev{$1});
                if (defined($prev_define_skips{$1})) {
                    if (!$prev_polarity_skips{$1}) {
                        print OUT "    \\else\n";
                        print OUT "    \\previouspage ".$prev_skips{$1} if ($prev_skips{$1});
                    }
                    print OUT "    \\endif\n";
                }
                print OUT "    \\".$title2type{$1}." ".$title2page{$1};
                if (defined($next_define_skips{$1})) {
                    print OUT "    \\if defined(".$next_define_skips{$1}.")\n";
                    if ($next_polarity_skips{$1}) {
                        print OUT "    \\nextpage ".$next_skips{$1} if ($next_skips{$1});
                        print OUT "    \\else\n";
                    }
                }
                print OUT "    \\nextpage ".$next{$1} if ($next{$1});
                if (defined($next_define_skips{$1})) {
                    if (!$next_polarity_skips{$1}) {
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
