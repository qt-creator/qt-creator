// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <QDateTime>
#include <QObject>
#include <QTimer>

#include <utils/filepath.h>

namespace Utils { class Unarchiver; }

namespace QmlDesigner {

class FileExtractor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString targetPath READ targetPath WRITE setTargetPath NOTIFY targetPathChanged)
    Q_PROPERTY(QString archiveName READ archiveName WRITE setArchiveName)
    Q_PROPERTY(QString detailedText READ detailedText NOTIFY detailedTextChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(QString size READ size NOTIFY sizeChanged)
    Q_PROPERTY(QString count READ count NOTIFY sizeChanged)
    Q_PROPERTY(QString sourceFile READ sourceFile WRITE setSourceFile)
    Q_PROPERTY(bool finished READ finished NOTIFY finishedChanged)
    Q_PROPERTY(bool targetFolderExists READ targetFolderExists NOTIFY targetFolderExistsChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QDateTime birthTime READ birthTime NOTIFY birthTimeChanged)
    Q_PROPERTY(bool clearTargetPathContents READ clearTargetPathContents WRITE setClearTargetPathContents NOTIFY clearTargetPathContentsChanged)
    Q_PROPERTY(bool alwaysCreateDir READ alwaysCreateDir WRITE setAlwaysCreateDir NOTIFY alwaysCreateDirChanged)

public:
    explicit FileExtractor(QObject *parent = nullptr);
    ~FileExtractor();

    Q_INVOKABLE void changeTargetPath(const QString &path);

    QString targetPath() const;
    void setTargetPath(const QString &path);
    void setSourceFile(const QString &sourceFilePath);
    void setArchiveName(const QString &filePath);
    const QString detailedText() const;
    bool finished() const;
    QString currentFile() const;
    QString size() const;
    QString count() const;
    bool targetFolderExists() const;
    int progress() const;
    QDateTime birthTime() const;
    void setClearTargetPathContents(bool value);
    bool clearTargetPathContents() const;
    void setAlwaysCreateDir(bool value);
    bool alwaysCreateDir() const;

    QString sourceFile() const;
    QString archiveName() const;

    Q_INVOKABLE void browse();
    Q_INVOKABLE void extract();

signals:
    void targetPathChanged();
    void detailedTextChanged();
    void finishedChanged();
    void currentFileChanged();
    void sizeChanged();
    void targetFolderExistsChanged();
    void progressChanged();
    void birthTimeChanged();
    void clearTargetPathContentsChanged();
    void alwaysCreateDirChanged();

private:
    Utils::FilePath m_targetPath;
    QString m_targetFolder; // The same as m_targetPath, but with the archive name also.
    Utils::FilePath m_sourceFile;
    QString m_detailedText;
    bool m_finished = false;
    QTimer m_timer;
    QString m_currentFile;
    QString m_size;
    QString m_count;
    QString m_archiveName;
    int m_progress = 0;
    QDateTime m_birthTime;
    bool m_clearTargetPathContents = false;
    bool m_alwaysCreateDir = false;

    qint64 m_bytesBefore = 0;
    qint64 m_compressedSize = 0;
    std::unique_ptr<Utils::Unarchiver> m_unarchiver;
};

} // QmlDesigner
