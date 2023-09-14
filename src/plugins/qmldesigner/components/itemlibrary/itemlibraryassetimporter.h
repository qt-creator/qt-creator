// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
    void postParseQuick3DAsset(ParseData &pd);
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
    void startNextImportProcess();
    void postImport();
    void finalizeQuick3DImport();
    QString sourceSceneTargetFilePath(const ParseData &pd);

    QSet<QHash<QString, QString>> m_importFiles;
    QHash<QString, QStringList> m_overwrittenImports;
    bool m_isImporting = false;
    bool m_cancelled = false;
    QString m_importPath;
    QTemporaryDir *m_tempDir = nullptr;
    QProcessUniquePointer m_puppetProcess;
    int m_importIdCounter = 0;
    int m_currentImportId = 0;
    QHash<int, ParseData> m_parseData;
    QString m_progressTitle;
    QStringList m_requiredImports;
    QList<int> m_puppetQueue;
};
} // QmlDesigner
