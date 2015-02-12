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
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SNIPPET_H
#define SNIPPET_H

#include <texteditor/texteditor_global.h>

#include <QChar>
#include <QList>
#include <QString>

namespace Core { class Id; }

namespace TextEditor {

class TEXTEDITOR_EXPORT NameMangler
{
public:
    virtual ~NameMangler() { }

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

    class ParsedSnippet {
    public:
        QString text;
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
    bool m_isRemoved;
    bool m_isModified;
    QString m_groupId;
    QString m_id; // Only built-in snippets have an id.
    QString m_trigger;
    QString m_content;
    QString m_complement;
};

} // TextEditor

#endif // SNIPPET_H
