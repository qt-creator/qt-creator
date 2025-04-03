// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

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
class QTemporaryDir;
QT_END_NAMESPACE

namespace QmlDesigner {

class Import3dImporter : public QObject
{
    Q_OBJECT

public:
    Import3dImporter(QObject *parent = nullptr);
    ~Import3dImporter();

    void importQuick3D(const QStringList &inputFiles, const QString &importPath,
                       const QVector<QJsonObject> &options,
                       const QHash<QString, int> &extToImportOptionsMap,
                       const QSet<QString> &preselectedFilesForOverwrite);

    void reImportQuick3D(const QHash<QString, QJsonObject> &importOptions);

    bool isImporting() const;
    void cancelImport();
    bool isCancelled() const;

    void addError(const QString &errMsg, const QString &srcPath = {});
    void addWarning(const QString &warningMsg, const QString &srcPath = {});
    void addInfo(const QString &infoMsg, const QString &srcPath = {});

    QString previewFileName() const { return "QDSImport3dPreviewScene.qml"; }
    QString tempDirNameBase() const { return "/qds3dimport"; }

    void finalizeQuick3DImport();
    void removeAssetFromImport(const QString &assetName);

    struct PreviewData
    {
        int optionsIndex = 0;
        QJsonObject renderedOptions;
        QJsonObject currentOptions;
        QString name;
        QString folderName;
        QString qmlName;
        QString type;
        qint64 size;
    };

signals:
    void errorReported(const QString &, const QString &);
    void warningReported(const QString &, const QString &);
    void infoReported(const QString &, const QString &);
    void progressChanged(int value, const QString &text);
    void importReadyForPreview(const QString &path, const QList<PreviewData> &previewData);
    void importNearlyFinished();
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
        QString importedQmlName;
        qint64 assetSize;
        int importId = -1;
        int optionsIndex = -1;
        QHash<QString, QStringList> overwrittenImports;
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
    QString generateAssetFolderName(const QString &assetName) const;
    QString generateRequiredImportForAsset(const QString &assetName) const;

    enum class OverwriteResult {
        Skip,
        Overwrite,
        Update
    };

    OverwriteResult confirmAssetOverwrite(const QString &assetName);
    void startNextImportProcess();
    void postImport();
    QString sourceSceneTargetFilePath(const ParseData &pd);

    QHash<QString, QHash<QString, QString>> m_importFiles; // Key: asset name
    bool m_isImporting = false;
    bool m_cancelled = false;
    QString m_importPath;
    QTemporaryDir *m_tempDir = nullptr;
    QProcessUniquePointer m_puppetProcess;
    int m_importIdCounter = 0;
    int m_currentImportId = 0;
    QHash<int, QString> m_importIdToAssetNameMap;
    QHash<QString, ParseData> m_parseData; // Key: asset name
    QString m_progressTitle;
    QList<int> m_puppetQueue;
};
} // QmlDesigner
