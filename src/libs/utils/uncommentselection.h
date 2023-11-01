// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QString>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QTextCursor;
QT_END_NAMESPACE

namespace Utils {

class MultiTextCursor;

class QTCREATOR_UTILS_EXPORT CommentDefinition
{
public:
    static CommentDefinition CppStyle;
    static CommentDefinition HashStyle;

    CommentDefinition();
    CommentDefinition(const QString &single,
                      const QString &multiStart = QString(), const QString &multiEnd = QString());

    bool isValid() const;
    bool hasSingleLineStyle() const;
    bool hasMultiLineStyle() const;

public:
    bool isAfterWhitespace = false;
    QString singleLine;
    QString multiLineStart;
    QString multiLineEnd;
};

QTCREATOR_UTILS_EXPORT
QTextCursor unCommentSelection(const QTextCursor &cursor,
                               const CommentDefinition &definiton = CommentDefinition(),
                               bool preferSingleLine = false);

QTCREATOR_UTILS_EXPORT
MultiTextCursor unCommentSelection(const MultiTextCursor &cursor,
                                   const CommentDefinition &definiton = CommentDefinition(),
                                   bool preferSingleLine = false);

} // namespace Utils
