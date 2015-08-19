/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include <cmbalivemessage.h>
#include <cmbcodecompletedmessage.h>
#include <cmbmessages.h>
#include <cmbcompletecodemessage.h>
#include <cmbendmessage.h>
#include <cmbregistertranslationunitsforcodecompletionmessage.h>
#include <cmbunregistertranslationunitsforcodecompletionmessage.h>
#include <readmessageblock.h>
#include <writemessageblock.h>

#include <QBuffer>
#include <QString>
#include <QVariant>
#include <vector>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using namespace testing;
namespace CodeModelBackeEndTest {

class ReadAndWriteMessageBlockTest : public ::testing::Test
{
protected:
    ReadAndWriteMessageBlockTest();

    virtual void SetUp() override;
    virtual void TearDown() override;

    template<class Type>
    void CompareMessage(const Type &message);

    QVariant writeCodeCompletedMessage();
    void popLastCharacterFromBuffer();
    void pushLastCharacterToBuffer();
    void readPartialMessage();

protected:
    QBuffer buffer;
    ClangBackEnd::WriteMessageBlock writeMessageBlock;
    ClangBackEnd::ReadMessageBlock readMessageBlock;
    char lastCharacter = 0;
};

ReadAndWriteMessageBlockTest::ReadAndWriteMessageBlockTest()
    :  writeMessageBlock(&buffer),
       readMessageBlock(&buffer)
{
}

void ReadAndWriteMessageBlockTest::SetUp()
{
    buffer.open(QIODevice::ReadWrite);
    writeMessageBlock = ClangBackEnd::WriteMessageBlock(&buffer);
    readMessageBlock = ClangBackEnd::ReadMessageBlock(&buffer);
}

void ReadAndWriteMessageBlockTest::TearDown()
{
    buffer.close();
}

TEST_F(ReadAndWriteMessageBlockTest, WriteMessageAndTestSize)
{
    writeMessageBlock.write(QVariant::fromValue(ClangBackEnd::EndMessage()));

    ASSERT_EQ(46, buffer.size());
}

TEST_F(ReadAndWriteMessageBlockTest, WriteSecondMessageAndTestSize)
{
    writeMessageBlock.write(QVariant::fromValue(ClangBackEnd::EndMessage()));

    ASSERT_EQ(46, buffer.size());
}

TEST_F(ReadAndWriteMessageBlockTest, WriteTwoMessagesAndTestCount)
{
    writeMessageBlock.write(QVariant::fromValue(ClangBackEnd::EndMessage()));
    writeMessageBlock.write(QVariant::fromValue(ClangBackEnd::EndMessage()));

    ASSERT_EQ(2, writeMessageBlock.counter());
}

TEST_F(ReadAndWriteMessageBlockTest, ReadThreeMessagesAndTestCount)
{
    writeMessageBlock.write(QVariant::fromValue(ClangBackEnd::EndMessage()));
    writeMessageBlock.write(QVariant::fromValue(ClangBackEnd::EndMessage()));
    writeMessageBlock.write(QVariant::fromValue(ClangBackEnd::EndMessage()));
    buffer.seek(0);

    ASSERT_EQ(3, readMessageBlock.readAll().count());
}

TEST_F(ReadAndWriteMessageBlockTest, CompareEndMessage)
{
    CompareMessage(ClangBackEnd::EndMessage());
}

TEST_F(ReadAndWriteMessageBlockTest, CompareAliveMessage)
{
    CompareMessage(ClangBackEnd::AliveMessage());
}

TEST_F(ReadAndWriteMessageBlockTest, CompareRegisterTranslationUnitForCodeCompletionMessage)
{
    ClangBackEnd::FileContainer fileContainer(Utf8StringLiteral("foo.cpp"), Utf8StringLiteral("pathToProject.pro"));
    QVector<ClangBackEnd::FileContainer> fileContainers({fileContainer});

    CompareMessage(ClangBackEnd::RegisterTranslationUnitForCodeCompletionMessage(fileContainers));
}

TEST_F(ReadAndWriteMessageBlockTest, CompareUnregisterFileForCodeCompletionMessage)
{
    ClangBackEnd::FileContainer fileContainer(Utf8StringLiteral("foo.cpp"), Utf8StringLiteral("pathToProject.pro"));

    CompareMessage(ClangBackEnd::UnregisterTranslationUnitsForCodeCompletionMessage({fileContainer}));
}

TEST_F(ReadAndWriteMessageBlockTest, CompareCompleteCodeMessage)
{
    CompareMessage(ClangBackEnd::CompleteCodeMessage(Utf8StringLiteral("foo.cpp"), 24, 33, Utf8StringLiteral("do what I want")));
}

TEST_F(ReadAndWriteMessageBlockTest, CompareCodeCompletedMessage)
{
    ClangBackEnd::CodeCompletions codeCompletions({Utf8StringLiteral("newFunction()")});

    CompareMessage(ClangBackEnd::CodeCompletedMessage(codeCompletions, 1));
}

TEST_F(ReadAndWriteMessageBlockTest, GetInvalidMessageForAPartialBuffer)
{
    writeCodeCompletedMessage();
    popLastCharacterFromBuffer();
    buffer.seek(0);

    readPartialMessage();
}

TEST_F(ReadAndWriteMessageBlockTest, ReadMessageAfterInterruption)
{
    const QVariant writeMessage = writeCodeCompletedMessage();
    popLastCharacterFromBuffer();
    buffer.seek(0);
    readPartialMessage();
    pushLastCharacterToBuffer();

    ASSERT_EQ(readMessageBlock.read(), writeMessage);
}

QVariant ReadAndWriteMessageBlockTest::writeCodeCompletedMessage()
{
    ClangBackEnd::CodeCompletedMessage message(ClangBackEnd::CodeCompletions({Utf8StringLiteral("newFunction()")}), 1);
    const QVariant writeMessage = QVariant::fromValue(message);
    writeMessageBlock.write(writeMessage);

    return writeMessage;
}

void ReadAndWriteMessageBlockTest::popLastCharacterFromBuffer()
{
    auto &internalBuffer = buffer.buffer();
    lastCharacter = internalBuffer.at(internalBuffer.size() - 1);
    internalBuffer.chop(1);
}

void ReadAndWriteMessageBlockTest::pushLastCharacterToBuffer()
{
    buffer.buffer().push_back(lastCharacter);
}

void ReadAndWriteMessageBlockTest::readPartialMessage()
{
    QVariant readMessage = readMessageBlock.read();

    ASSERT_FALSE(readMessage.isValid());
}

template<class Type>
void ReadAndWriteMessageBlockTest::CompareMessage(const Type &message)
{
    const QVariant writeMessage = QVariant::fromValue(message);
    writeMessageBlock.write(writeMessage);
    buffer.seek(0);

    const QVariant readMessage = readMessageBlock.read();

    ASSERT_EQ(writeMessage, readMessage);
}

}
