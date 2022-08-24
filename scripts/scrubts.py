#!/usr/bin/env python

# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

# See argparse description in main
#
# Run on all .ts files in Qt Creator from qtcreator root dir:
#  for tsfile in share/qtcreator/translations/qtcreator_*.ts; do python scripts/scrubts.py $tsfile -context FooBar; done

import argparse
import pathlib
import re
import sys


def rewriteLines(input, scrubbedContext, tsFilePath):
    result = []
    previouslyInContext = False
    contextWasPresent = False
    messageHashes = []

    lineIter = iter(input)
    for line in lineIter:
        # Context merging
        if line.count(r"</name>") == 1: # Any new context
            if line.count(scrubbedContext + r"</name>") == 1: # It the context being scrubbed
                contextWasPresent = True
                if previouslyInContext: # Previous context was a scrubbed context, so merge them
                    result = result[ : -2] # Remove recent:   </context>\n<context>
                    continue               # ...and skip this input line
                else:
                    previouslyInContext = True
            else:
                previouslyInContext = False

        # Message de-duplicating
        if previouslyInContext and line.count(r"<message>") == 1: # message in scrubbed context
            # Iterate through message
            messageLines = [line]
            for messageLine in lineIter:
                messageLines.append(messageLine)
                if messageLine.count(r"</message>") == 1: # message finished
                    break

            # Duplication check
            messageHash = hash(str(messageLines))
            if messageHash not in messageHashes:
                result = result + messageLines
                messageHashes.append(messageHash) # Append if not a duplicate

            continue

        result.append(line)

    if not contextWasPresent:
        error = f"Context \"{scrubbedContext}\" was not found in {tsFilePath}"
        sys.exit(error)

    return result


def processTsFile(tsFilePath, scrubbedContext):
    with open(tsFilePath, 'r') as tsInputFile:
        lines = tsInputFile.readlines()

    result = rewriteLines(lines, scrubbedContext, tsFilePath)

    with open(tsFilePath, 'w') as tsOutputFile:
        for line in result:
            tsOutputFile.write(line)


def main():
    parser = argparse.ArgumentParser(description='Rewrites a .ts file, removing duplicate messages '
                                                 'of a specified translation context and joining '
                                                 'adjacent occurrences of that context. '
                                                 'Unlike lrelease and lconvert, this script does '
                                                 'an exact comparison of the whole <message/> xml '
                                                 'tag.')
    parser.add_argument('tsfile',
                        help='The .ts file to be processed.',
                        type=pathlib.Path)
    parser.add_argument('-context',
                        help='Translation context to scrubbed.',
                        required=True)
    args = parser.parse_args()

    processTsFile(args.tsfile, args.context)

    return 0


if __name__ == '__main__':
    sys.exit(main())
