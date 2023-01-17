// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor_global.h>

#include "snippetparser.h"

#include <QChar>
#include <QCoreApplication>
#include <QList>
#include <QString>

namespace TextEditor {

class TEXTEDITOR_EXPORT Snippet
{
public:
    explicit Snippet(const QString &groupId = QString(), const QString &id = QString());
    ~Snippet();

    const QString &id() const;
    const QString &groupId() const;

    bool isBuiltIn() const;

    void setTrigger(const QString &trigger);
    const QString &trigger() const;
    static bool isValidTrigger(const QString &trigger);

    void setContent(const QString &content);
    const QString &content() const;

    void setComplement(const QString &complement);
    const QString &complement() const;

    void setIsRemoved(bool removed);
    bool isRemoved() const;

    void setIsModified(bool modified);
    bool isModified() const;

    QString generateTip() const;

    static const QChar kVariableDelimiter;
    static const QChar kEscapeChar;

    static SnippetParseResult parse(const QString &snippet);

private:
    bool m_isRemoved = false;
    bool m_isModified = false;
    QString m_groupId;
    QString m_id; // Only built-in snippets have an id.
    QString m_trigger;
    QString m_content;
    QString m_complement;
};

} // TextEditor
