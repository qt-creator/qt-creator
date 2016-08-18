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

#pragma once

#include <texteditor/texteditor_global.h>

#include <QChar>
#include <QList>
#include <QString>

namespace Core { class Id; }

namespace TextEditor {

class TEXTEDITOR_EXPORT NameMangler
{
public:
    virtual ~NameMangler();

    virtual Core::Id id() const = 0;
    virtual QString mangle(const QString &unmangled) const = 0;
};

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

    class ParsedSnippet {
    public:
        QString text;
        QString errorMessage;
        bool success;
        struct Range {
            Range(int s, int l, NameMangler *m) : start(s), length(l), mangler(m) { }
            int start;
            int length;
            NameMangler *mangler;
        };
        QList<Range> ranges;
    };

    static ParsedSnippet parse(const QString &snippet);

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
