// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <textmodifier.h>

class TextModifierMock : public QmlDesigner::TextModifier
{
    MOCK_METHOD(void, replace, (int offset, int length, const QString &replacement), (override));
    MOCK_METHOD(void, move, (const MoveInfo &moveInfo), (override));
    MOCK_METHOD(void, indent, (int offset, int length), (override));
    MOCK_METHOD(void, indentLines, (int startLine, int endLine), (override));
    MOCK_METHOD(TextEditor::TabSettings, tabSettings, (), (const, override));
    MOCK_METHOD(void, startGroup, (), (override));
    MOCK_METHOD(void, flushGroup, (), (override));
    MOCK_METHOD(void, commitGroup, (), (override));
    MOCK_METHOD(QTextDocument *, textDocument, (), (const, override));
    MOCK_METHOD(QString, text, (), (const, override));
    MOCK_METHOD(QTextCursor, textCursor, (), (const, override));
    MOCK_METHOD(void, deactivateChangeSignals, (), (override));
    MOCK_METHOD(void, reactivateChangeSignals, (), (override));
    MOCK_METHOD(bool, renameId, (const QString &oldId, const QString &newId), (override));
    MOCK_METHOD(QStringList,
                autoComplete,
                (QTextDocument * textDocument, int positio, bool explicitComplete),
                (override));
    MOCK_METHOD(bool, moveToComponent, (int nodeOffset, const QString &importData), (override));
};
