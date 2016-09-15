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

#include <QString>

#include <utf8stringvector.h>

using namespace ::testing;

TEST(Utf8, QStringConversionConstructor)
{
    ASSERT_THAT(Utf8String(QStringLiteral("text")), Utf8StringLiteral("text"));
}

TEST(Utf8, QByteArrayConversionFunction)
{
    ASSERT_THAT(Utf8String::fromByteArray("text"), Utf8StringLiteral("text"));
}

TEST(Utf8, QStringConversionFunction)
{
    ASSERT_THAT(Utf8String::fromString(QStringLiteral("text")), Utf8StringLiteral("text"));
}

TEST(Utf8, Utf8ConversationFunction)
{
    ASSERT_THAT(Utf8String::fromUtf8("text"), Utf8StringLiteral("text"));
}

TEST(Utf8, Mid)
{
    Utf8String text(Utf8StringLiteral("some text"));

    ASSERT_THAT(text.mid(5, 4), Utf8StringLiteral("text"));
    ASSERT_THAT(text.mid(5), Utf8StringLiteral("text"));
}

TEST(Utf8, ByteSize)
{
    ASSERT_THAT(Utf8StringLiteral("text").byteSize(), 4);
}

TEST(Utf8, Append)
{
    Utf8String text(Utf8StringLiteral("some "));
    text.append(Utf8StringLiteral("text"));

    ASSERT_THAT(text, Utf8StringLiteral("some text"));
}

TEST(Utf8, ToByteArray)
{
    Utf8String text(Utf8StringLiteral("some text"));

    ASSERT_THAT(text.toByteArray(), QByteArrayLiteral("some text"));
}

TEST(Utf8, ToString)
{
    Utf8String text(Utf8StringLiteral("some text"));

    ASSERT_THAT(text.toString(), QStringLiteral("some text"));
}


TEST(Utf8, Contains)
{
    Utf8String text(Utf8StringLiteral("some text"));

    ASSERT_TRUE(text.contains(Utf8StringLiteral("text")));
    ASSERT_TRUE(text.contains("text"));
    ASSERT_TRUE(text.contains('x'));
}

TEST(Utf8, EqualOperator)
{
    ASSERT_TRUE(Utf8StringLiteral("text") == Utf8StringLiteral("text"));
    ASSERT_FALSE(Utf8StringLiteral("text") == Utf8StringLiteral("text2"));
}

TEST(Utf8, SmallerOperator)
{
    ASSERT_TRUE(Utf8StringLiteral("some") < Utf8StringLiteral("text"));
    ASSERT_TRUE(Utf8StringLiteral("text") < Utf8StringLiteral("texta"));
    ASSERT_FALSE(Utf8StringLiteral("text") < Utf8StringLiteral("some"));
    ASSERT_FALSE(Utf8StringLiteral("text") < Utf8StringLiteral("text"));
}

TEST(Utf8, UnequalOperator)
{
    ASSERT_FALSE(Utf8StringLiteral("text") != Utf8StringLiteral("text"));
    ASSERT_TRUE(Utf8StringLiteral("text") != Utf8StringLiteral("text2"));
}

TEST(Utf8, Join)
{
    Utf8StringVector vector;

    vector.append(Utf8StringLiteral("some"));
    vector.append(Utf8StringLiteral("text"));

    ASSERT_THAT(Utf8StringLiteral("some, text"), vector.join(Utf8StringLiteral(", ")));
}

TEST(Utf8, Split)
{
    Utf8String test(Utf8StringLiteral("some text"));

    Utf8StringVector splittedText = test.split(' ');

    ASSERT_THAT(splittedText.at(0), Utf8StringLiteral("some"));
    ASSERT_THAT(splittedText.at(1), Utf8StringLiteral("text"));
}

TEST(Utf8, IsEmpty)
{
    ASSERT_FALSE(Utf8StringLiteral("text").isEmpty());
    ASSERT_TRUE(Utf8String().isEmpty());
}

TEST(Utf8, HasContent)
{
    ASSERT_TRUE(Utf8StringLiteral("text").hasContent());
    ASSERT_FALSE(Utf8String().hasContent());
}

