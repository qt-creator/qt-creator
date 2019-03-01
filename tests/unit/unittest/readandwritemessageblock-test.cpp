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

#include <clangcodemodelclientmessages.h>
#include <clangcodemodelservermessages.h>

#include <diagnosticcontainer.h>
#include <tokeninfocontainer.h>
#include <messageenvelop.h>
#include <readmessageblock.h>
#include <writemessageblock.h>

#include <QBuffer>
#include <QString>
#include <QVariant>

#include <vector>

using namespace testing;
namespace CodeModelBackeEndTest {

class ReadAndWriteMessageBlock : public ::testing::Test
{
protected:
    ReadAndWriteMessageBlock();

    virtual void SetUp() override;
    virtual void TearDown() override;

    template<class Type>
    void CompareMessage(const Type &message);

    ClangBackEnd::MessageEnvelop writeCompletionsMessage();
    void popLastCharacterFromBuffer();
    void pushLastCharacterToBuffer();
    void readPartialMessage();

protected:
    Utf8String filePath{Utf8StringLiteral("foo.cpp")};
    ClangBackEnd::FileContainer fileContainer{filePath,
                                              Utf8StringLiteral("unsaved content"),
                                              true,
                                              1};
    QBuffer buffer;
    ClangBackEnd::WriteMessageBlock writeMessageBlock;
    ClangBackEnd::ReadMessageBlock readMessageBlock;
    char lastCharacter = 0;
};

ReadAndWriteMessageBlock::ReadAndWriteMessageBlock()
    :  writeMessageBlock(&buffer),
       readMessageBlock(&buffer)
{
}

void ReadAndWriteMessageBlock::SetUp()
{
    buffer.open(QIODevice::ReadWrite);
    writeMessageBlock = ClangBackEnd::WriteMessageBlock(&buffer);
    readMessageBlock = ClangBackEnd::ReadMessageBlock(&buffer);
}

void ReadAndWriteMessageBlock::TearDown()
{
    buffer.close();
}

TEST_F(ReadAndWriteMessageBlock, WriteMessageAndTestSize)
{
    writeMessageBlock.write(ClangBackEnd::EndMessage());

    ASSERT_EQ(17, buffer.size());
}

TEST_F(ReadAndWriteMessageBlock, WriteSecondMessageAndTestSize)
{
    writeMessageBlock.write(ClangBackEnd::EndMessage());

    ASSERT_EQ(17, buffer.size());
}

TEST_F(ReadAndWriteMessageBlock, WriteTwoMessagesAndTestCount)
{
    writeMessageBlock.write(ClangBackEnd::EndMessage());
    writeMessageBlock.write(ClangBackEnd::EndMessage());

    ASSERT_EQ(2, writeMessageBlock.counter());
}

TEST_F(ReadAndWriteMessageBlock, ReadThreeMessagesAndTestCount)
{
    writeMessageBlock.write(ClangBackEnd::EndMessage());
    writeMessageBlock.write(ClangBackEnd::EndMessage());
    writeMessageBlock.write(ClangBackEnd::EndMessage());
    buffer.seek(0);

    ASSERT_THAT(readMessageBlock.readAll(), SizeIs(3));
}

TEST_F(ReadAndWriteMessageBlock, WriteMessagesToWriteBlockWithoutIoDeviceAndNoMessagesAreSent)
{
    ClangBackEnd::WriteMessageBlock writeMessageBlock;

    writeMessageBlock.write(ClangBackEnd::EndMessage());
    writeMessageBlock.write(ClangBackEnd::EndMessage());
    buffer.seek(0);

    ASSERT_THAT(readMessageBlock.readAll(), IsEmpty());
}

TEST_F(ReadAndWriteMessageBlock, WriteMessagesToWriteBlockWithoutIoDeviceAndSetIoDeviceToNullPointerLater)
{
    ClangBackEnd::WriteMessageBlock writeMessageBlock;
    writeMessageBlock.write(ClangBackEnd::EndMessage());
    writeMessageBlock.write(ClangBackEnd::EndMessage());

    writeMessageBlock.setLocalSocket(nullptr);
    buffer.seek(0);

    ASSERT_THAT(readMessageBlock.readAll(), IsEmpty());
}

TEST_F(ReadAndWriteMessageBlock, WriteMessagesToWriteBlockWithoutIoDeviceAndSetIoDeviceLater)
{
    ClangBackEnd::WriteMessageBlock writeMessageBlock;
    writeMessageBlock.write(ClangBackEnd::EndMessage());
    writeMessageBlock.write(ClangBackEnd::EndMessage());

    writeMessageBlock.setIoDevice(&buffer);
    buffer.seek(0);

    ASSERT_THAT(readMessageBlock.readAll(), SizeIs(2));
}

TEST_F(ReadAndWriteMessageBlock, ResetStateResetsCounter)
{
    writeMessageBlock.write(ClangBackEnd::EndMessage());
    writeMessageBlock.write(ClangBackEnd::EndMessage());
    buffer.seek(0);

    writeMessageBlock.resetState();

    ASSERT_THAT(writeMessageBlock.counter(), 0);
}

TEST_F(ReadAndWriteMessageBlock, ResetStateResetsWritingBlock)
{
    ClangBackEnd::WriteMessageBlock writeMessageBlock;
    writeMessageBlock.write(ClangBackEnd::EndMessage());

    writeMessageBlock.resetState();

    writeMessageBlock.setIoDevice(&buffer);
    buffer.seek(0);
    ASSERT_THAT(readMessageBlock.readAll(), IsEmpty());
}

TEST_F(ReadAndWriteMessageBlock, CompareEndMessage)
{
    CompareMessage(ClangBackEnd::EndMessage());
}

TEST_F(ReadAndWriteMessageBlock, CompareAliveMessage)
{
    CompareMessage(ClangBackEnd::AliveMessage());
}

TEST_F(ReadAndWriteMessageBlock, CompareDocumentsOpenedMessage)
{
    CompareMessage(ClangBackEnd::DocumentsOpenedMessage({fileContainer}, filePath, {filePath}));
}

TEST_F(ReadAndWriteMessageBlock, CompareDocumentsChangedMessage)
{
    CompareMessage(ClangBackEnd::DocumentsChangedMessage({fileContainer}));
}

TEST_F(ReadAndWriteMessageBlock, CompareDocumentsClosedMessage)
{
    CompareMessage(ClangBackEnd::DocumentsClosedMessage({fileContainer}));
}

TEST_F(ReadAndWriteMessageBlock, CompareRequestCompletionsMessage)
{
    CompareMessage(ClangBackEnd::RequestCompletionsMessage(Utf8StringLiteral("foo.cpp"), 24, 33));
}

TEST_F(ReadAndWriteMessageBlock, CompareCompletionsMessage)
{
    ClangBackEnd::CodeCompletions codeCompletions({Utf8StringLiteral("newFunction()")});

    CompareMessage(ClangBackEnd::CompletionsMessage(codeCompletions, 1));
}

TEST_F(ReadAndWriteMessageBlock, CompareAnnotationsMessage)
{
    ClangBackEnd::DiagnosticContainer diagnostic(Utf8StringLiteral("don't do that"),
                                                Utf8StringLiteral("warning"),
                                                {Utf8StringLiteral("-Wpadded"), Utf8StringLiteral("-Wno-padded")},
                                                ClangBackEnd::DiagnosticSeverity::Warning,
                                                {Utf8StringLiteral("foo.cpp"), 20u, 103u},
                                                {{{Utf8StringLiteral("foo.cpp"), 20u, 103u}, {Utf8StringLiteral("foo.cpp"), 20u, 110u}}},
                                                {},
                                                {});

    ClangBackEnd::HighlightingTypes types;
    types.mainHighlightingType = ClangBackEnd::HighlightingType::Keyword;
    ClangBackEnd::TokenInfoContainer tokenInfo(1, 1, 1, types);

    CompareMessage(ClangBackEnd::AnnotationsMessage(fileContainer,
                                                    {diagnostic},
                                                    {},
                                                    {tokenInfo},
                                                    QVector<ClangBackEnd::SourceRangeContainer>()));
}

TEST_F(ReadAndWriteMessageBlock, CompareUnsavedFilesUpdatedMessage)
{
    CompareMessage(ClangBackEnd::UnsavedFilesUpdatedMessage({fileContainer}));
}

TEST_F(ReadAndWriteMessageBlock, CompareUnsavedFilesRemovedMessage)
{
    CompareMessage(ClangBackEnd::UnsavedFilesRemovedMessage({fileContainer}));
}

TEST_F(ReadAndWriteMessageBlock, CompareRequestAnnotationsMessage)
{
    CompareMessage(ClangBackEnd::RequestAnnotationsMessage(fileContainer));
}

TEST_F(ReadAndWriteMessageBlock, CompareRequestReferencesMessage)
{
    CompareMessage(ClangBackEnd::RequestReferencesMessage{fileContainer, 13, 37});
}

TEST_F(ReadAndWriteMessageBlock, CompareRequestFollowSymbolMessage)
{
    CompareMessage(ClangBackEnd::RequestFollowSymbolMessage{fileContainer, 13, 37});
}

TEST_F(ReadAndWriteMessageBlock, CompareReferencesMessage)
{
    const QVector<ClangBackEnd::SourceRangeContainer> references{
        true,
        {{fileContainer.filePath, 12, 34},
         {fileContainer.filePath, 56, 78}}
    };
    CompareMessage(ClangBackEnd::ReferencesMessage(fileContainer, references, true, 1));
}

TEST_F(ReadAndWriteMessageBlock, GetInvalidMessageForAPartialBuffer)
{
    writeCompletionsMessage();
    popLastCharacterFromBuffer();
    buffer.seek(0);

    readPartialMessage();
}

TEST_F(ReadAndWriteMessageBlock, ReadMessageAfterInterruption)
{
    const auto writeMessage = writeCompletionsMessage();
    popLastCharacterFromBuffer();
    buffer.seek(0);
    readPartialMessage();
    pushLastCharacterToBuffer();

    ASSERT_EQ(readMessageBlock.read(), writeMessage);
}

ClangBackEnd::MessageEnvelop ReadAndWriteMessageBlock::writeCompletionsMessage()
{
    ClangBackEnd::CompletionsMessage message(
        ClangBackEnd::CodeCompletions({Utf8StringLiteral("newFunction()")}), 1);

    writeMessageBlock.write(message);

    return message;
}

void ReadAndWriteMessageBlock::popLastCharacterFromBuffer()
{
    auto &internalBuffer = buffer.buffer();
    lastCharacter = internalBuffer.at(internalBuffer.size() - 1);
    internalBuffer.chop(1);
}

void ReadAndWriteMessageBlock::pushLastCharacterToBuffer()
{
    buffer.buffer().push_back(lastCharacter);
}

void ReadAndWriteMessageBlock::readPartialMessage()
{
    const ClangBackEnd::MessageEnvelop readMessage = readMessageBlock.read();

    ASSERT_FALSE(readMessage.isValid());
}

template<class Type>
void ReadAndWriteMessageBlock::CompareMessage(const Type &message)
{
    const ClangBackEnd::MessageEnvelop writeMessage = message;
    writeMessageBlock.write(writeMessage);
    buffer.seek(0);

    const ClangBackEnd::MessageEnvelop readMessage = readMessageBlock.read();

    ASSERT_EQ(writeMessage, readMessage);
}

}
