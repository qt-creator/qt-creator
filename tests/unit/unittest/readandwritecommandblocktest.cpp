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

#include <cmbalivecommand.h>
#include <cmbcodecompletedcommand.h>
#include <cmbcommands.h>
#include <cmbcompletecodecommand.h>
#include <cmbendcommand.h>
#include <cmbregistertranslationunitsforcodecompletioncommand.h>
#include <cmbunregistertranslationunitsforcodecompletioncommand.h>
#include <readcommandblock.h>
#include <writecommandblock.h>

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

class ReadAndWriteCommandBlockTest : public ::testing::Test
{
protected:
    ReadAndWriteCommandBlockTest();

    virtual void SetUp() override;
    virtual void TearDown() override;

    template<class Type>
    void CompareCommand(const Type &command);

    QVariant writeCodeCompletedCommand();
    void popLastCharacterFromBuffer();
    void pushLastCharacterToBuffer();
    void readPartialCommand();

protected:
    QBuffer buffer;
    ClangBackEnd::WriteCommandBlock writeCommandBlock;
    ClangBackEnd::ReadCommandBlock readCommandBlock;
    char lastCharacter = 0;
};

ReadAndWriteCommandBlockTest::ReadAndWriteCommandBlockTest()
    :  writeCommandBlock(&buffer),
       readCommandBlock(&buffer)
{
}

void ReadAndWriteCommandBlockTest::SetUp()
{
    buffer.open(QIODevice::ReadWrite);
    writeCommandBlock = ClangBackEnd::WriteCommandBlock(&buffer);
    readCommandBlock = ClangBackEnd::ReadCommandBlock(&buffer);
}

void ReadAndWriteCommandBlockTest::TearDown()
{
    buffer.close();
}

TEST_F(ReadAndWriteCommandBlockTest, WriteCommandAndTestSize)
{
    writeCommandBlock.write(QVariant::fromValue(ClangBackEnd::EndCommand()));

    ASSERT_EQ(46, buffer.size());
}

TEST_F(ReadAndWriteCommandBlockTest, WriteSecondCommandAndTestSize)
{
    writeCommandBlock.write(QVariant::fromValue(ClangBackEnd::EndCommand()));

    ASSERT_EQ(46, buffer.size());
}

TEST_F(ReadAndWriteCommandBlockTest, WriteTwoCommandsAndTestCount)
{
    writeCommandBlock.write(QVariant::fromValue(ClangBackEnd::EndCommand()));
    writeCommandBlock.write(QVariant::fromValue(ClangBackEnd::EndCommand()));

    ASSERT_EQ(2, writeCommandBlock.counter());
}

TEST_F(ReadAndWriteCommandBlockTest, ReadThreeCommandsAndTestCount)
{
    writeCommandBlock.write(QVariant::fromValue(ClangBackEnd::EndCommand()));
    writeCommandBlock.write(QVariant::fromValue(ClangBackEnd::EndCommand()));
    writeCommandBlock.write(QVariant::fromValue(ClangBackEnd::EndCommand()));
    buffer.seek(0);

    ASSERT_EQ(3, readCommandBlock.readAll().count());
}

TEST_F(ReadAndWriteCommandBlockTest, CompareEndCommand)
{
    CompareCommand(ClangBackEnd::EndCommand());
}

TEST_F(ReadAndWriteCommandBlockTest, CompareAliveCommand)
{
    CompareCommand(ClangBackEnd::AliveCommand());
}

TEST_F(ReadAndWriteCommandBlockTest, CompareRegisterTranslationUnitForCodeCompletionCommand)
{
    ClangBackEnd::FileContainer fileContainer(Utf8StringLiteral("foo.cpp"), Utf8StringLiteral("pathToProject.pro"));
    QVector<ClangBackEnd::FileContainer> fileContainers({fileContainer});

    CompareCommand(ClangBackEnd::RegisterTranslationUnitForCodeCompletionCommand(fileContainers));
}

TEST_F(ReadAndWriteCommandBlockTest, CompareUnregisterFileForCodeCompletionCommand)
{
    ClangBackEnd::FileContainer fileContainer(Utf8StringLiteral("foo.cpp"), Utf8StringLiteral("pathToProject.pro"));

    CompareCommand(ClangBackEnd::UnregisterTranslationUnitsForCodeCompletionCommand({fileContainer}));
}

TEST_F(ReadAndWriteCommandBlockTest, CompareCompleteCodeCommand)
{
    CompareCommand(ClangBackEnd::CompleteCodeCommand(Utf8StringLiteral("foo.cpp"), 24, 33, Utf8StringLiteral("do what I want")));
}

TEST_F(ReadAndWriteCommandBlockTest, CompareCodeCompletedCommand)
{
    ClangBackEnd::CodeCompletions codeCompletions({Utf8StringLiteral("newFunction()")});

    CompareCommand(ClangBackEnd::CodeCompletedCommand(codeCompletions, 1));
}

TEST_F(ReadAndWriteCommandBlockTest, GetInvalidCommandForAPartialBuffer)
{
    writeCodeCompletedCommand();
    popLastCharacterFromBuffer();
    buffer.seek(0);

    readPartialCommand();
}

TEST_F(ReadAndWriteCommandBlockTest, ReadCommandAfterInterruption)
{
    const QVariant writeCommand = writeCodeCompletedCommand();
    popLastCharacterFromBuffer();
    buffer.seek(0);
    readPartialCommand();
    pushLastCharacterToBuffer();

    ASSERT_EQ(readCommandBlock.read(), writeCommand);
}

QVariant ReadAndWriteCommandBlockTest::writeCodeCompletedCommand()
{
    ClangBackEnd::CodeCompletedCommand command(ClangBackEnd::CodeCompletions({Utf8StringLiteral("newFunction()")}), 1);
    const QVariant writeCommand = QVariant::fromValue(command);
    writeCommandBlock.write(writeCommand);

    return writeCommand;
}

void ReadAndWriteCommandBlockTest::popLastCharacterFromBuffer()
{
    auto &internalBuffer = buffer.buffer();
    lastCharacter = internalBuffer.at(internalBuffer.size() - 1);
    internalBuffer.chop(1);
}

void ReadAndWriteCommandBlockTest::pushLastCharacterToBuffer()
{
    buffer.buffer().push_back(lastCharacter);
}

void ReadAndWriteCommandBlockTest::readPartialCommand()
{
    QVariant readCommand = readCommandBlock.read();

    ASSERT_FALSE(readCommand.isValid());
}

template<class Type>
void ReadAndWriteCommandBlockTest::CompareCommand(const Type &command)
{
    const QVariant writeCommand = QVariant::fromValue(command);
    writeCommandBlock.write(writeCommand);
    buffer.seek(0);

    const QVariant readCommand = readCommandBlock.read();

    ASSERT_EQ(writeCommand, readCommand);
}

}
