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

#include "import.h"

#include <qprocessuniqueptr.h>

#include <QSet>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QDir>
#include <QFileInfo>

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
                       const QHash<QString, int> &extToImportOptionsMap,
                       const QSet<QString> &preselectedFilesForOverwrite);

    bool isImporting() const;
    void cancelImport();
    bool isCancelled() const;

    void addError(const QString &errMsg, const QString &srcPath = {}) const;
    void addWarning(const QString &warningMsg, const QString &srcPath = {}) const;
    void addInfo(const QString &infoMsg, const QString &srcPath = {}) const;

signals:
    void errorReported(const QString &, const QString &) const;
    void warningReported(const QString &, const QString &) const;
    void infoReported(const QString &, const QString &) const;
    void progressChanged(int value, const QString &text) const;
    void importNearlyFinished() const;
    void importFinished();

private slots:
    void importProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void iconProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    struct ParseData {
        QJsonObject options;
        QDir targetDir;
        QDir outDir;
        QString targetDirPath;
        QFileInfo sourceInfo;
        QString assetName;
        QString originalAssetName;
        int importId;
    };

    void notifyFinished();
    void reset();
    void parseFiles(const QStringList &filePaths, const QVector<QJsonObject> &options,
                    const QHash<QString, int> &extToImportOptionsMap,
                    const QSet<QString> &preselectedFilesForOverwrite);
    bool preParseQuick3DAsset(const QString &file, ParseData &pd,
                              const QSet<QString> &preselectedFilesForOverwrite);
    void postParseQuick3DAsset(const ParseData &pd);
    void copyImportedFiles();

    void notifyProgress(int value, const QString &text);
    void notifyProgress(int value);
    void keepUiAlive() const;

    enum class OverwriteResult {
        Skip,
        Overwrite,
        Update
    };

    OverwriteResult confirmAssetOverwrite(const QString &assetName);
    bool startImportProcess(const ParseData &pd);
    bool startIconProcess(int size, const QString &iconFile, const QString &iconSource);
    void postImport();
    void finalizeQuick3DImport();
    QString sourceSceneTargetFilePath(const ParseData &pd);

    QSet<QHash<QString, QString>> m_importFiles;
    QHash<QString, QStringList> m_overwrittenImports;
    bool m_isImporting = false;
    bool m_cancelled = false;
    QString m_importPath;
    QTemporaryDir *m_tempDir = nullptr;
    std::vector<QProcessUniquePointer> m_qmlPuppetProcesses;
    int m_qmlPuppetCount = 0;
    int m_qmlImportFinishedCount = 0;
    int m_importIdCounter = 1000000; // Use ids in range unlikely to clash with any normal process exit codes
    QHash<int, ParseData> m_parseData;
    QString m_progressTitle;
    QList<Import> m_requiredImports;
};
} // QmlDesigner
