/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <lineprefixer.h>
#include <utf8string.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

namespace {

QByteArray runPrefixer(QList<QByteArray> inputChunks)
{
    QByteArray actualOutput;
    ClangBackEnd::LinePrefixer prefixer("PREFIX ");
    foreach (const QByteArray &chunk, inputChunks)
        actualOutput += prefixer.prefix(chunk);
    return actualOutput;
}

TEST(LinePrefixer, OneChunkEndsWithNewline)
{
    const QList<QByteArray> inputChunks { "hello\n" };
    ASSERT_THAT(Utf8String::fromByteArray(runPrefixer(inputChunks)),
                Utf8String::fromUtf8("PREFIX hello\n"));
}

TEST(LinePrefixer, OneChunkEndsWithoutNewline)
{
    const QList<QByteArray> inputChunks { "hello" };
    ASSERT_THAT(Utf8String::fromByteArray(runPrefixer(inputChunks)),
                Utf8String::fromUtf8("PREFIX hello"));
}

TEST(LinePrefixer, OneChunkStartsWithNewline)
{
    const QList<QByteArray> inputChunks { "\nhello" };
    ASSERT_THAT(Utf8String::fromByteArray(runPrefixer(inputChunks)),
                Utf8String::fromUtf8("PREFIX \n"
                                     "PREFIX hello"));
}

TEST(LinePrefixer, OneChunkStartsAndEndsWithNewline)
{
    const QList<QByteArray> inputChunks { "\nhello\n" };
    ASSERT_THAT(Utf8String::fromByteArray(runPrefixer(inputChunks)),
                Utf8String::fromUtf8("PREFIX \n"
                                     "PREFIX hello\n"));
}

TEST(LinePrefixer, OneChunkEndsWithExtraNewline)
{
    const QList<QByteArray> inputChunks { "hello\n\n" };
    ASSERT_THAT(Utf8String::fromByteArray(runPrefixer(inputChunks)),
                Utf8String::fromUtf8("PREFIX hello\n"
                                     "PREFIX \n"));
}

TEST(LinePrefixer, OneChunkEndsWithTwoExtraNewlines)
{
    const QList<QByteArray> inputChunks { "hello\n\n\n" };
    ASSERT_THAT(Utf8String::fromByteArray(runPrefixer(inputChunks)),
                Utf8String::fromUtf8("PREFIX hello\n"
                                     "PREFIX \n"
                                     "PREFIX \n"));
}

TEST(LinePrefixer, ChunkWithoutNewlineAndChunkWithNewline)
{
    const QList<QByteArray> inputChunks { "hello", "\n" };
    ASSERT_THAT(Utf8String::fromByteArray(runPrefixer(inputChunks)),
                Utf8String::fromUtf8("PREFIX hello\n"));
}

TEST(LinePrefixer, ChunkWithoutNewlineAndChunkWithTwoNewlines)
{
    const QList<QByteArray> inputChunks { "hello", "\n\n" };
    ASSERT_THAT(Utf8String::fromByteArray(runPrefixer(inputChunks)),
                Utf8String::fromUtf8("PREFIX hello\n"
                                     "PREFIX \n"));
}

TEST(LinePrefixer, ChunkWithTwoNewlinesAndChunkWithoutNewline)
{
    const QList<QByteArray> inputChunks { "\n\n", "hello" };
    ASSERT_THAT(Utf8String::fromByteArray(runPrefixer(inputChunks)),
                Utf8String::fromUtf8("PREFIX \n"
                                     "PREFIX \n"
                                     "PREFIX hello"));
}

} // anonymous namespace
