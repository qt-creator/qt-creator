// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"

#include <import.h>

#include <QObject>
#include <QString>
#include <QUrl>
#include <QFileSystemWatcher>
#include <QMultiHash>
#include <QPointer>
#include <QFileInfo>
#include <QDir>

namespace QmlDesigner {

class Model;

class QMLDESIGNERCORE_EXPORT SubComponentManager : public QObject
{
    Q_OBJECT
public:
    explicit SubComponentManager(Model *model,
                                 class ExternalDependenciesInterface &externalDependencies);

    void update(const QUrl &fileUrl, const Imports &imports);
    void addAndParseImport(const Import &import);

    QStringList qmlFiles() const;
    QStringList directories() const;

private: // functions
    void parseDirectory(const QString &canonicalDirPath,  bool addToLibrary = true, const TypeName &qualification = TypeName());
    void parseFile(const QString &canonicalFilePath,  bool addToLibrary, const QString&);
    void parseFile(const QString &canonicalFilePath);

    bool addImport(const Import &import, int index = -1);
    void removeImport(int index);
    void parseDirectories();
    QFileInfoList watchedFiles(const QString &canonicalDirPath);
    void unregisterQmlFile(const QFileInfo &fileInfo, const QString &qualifier);
    void registerQmlFile(const QFileInfo &fileInfo, const QString &qualifier, bool addToLibrary);
    Model *model() const;
    QStringList importPaths() const;
    void parseQuick3DAssetsDir(const QString &quick3DAssetsPath);
    void parseQuick3DAssetsItem(const QString &importUrl, const QString &quick3DAssetsPath = {});
    QStringList quick3DAssetPaths() const;
    TypeName resolveDirQualifier(const QString &dirPath) const;

private: // variables
    QFileSystemWatcher m_watcher;
    Imports m_imports;
    // key: canonical directory path
    QMultiHash<QString,QString> m_dirToQualifier;
    QUrl m_filePath;
    QDir m_filePathDir;
    QPointer<Model> m_model;
    ExternalDependenciesInterface &m_externalDependencies;
};

} // namespace QmlDesigner
