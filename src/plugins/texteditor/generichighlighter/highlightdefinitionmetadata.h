/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef HIGHLIGHTDEFINITIONMETADATA_H
#define HIGHLIGHTDEFINITIONMETADATA_H

#include <QtCore/QString>
#include <QtCore/QLatin1String>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

namespace TextEditor {
namespace Internal {

class HighlightDefinitionMetaData
{
public:
    HighlightDefinitionMetaData();

    void setName(const QString &name);
    const QString &name() const;

    void setVersion(const QString &version);
    const QString &version() const;

    void setPatterns(const QStringList &patterns);
    const QStringList &patterns() const;

    void setMimeTypes(const QStringList &mimeTypes);
    const QStringList &mimeTypes() const;

    void setUrl(const QUrl &url);
    const QUrl &url() const;

    static const QLatin1String kName;
    static const QLatin1String kExtensions;
    static const QLatin1String kMimeType;
    static const QLatin1String kVersion;
    static const QLatin1String kUrl;

private:
    QString m_name;
    QString m_version;
    QStringList m_patterns;
    QStringList m_mimeTypes;
    QUrl m_url;
};

} // namespace Internal
} // namespace TextEditor

#endif // HIGHLIGHTDEFINITIONMETADATA_H