TEST(Utf8, Replace)
{
    Utf8String text(Utf8StringLiteral("some text"));

    text.replace(Utf8StringLiteral("some"), Utf8StringLiteral("any"));

    ASSERT_THAT(text, Utf8StringLiteral("any text"));
}

TEST(Utf8, ReplaceNBytesFromIndexPosition)
{
    Utf8String text(Utf8StringLiteral("min"));

    text.replace(1, 1, Utf8StringLiteral("aa"));

    ASSERT_THAT(text, Utf8StringLiteral("maan"));
}

TEST(Utf8, StartsWith)
{
    Utf8String text(Utf8StringLiteral("$column"));

    ASSERT_TRUE(text.startsWith(Utf8StringLiteral("$col")));
    ASSERT_FALSE(text.startsWith(Utf8StringLiteral("col")));
    ASSERT_TRUE(text.startsWith("$col"));
    ASSERT_FALSE(text.startsWith("col"));
    ASSERT_TRUE(text.startsWith('$'));
    ASSERT_FALSE(text.startsWith('@'));
}

TEST(Utf8, EndsWith)
{
    Utf8String text(Utf8StringLiteral("/my/path"));

    ASSERT_TRUE(text.endsWith(Utf8StringLiteral("path")));
}

TEST(Utf8, Clear)
{
    Utf8String text(Utf8StringLiteral("$column"));

    text.clear();

    ASSERT_TRUE(text.isEmpty());
}

TEST(Utf8, Number)
{
    ASSERT_THAT(Utf8String::number(20), Utf8StringLiteral("20"));
}

TEST(Utf8, FromIntegerVector)
{
    QVector<int> integers({1, 2, 3});

    ASSERT_THAT(Utf8StringVector::fromIntegerVector(integers).join(Utf8StringLiteral(", ")), Utf8StringLiteral("1, 2, 3"));
}

TEST(Utf8, PlusOperator)
{
    auto text = Utf8StringLiteral("foo") + Utf8StringLiteral("bar");

    ASSERT_THAT(text, Utf8StringLiteral("foobar"));
}

TEST(Utf8, PlusAssignmentOperator)
{
    Utf8String text("foo", 3);

    text += Utf8StringLiteral("bar");

    ASSERT_THAT(text, Utf8StringLiteral("foobar"));
}

TEST(Utf8, CompareCharPointer)
{
    Utf8String text("foo", 3);

    ASSERT_TRUE(text == "foo");
    ASSERT_FALSE(text == "foo2");
}

TEST(Utf8, RemoveFastFromVectorFailed)
{
    Utf8StringVector values({Utf8StringLiteral("a"),
                             Utf8StringLiteral("b"),
                             Utf8StringLiteral("c"),
                             Utf8StringLiteral("d")});

    ASSERT_FALSE(values.removeFast(Utf8StringLiteral("e")));
}

TEST(Utf8, RemoveFastFromVector)
{
    Utf8StringVector values({Utf8StringLiteral("a"),
                             Utf8StringLiteral("b"),
                             Utf8StringLiteral("c"),
                             Utf8StringLiteral("d")});

    bool removed = values.removeFast(Utf8StringLiteral("b"));

    ASSERT_TRUE(removed);
    ASSERT_THAT(values, Not(Contains(Utf8StringLiteral("b"))));
}

TEST(Utf8, ConverteAutomaticallyFromQString)
{
    QString text(QStringLiteral("foo"));

    Utf8String utf8Text(text);

    ASSERT_THAT(utf8Text, Utf8StringLiteral("foo"));
}

TEST(Utf8, ConverteAutomaticallyToQString)
{
    Utf8String utf8Text(Utf8StringLiteral("foo"));

    QString text = utf8Text;

    ASSERT_THAT(text, QStringLiteral("foo"));
}

TEST(Utf8, ConverteAutomaticallyToQByteArray)
{
    Utf8String utf8Text(Utf8StringLiteral("foo"));

    QByteArray bytes = utf8Text;

    ASSERT_THAT(bytes, QByteArrayLiteral("foo"));
}

