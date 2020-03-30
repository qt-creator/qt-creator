/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QSet>
#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qhash.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qprocess.h>

#include "import.h"

QT_BEGIN_NAMESPACE
class QSSGAssetImportManager;
class QTemporaryDir;
QT_END_NAMESPACE

namespace QmlDesigner {

class ItemLibraryAssetImporter : public QObject
{
    Q_OBJECT

public:
    ItemLibraryAssetImporter(QObject *parent = nullptr);
    ~ItemLibraryAssetImporter();

    void importQuick3D(const QStringList &inputFiles, const QString &importPath,
                       const QVector<QJsonObject> &options,
                       const QHash<QString, int> &extToImportOptionsMap);

    bool isImporting() const;
    void cancelImport();
    bool isCancelled() const;

    void addError(const QString &errMsg, const QString &srcPath = {}) const;
    void addWarning(const QString &warningMsg, const QString &srcPath = {}) const;
    void addInfo(const QString &infoMsg, const QString &srcPath = {}) const;

    bool isQuick3DAsset(const QString &fileName) const;
    QVariantMap supportedOptions(const QString &modelFile) const;
    QHash<QString, QVariantMap> allOptions() const;
    QHash<QString, QStringList> supportedExtensions() const;

signals:
    void errorReported(const QString &, const QString &) const;
    void warningReported(const QString &, const QString &) const;
    void infoReported(const QString &, const QString &) const;
    void progressChanged(int value, const QString &text) const;
    void importNearlyFinished() const;
    void importFinished();

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void notifyFinished();
    void reset();
    void parseFiles(const QStringList &filePaths, const QVector<QJsonObject> &options,
                    const QHash<QString, int> &extToImportOptionsMap);
    void parseQuick3DAsset(const QString &file, const QVariantMap &options);
    void copyImportedFiles();

    void notifyProgress(int value, const QString &text) const;
    void keepUiAlive() const;
    bool confirmAssetOverwrite(const QString &assetName);
    bool generateComponentIcon(int size, const QString &iconFile, const QString &iconSource);
    void finalizeQuick3DImport();

#ifdef IMPORT_QUICK3D_ASSETS
    QScopedPointer<QSSGAssetImportManager> m_quick3DAssetImporter;
    QSet<QHash<QString, QString>> m_importFiles;
    QSet<QString> m_overwrittenImports;
#endif
    bool m_isImporting = false;
    bool m_cancelled = false;
    QString m_importPath;
    QTemporaryDir *m_tempDir = nullptr;
    QSet<QProcess *> m_qmlPuppetProcesses;
    int m_qmlPuppetCount = 0;
};
} // QmlDesigner
