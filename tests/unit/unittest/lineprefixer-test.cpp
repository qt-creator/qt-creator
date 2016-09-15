/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "googletest.h"

#include <lineprefixer.h>
#include <utf8string.h>

namespace {

QByteArray runPrefixer(QList<QByteArray> inputChunks);

TEST(LinePrefixer, OneChunkEndsWithNewline)
{
    const QList<QByteArray> inputChunks = {"hello\n"};

    auto text = runPrefixer(inputChunks);

    ASSERT_THAT(text, "PREFIX hello\n");
}

TEST(LinePrefixer, OneChunkEndsWithoutNewline)
{
    const QList<QByteArray> inputChunks = {"hello"};

    auto text = runPrefixer(inputChunks);

    ASSERT_THAT(text, "PREFIX hello");
}

TEST(LinePrefixer, OneChunkStartsWithNewline)
{
    const QList<QByteArray> inputChunks = {"\nhello"};

    auto text = runPrefixer(inputChunks);

    ASSERT_THAT(text, "PREFIX \n"
                      "PREFIX hello");
}

TEST(LinePrefixer, OneChunkStartsAndEndsWithNewline)
{
    const QList<QByteArray> inputChunks = {"\nhello\n"};

    auto text = runPrefixer(inputChunks);

    ASSERT_THAT(text, "PREFIX \n"
                      "PREFIX hello\n");
}

TEST(LinePrefixer, OneChunkEndsWithExtraNewline)
{
    const QList<QByteArray> inputChunks = {"hello\n\n"};

    auto text = runPrefixer(inputChunks);

    ASSERT_THAT(text, "PREFIX hello\n"
                      "PREFIX \n");
}

TEST(LinePrefixer, OneChunkEndsWithTwoExtraNewlines)
{
    const QList<QByteArray> inputChunks = {"hello\n\n\n"};

    auto text = runPrefixer(inputChunks);

    ASSERT_THAT(text, "PREFIX hello\n"
                      "PREFIX \n"
                      "PREFIX \n");
}

TEST(LinePrefixer, ChunkWithoutNewlineAndChunkWithNewline)
{
    const QList<QByteArray> inputChunks = {"hello", "\n"};

    auto text = runPrefixer(inputChunks);

    ASSERT_THAT(text, "PREFIX hello\n");
}

TEST(LinePrefixer, ChunkWithoutNewlineAndChunkWithTwoNewlines)
{
    const QList<QByteArray> inputChunks = {"hello", "\n\n"};

    auto text = runPrefixer(inputChunks);

    ASSERT_THAT(text, "PREFIX hello\n"
                      "PREFIX \n");
}

TEST(LinePrefixer, ChunkWithTwoNewlinesAndChunkWithoutNewline)
{
    const QList<QByteArray> inputChunks = {"\n\n", "hello"};

    auto text = runPrefixer(inputChunks);

    ASSERT_THAT(text, "PREFIX \n"
                      "PREFIX \n"
                      "PREFIX hello");
}

QByteArray runPrefixer(QList<QByteArray> inputChunks)
{
    QByteArray actualOutput;
    ClangBackEnd::LinePrefixer prefixer("PREFIX ");

    for (const auto &chunk : inputChunks)
        actualOutput += prefixer.prefix(chunk);

    return actualOutput;
}

} // anonymous namespace
