#!/usr/bin/env python

# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# See argparse description in main
#
# Run on all .ts files in Qt Creator from qtcreator root dir:
#  for tsfile in share/qtcreator/translations/qtcreator_*.ts; do python scripts/scrubts.py $tsfile -context FooBar; done

import argparse
import pathlib
import sys
from dataclasses import dataclass
from enum import Enum, auto

def rewriteLines(input, scrubbedContext, tsFilePath):
    result = []
    previouslyInContext = False
    contextWasPresent = False
    messageHashes = []
    mergedContextsCount = 0
    removedDuplicatesCount = 0

    lineIter = iter(input)
    for line in lineIter:
        # Context merging
        if line.count(r"</name>") == 1: # Any new context
            if line.count(scrubbedContext + r"</name>") == 1: # It the context being scrubbed
                contextWasPresent = True
                if previouslyInContext: # Previous context was a scrubbed context, so merge them
                    mergedContextsCount += 1
                    result = result[ : -2] # Remove recent:   </context>\n<context>
                    continue               # ...and skip this input line
                else:
                    previouslyInContext = True
            else:
                previouslyInContext = False

        # Message de-duplicating
        if previouslyInContext and line.count(r"<message") == 1: # message in scrubbed context
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
            else:
                removedDuplicatesCount += 1

            continue

        result.append(line)

    if not contextWasPresent:
        error = f"Context \"{scrubbedContext}\" was not found in {tsFilePath}"
        sys.exit(error)

    print (f"{tsFilePath}:")
    print (f"  {removedDuplicatesCount} identical duplicate message(s) removed.")
    print (f"  {mergedContextsCount} occurrence(s) of context \"{scrubbedContext}\" merged.")

    return result


def findDistinctDuplicates(input, scrubbedContext, tsFilePath):
    inContext = False
    inputLineNr = 0

    @dataclass
    class Translation:
        lineNr: int
        translationXml: []

    @dataclass
    class Source:
        sourceXml: str
        translations: []

    messages = {}

    lineIter = iter(input)
    for line in lineIter:
        inputLineNr += 1
        if line.count(r"</name>") == 1: # Any new context
            inContext = (line.count(scrubbedContext + r"</name>") == 1)
            continue
        if line.count(r"<message") == 0:
            continue
        if inContext:
            sourceXml = []
            translationXml = []
            lineNr = inputLineNr

            class Position(Enum):
                MESSAGESTART = auto()
                LOCATION = auto()
                SOURCE = auto()
                COMMENT = auto()
                EXTRACOMMENT = auto()
                TRANSLATORCOMMENT = auto()
                TRANSLATION = auto()
                MESSAGEOVER = auto()

            pos = Position.MESSAGESTART

            for messageLine in lineIter:
                inputLineNr += 1
                if messageLine.count(r"<location") == 1:
                    pos = Position.LOCATION
                elif messageLine.count(r"<source") == 1:
                    pos = Position.SOURCE
                elif messageLine.count(r"<comment") == 1:
                    pos = Position.COMMENT
                elif messageLine.count(r"<extracomment") == 1:
                    pos = Position.EXTRACOMMENT
                elif messageLine.count(r"<translatorcomment") == 1:
                    pos = Position.TRANSLATORCOMMENT
                elif messageLine.count(r"<translation") == 1:
                    pos = Position.TRANSLATION
                elif messageLine.count(r"</message>") == 1:
                    pos = Position.MESSAGEOVER

                if pos == Position.SOURCE or pos == Position.COMMENT:
                    sourceXml.append(messageLine)
                elif pos == Position.TRANSLATION or pos == Position.EXTRACOMMENT or pos == Position.TRANSLATORCOMMENT:
                    translationXml.append(messageLine)
                elif pos == Position.MESSAGEOVER:
                    break

            sourceXmlHash = hash(str(sourceXml))
            translation = Translation(lineNr, translationXml)
            if sourceXmlHash in messages:
                messages[sourceXmlHash].translations.append(translation)
            else:
                messages[sourceXmlHash] = Source(sourceXml, [translation])

    for sourceId in messages:
        source = messages[sourceId]
        translationsCount = len(source.translations)
        if translationsCount > 1:
            print (f"\n==========================================")
            print (f"\n{translationsCount} duplicates for source:")
            for sourceXmlLine in source.sourceXml:
                print (sourceXmlLine.rstrip())
            for translation in source.translations:
                print (f"\n{tsFilePath}:{translation.lineNr}")
                for translationXmlLine in translation.translationXml:
                    print (translationXmlLine.rstrip())


def processTsFile(tsFilePath, scrubbedContext):
    with open(tsFilePath, 'r') as tsInputFile:
        lines = tsInputFile.readlines()

    result = rewriteLines(lines, scrubbedContext, tsFilePath)
    if lines != result:
        with open(tsFilePath, 'w') as tsOutputFile:
            for line in result:
                tsOutputFile.write(line)

    findDistinctDuplicates(result, scrubbedContext, tsFilePath)


def main():
    parser = argparse.ArgumentParser(
        description='''Rewrites a .ts file, removing identical duplicate messages of a specified
                       translation context and joining adjacent occurrences of that context.
                       Unlike lrelease and lconvert, this script does an exact comparison of the
                       whole <message/> xml tag when removing duplicates.
                       Subsequently, the remaining duplicate messages with identical source but
                       different translation are listed with filename:linenumber.''')
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
