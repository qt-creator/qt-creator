/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SNIPPET_H
#define SNIPPET_H

#include <texteditor/texteditor_global.h>

#include <QChar>
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
