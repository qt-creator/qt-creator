#! /usr/bin/perl -w

# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

use strict;

my %scores = ();

my $files = join("\n", <*_??.ts>);
my $res = `xmlpatterns -param files=\"$files\" check-ts.xq`;
for my $i (split(/ /, $res)) {
  $i =~ /^(?:[^\/]+\/)*qtcreator_([^.]+)\.ts:(.*)$/;
  my ($lang, $pc) = ($1, $2);
  $scores{$lang} = $pc;
}

my $code = "";

for my $lang (sort(keys(%scores))) {
  my $pc = $scores{$lang};
  my $fail = "";
  if (int($pc) < 98) {
    $fail = "    (excluded)";
  } else {
    $code .= " ".$lang;
  }
  printf "%-5s %3d%s\n", $lang, $pc, $fail;
}

my $fn = "translations.pro";
my $nfn = $fn."new";
open IN, $fn or die;
open OUT, ">".$nfn or die;
while (1) {
  $_ = <IN>;
  last if (/^LANGUAGES /);
  print OUT $_;
}
while ($_ =~ /\\\n$/) {
  $_ = <IN>;
}
print OUT "LANGUAGES =".$code."\n";
while (<IN>) {
  print OUT $_;
}
close OUT;
close IN;
rename $nfn, $fn;
