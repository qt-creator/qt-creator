#! /usr/bin/perl -w
#############################################################################
##
## Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
## All rights reserved.
## Contact: http://www.qt-project.org/
##
## This file is part of the translations module of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL$
## GNU Lesser General Public License Usage
## This file may be used under the terms of the GNU Lesser General Public
## License version 2.1 as published by the Free Software Foundation and
## appearing in the file LICENSE.LGPL included in the packaging of this file.
## Please review the following information to ensure the GNU Lesser General
## Public License version 2.1 requirements will be met:
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Nokia gives you certain additional
## rights. These rights are described in the Nokia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## Other Usage
##
## Alternatively, this file may be used in accordance with the terms and
## conditions contained in a signed written agreement between you and Nokia.
##
##
##
##
##
##
##
##
##
## $QT_END_LICENSE$
##
#############################################################################


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
