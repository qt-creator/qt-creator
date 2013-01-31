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

#ifndef TEXTEDITORHELPITEM_H
#define TEXTEDITORHELPITEM_H

#include "texteditor_global.h"

#include <QString>

namespace TextEditor {

class TEXTEDITOR_EXPORT HelpItem
{
public:
    enum Category {
        ClassOrNamespace,
        Enum,
        Typedef,
        Macro,
        Brief,
        Function,
        QmlComponent,
        QmlProperty,
        QMakeVariableOfFunction,
        Unknown
    };

    HelpItem();
    HelpItem(const QString &helpId, Category category);
    HelpItem(const QString &helpId, const QString &docMark, Category category);
    ~HelpItem();

    void setHelpId(const QString &id);
    const QString &helpId() const;

    void setDocMark(const QString &mark);
    const QString &docMark() const;

    void setCategory(Category cat);
    Category category() const;

    bool isValid() const;

    QString extractContent(bool extended) const;

private:
    QString m_helpId;
    QString m_docMark;
    Category m_category;
};

} // namespace TextEditor

#endif // TEXTEDITORHELPITEM_H
