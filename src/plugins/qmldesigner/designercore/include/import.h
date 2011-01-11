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

#ifndef IMPORT_H
#define IMPORT_H

#include <QtCore/QString>
#include <QStringList>

#include "corelib_global.h"

namespace QmlDesigner {

class CORESHARED_EXPORT Import
{
public:
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

    QString toString(bool addSemicolon = false) const;

    bool operator==(const Import &other) const;

private:
    Import(const QString &url, const QString &file, const QString &version, const QString &alias, const QStringList &importPaths);

private:
    QString m_url;
    QString m_file;
    QString m_version;
    QString m_alias;
    QStringList m_importPathList;
};

CORESHARED_EXPORT uint qHash(const Import &import);

} // namespace QmlDesigner

#endif // IMPORT_H
