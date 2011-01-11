/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef TEXTEDITORHELPITEM_H
#define TEXTEDITORHELPITEM_H

#include "texteditor_global.h"

#include <QtCore/QString>

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
