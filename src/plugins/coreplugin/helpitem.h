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

#include "core_global.h"

#include <utils/optional.h>

#include <QString>
#include <QUrl>
#include <QVariant>

#include <vector>

namespace Core {

class CORE_EXPORT HelpItem
{
public:
    using Link = std::pair<QString, QUrl>;
    using Links = std::vector<Link>;

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
    HelpItem(const char *helpId);
    HelpItem(const QString &helpId);
    HelpItem(const QString &helpId, const QString &docMark, Category category);
    HelpItem(const QStringList &helpIds, const QString &docMark, Category category);
    explicit HelpItem(const QUrl &url);
    HelpItem(const QUrl &url, const QString &docMark, Category category);

    void setHelpUrl(const QUrl &url);
    const QUrl &helpUrl() const;

    void setHelpIds(const QStringList &ids);
    const QStringList &helpIds() const;

    void setDocMark(const QString &mark);
    const QString &docMark() const;

    void setCategory(Category cat);
    Category category() const;

    bool isEmpty() const;
    bool isValid() const;

    QString extractContent(bool extended) const;

    const Links &links() const;
    const Links bestLinks() const;
    const QString keyword() const;
    bool isFuzzyMatch() const;

private:
    QUrl m_helpUrl;
    QStringList m_helpIds;
    QString m_docMark;
    Category m_category = Unknown;
    mutable Utils::optional<Links> m_helpLinks; // cached help links
    mutable QString m_keyword;
    mutable bool m_isFuzzyMatch = false;
};

} // namespace Core

Q_DECLARE_METATYPE(Core::HelpItem)
