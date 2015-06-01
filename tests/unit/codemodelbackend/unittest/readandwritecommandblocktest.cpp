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

#include "gtest/gtest.h"
#include "gmock/gmock-matchers.h"
#include "gtest-qt-printing.h"

#include <QString>
#include <QBuffer>
#include <QVariant>
#include <vector>

#include <cmbendcommand.h>
#include <cmbalivecommand.h>
#include <cmbcommands.h>
#include <cmbregistertranslationunitsforcodecompletioncommand.h>
#include <cmbunregistertranslationunitsforcodecompletioncommand.h>
#include <cmbcodecompletedcommand.h>
#include <cmbcompletecodecommand.h>
#include <writecommandblock.h>
#include <readcommandblock.h>

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

    QBuffer buffer;
    CodeModelBackEnd::WriteCommandBlock writeCommandBlock;
    CodeModelBackEnd::ReadCommandBlock readCommandBlock;
};

ReadAndWriteCommandBlockTest::ReadAndWriteCommandBlockTest()
    :  writeCommandBlock(&buffer),
       readCommandBlock(&buffer)
{
}

void ReadAndWriteCommandBlockTest::SetUp()
{
    buffer.open(QIODevice::ReadWrite);
    writeCommandBlock = CodeModelBackEnd::WriteCommandBlock(&buffer);
    readCommandBlock = CodeModelBackEnd::ReadCommandBlock(&buffer);
}

void ReadAndWriteCommandBlockTest::TearDown()
{
    buffer.close();
}

TEST_F(ReadAndWriteCommandBlockTest, WriteCommandAndTestSize)
{
    writeCommandBlock.write(QVariant::fromValue(CodeModelBackEnd::EndCommand()));

    ASSERT_EQ(50, buffer.size());
}

TEST_F(ReadAndWriteCommandBlockTest, WriteSecondCommandAndTestSize)
{
    writeCommandBlock.write(QVariant::fromValue(CodeModelBackEnd::EndCommand()));

    ASSERT_EQ(50, buffer.size());
}

TEST_F(ReadAndWriteCommandBlockTest, WriteTwoCommandsAndTestCount)
{
    writeCommandBlock.write(QVariant::fromValue(CodeModelBackEnd::EndCommand()));
    writeCommandBlock.write(QVariant::fromValue(CodeModelBackEnd::EndCommand()));

    ASSERT_EQ(2, writeCommandBlock.counter());
}

TEST_F(ReadAndWriteCommandBlockTest, ReadThreeCommandsAndTestCount)
{
    writeCommandBlock.write(QVariant::fromValue(CodeModelBackEnd::EndCommand()));
    writeCommandBlock.write(QVariant::fromValue(CodeModelBackEnd::EndCommand()));
    writeCommandBlock.write(QVariant::fromValue(CodeModelBackEnd::EndCommand()));
    buffer.seek(0);

    ASSERT_EQ(3, readCommandBlock.readAll().count());
}

TEST_F(ReadAndWriteCommandBlockTest, CompareEndCommand)
{
    CompareCommand(CodeModelBackEnd::EndCommand());
}

TEST_F(ReadAndWriteCommandBlockTest, CompareAliveCommand)
{
    CompareCommand(CodeModelBackEnd::AliveCommand());
}

TEST_F(ReadAndWriteCommandBlockTest, CompareRegisterTranslationUnitForCodeCompletionCommand)
{
    CodeModelBackEnd::FileContainer fileContainer(Utf8StringLiteral("foo.cpp"), Utf8StringLiteral("pathToProject.pro"));
    QVector<CodeModelBackEnd::FileContainer> fileContainers({fileContainer});

    CompareCommand(CodeModelBackEnd::RegisterTranslationUnitForCodeCompletionCommand(fileContainers));
}

TEST_F(ReadAndWriteCommandBlockTest, CompareUnregisterFileForCodeCompletionCommand)
{
    CodeModelBackEnd::FileContainer fileContainer(Utf8StringLiteral("foo.cpp"), Utf8StringLiteral("pathToProject.pro"));

    CompareCommand(CodeModelBackEnd::UnregisterTranslationUnitsForCodeCompletionCommand({fileContainer}));
}

TEST_F(ReadAndWriteCommandBlockTest, CompareCompleteCodeCommand)
{
    CompareCommand(CodeModelBackEnd::CompleteCodeCommand(Utf8StringLiteral("foo.cpp"), 24, 33, Utf8StringLiteral("do what I want")));
}

TEST_F(ReadAndWriteCommandBlockTest, CompareCodeCompletedCommand)
{
    QVector<CodeModelBackEnd::CodeCompletion> codeCompletions({Utf8StringLiteral("newFunction()")});

    CompareCommand(CodeModelBackEnd::CodeCompletedCommand(codeCompletions, 1));
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


