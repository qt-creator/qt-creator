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

#ifndef SUBCOMPONENTMANAGER_H
#define SUBCOMPONENTMANAGER_H

#include "qmldesignercorelib_global.h"

#include <import.h>

#include <QObject>
#include <QString>
#include <QUrl>
#include <QFileSystemWatcher>
#include <QPointer>
#include <QFileInfo>

namespace QmlDesigner {

class Model;

class QMLDESIGNERCORE_EXPORT SubComponentManager : public QObject
{
    Q_OBJECT
public:
    explicit SubComponentManager(Model *model, QObject *parent = 0);

    void update(const QUrl &fileUrl, const QList<Import> &imports);

    QStringList qmlFiles() const;
    QStringList directories() const;

private slots:
    void parseDirectory(const QString &canonicalDirPath,  bool addToLibrary = true, const TypeName &qualification = TypeName());
    void parseFile(const QString &canonicalFilePath,  bool addToLibrary, const QString&);
    void parseFile(const QString &canonicalFilePath);

private: // functions
    void addImport(int pos, const Import &import);
    void removeImport(int pos);
    void parseDirectories();
    QList<QFileInfo> watchedFiles(const QString &canonicalDirPath);
    void unregisterQmlFile(const QFileInfo &fileInfo, const QString &qualifier);
    void registerQmlFile(const QFileInfo &fileInfo, const QString &qualifier, bool addToLibrary);
    Model *model() const;
    QStringList importPaths() const;

private: // variables
    QFileSystemWatcher m_watcher;
    QList<Import> m_imports;
    // key: canonical directory path
    QMultiHash<QString,QString> m_dirToQualifier;
    QUrl m_filePath;
    QPointer<Model> m_model;
};

} // namespace QmlDesigner


#endif // SUBCOMPONENTMANAGER_H
