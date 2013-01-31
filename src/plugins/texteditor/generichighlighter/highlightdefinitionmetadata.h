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

#ifndef HIGHLIGHTDEFINITIONMETADATA_H
#define HIGHLIGHTDEFINITIONMETADATA_H

#include <QString>
#include <QLatin1String>
#include <QStringList>
#include <QUrl>

namespace TextEditor {
namespace Internal {

class HighlightDefinitionMetaData
{
public:
    HighlightDefinitionMetaData();

    void setPriority(const int priority);
    int priority() const;

    void setId(const QString &id);
    const QString &id() const;

    void setName(const QString &name);
    const QString &name() const;

    void setVersion(const QString &version);
    const QString &version() const;

    void setFileName(const QString &fileName);
    const QString &fileName() const;

    void setPatterns(const QStringList &patterns);
    const QStringList &patterns() const;

    void setMimeTypes(const QStringList &mimeTypes);
    const QStringList &mimeTypes() const;

    void setUrl(const QUrl &url);
    const QUrl &url() const;

    static const QLatin1String kPriority;
    static const QLatin1String kName;
    static const QLatin1String kExtensions;
    static const QLatin1String kMimeType;
    static const QLatin1String kVersion;
    static const QLatin1String kUrl;

private:
    int m_priority;
    QString m_id;
    QString m_name;
    QString m_version;
    QString m_fileName;
    QStringList m_patterns;
    QStringList m_mimeTypes;
    QUrl m_url;
};

} // namespace Internal
} // namespace TextEditor

#endif // HIGHLIGHTDEFINITIONMETADATA_H
