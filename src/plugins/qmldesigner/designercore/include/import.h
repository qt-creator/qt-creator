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

#ifndef IMPORT_H
#define IMPORT_H

#include <QString>
#include <QStringList>
#include <QMetaType>

#include "qmldesignercorelib_global.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT Import
{
public:
    Import();

    static Import createLibraryImport(const QString &url, const QString &version = QString(), const QString &alias = QString(), const QStringList &importPaths = QStringList());
    static Import createFileImport(const QString &file, const QString &version = QString(), const QString &alias = QString(), const QStringList &importPaths = QStringList());
    static Import empty();

    bool isEmpty() const { return m_url.isEmpty() && m_file.isEmpty(); }
    bool isFileImport() const { return m_url.isEmpty() && !m_file.isEmpty(); }
    bool isLibraryImport() const { return !m_url.isEmpty() && m_file.isEmpty(); }
    bool hasVersion() const { return !m_version.isEmpty(); }
    bool hasAlias() const { return !m_alias.isEmpty(); }

    QString url() const { return m_url; }
    QString file() const { return m_file; }
    QString version() const { return m_version; }
    QString alias() const { return m_alias; }
    QStringList importPaths() const { return m_importPathList; }

    QString toString(bool skipAlias = false) const;
    QString toImportString() const;

    bool operator==(const Import &other) const;
    bool isSameModule(const Import &other) const;

private:
    Import(const QString &url, const QString &file, const QString &version, const QString &alias, const QStringList &importPaths);

private:
    QString m_url;
    QString m_file;
    QString m_version;
    QString m_alias;
    QStringList m_importPathList;
};

QMLDESIGNERCORE_EXPORT uint qHash(const Import &import);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::Import)

#endif // IMPORT_H
