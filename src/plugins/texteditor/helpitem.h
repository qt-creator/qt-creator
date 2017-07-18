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

#include "texteditor_global.h"

#include <QMap>
#include <QString>
#include <QUrl>

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
    HelpItem(const QString &helpId, const QString &docMark, Category category,
             const QMap<QString, QUrl> &helpLinks);
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
    QMap<QString, QUrl> retrieveHelpLinks() const;

private:
    QString m_helpId;
    QString m_docMark;
    Category m_category = Unknown;
    mutable QMap<QString, QUrl> m_helpLinks; // cached help links
};

} // namespace TextEditor
