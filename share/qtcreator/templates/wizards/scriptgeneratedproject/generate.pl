#!/usr/bin/perl -w

# *************************************************************************
#
# This file is part of Qt Creator
#
# Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
#
# Contact: Nokia Corporation (qt-info@nokia.com)
#
# Commercial Usage
#
# Licensees holding valid Qt Commercial licenses may use this file in
# accordance with the Qt Commercial License Agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and Nokia.
#
# GNU Lesser General Public License Usage
#
# Alternatively, this file may be used under the terms of the GNU Lesser
# General Public License version 2.1 as published by the Free Software
# Foundation and appearing in the file LICENSE.LGPL included in the
# packaging of this file.  Please review the following information to
# ensure the GNU Lesser General Public License version 2.1 requirements
# will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
#
# If you are unsure which license is appropriate for your use, please
# contact the sales department at http://qt.nokia.com/contact.
#
# *************************************************************************

use strict;
use Getopt::Long;
use IO::File;

my $optDryRun = 0;
my $fieldClassName = 'MyClass';
my $standardFieldProjectName = 'MyProject';
my $standardFieldCppHeaderSuffix = 'h';
my $standardFieldCppSourceSuffix = 'cpp';

my $USAGE=<<EOF;
Usage: generate.pl [--dry-run] <parameter-mappings>

Custom wizard project generation example script.

Known parameters: ClassName=<value>
EOF

if (!GetOptions("dry-run" => \$optDryRun)) {
    print $USAGE;
    exit (1);
}

if (scalar(@ARGV) == 0) {
    print $USAGE;
    exit (1);
}

# -- Parse the 'field=value' pairs
foreach my $arg (@ARGV) {
    my ($key, $value) = split('=', $arg);
    $fieldClassName = $value if ($key eq 'ClassName');
#   -- Standard fields defined by the custom project wizard
    $standardFieldProjectName = $value if ($key eq 'ProjectName');
    $standardFieldCppHeaderSuffix = $value if ($key eq 'CppHeaderSuffix');
    $standardFieldCppSourceSuffix = $value if ($key eq 'CppSourceSuffix');
}

# -- Determine file names
my $baseFileName = lc($fieldClassName);
my $sourceFileName = $baseFileName . '.' . $standardFieldCppSourceSuffix;
my $headerFileName = $baseFileName . '.' . $standardFieldCppHeaderSuffix;
my $mainSourceFileName = 'main.' . $standardFieldCppSourceSuffix;
my $projectFileName = lc($standardFieldProjectName) . '.pro';

if ($optDryRun) {
#   -- Step 1) Dry run: Print file names along with attributes
    print $sourceFileName,",openeditor\n";
    print $headerFileName,",openeditor\n";
    print $mainSourceFileName,",openeditor\n";
    print $projectFileName,",openproject\n";
} else {
#   -- Step 2) Actual file creation
    print 'Generating ',  $headerFileName, ' ', $sourceFileName, ' ',
          $mainSourceFileName, ' ', $projectFileName, "\n";
    my $headerFile = new IO::File('>' . $headerFileName) or die ('Unable to open ' . $headerFileName . ' :' . $!);
    print $headerFile '#ifndef ', uc($fieldClassName), "_H\n#define ", uc($fieldClassName), "_H\n\n",
          'class ', $fieldClassName, "{\npublic:\n    ", $fieldClassName, "();\n\n};\n\n#endif\n";
    $headerFile->close();

    my $sourceFile = new IO::File('>' . $sourceFileName) or die ('Unable to open ' . $sourceFileName . ' :' . $!);
    print $sourceFile  '#include "', $headerFileName ,"\"\n\n",
            $fieldClassName,'::', $fieldClassName, "()\n{\n}\n";
    $sourceFile->close();

    my $mainSourceFile = new IO::File('>' . $mainSourceFileName) or die ('Unable to open ' . $mainSourceFileName . ' :' . $!);
    print $mainSourceFile  '#include "', $headerFileName ,"\"\n\n",
          "int main(int argc, char *argv[])\n{\n    ", $fieldClassName,' ', lc($fieldClassName),
          ";\n    return 0;\n}\n";
    $mainSourceFile->close();

    my $projectFile = new IO::File('>' . $projectFileName) or die ('Unable to open ' . $projectFileName . ' :' . $!);
    print $projectFile "TEMPLATE = app\nQT -= core\nCONFIG += console\nTARGET = ", $standardFieldProjectName,
          "\nSOURCES += ", $sourceFileName, ' ',$headerFileName, ' ', $mainSourceFileName,
          "\nHEADERS += ", $headerFileName,"\n";
    $projectFile->close();
}
