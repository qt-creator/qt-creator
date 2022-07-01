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

#include <qmlitemnode.h>

#include <QDir>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QtQml>

class FileResourcesItem
{
    Q_GADGET

    Q_PROPERTY(QString absoluteFilePath READ absoluteFilePath CONSTANT)
    Q_PROPERTY(QString relativeFilePath READ relativeFilePath CONSTANT)
    Q_PROPERTY(QString fileName READ fileName CONSTANT)

public:
    FileResourcesItem(const QString &absoluteFilePath,
                      const QString &relativeFilePath,
                      const QString &fileName)
        : m_absoluteFilePath(absoluteFilePath)
        , m_relativeFilePath(relativeFilePath)
        , m_fileName(fileName)
    {}

    FileResourcesItem() = default;
    FileResourcesItem(const FileResourcesItem &other) = default;
    FileResourcesItem &operator=(const FileResourcesItem &other) = default;

    const QString &absoluteFilePath() const { return m_absoluteFilePath; }
    const QString &relativeFilePath() const { return m_relativeFilePath; }
    const QString &fileName() const { return m_fileName; }

private:
    QString m_absoluteFilePath;
    QString m_relativeFilePath;
    QString m_fileName;
};

class FileResourcesModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString fileName READ fileName WRITE setFileNameStr NOTIFY fileNameChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(QUrl path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QUrl docPath READ docPath CONSTANT)
    Q_PROPERTY(QList<FileResourcesItem> model READ model NOTIFY modelChanged)

public:
    explicit FileResourcesModel(QObject *parent = nullptr);

    void setModelNodeBackend(const QVariant &modelNodeBackend);
    QString fileName() const;
    void setFileName(const QUrl &fileName);
    void setFileNameStr(const QString &fileName);
    void setPath(const QUrl &url);
    QUrl path() const;
    QUrl docPath() const;
    void setFilter(const QString &filter);
    QString filter() const;
    QList<FileResourcesItem> model() const;

    void setupModel();
    void refreshModel();

    Q_INVOKABLE void openFileDialog();
    Q_INVOKABLE QString resolve(const QString &relative) const;
    Q_INVOKABLE bool isLocal(const QString &path) const;

    static void registerDeclarativeType();

signals:
    void fileNameChanged(const QUrl &fileName);
    void filterChanged(const QString &filte);
    void modelNodeBackendChanged();
    void pathChanged(const QUrl &path);
    void modelChanged();

private:
    QVariant modelNodeBackend() const;

private:
    QUrl m_fileName;
    QUrl m_path;
    QDir m_dirPath;
    QDir m_docPath;
    QString m_filter;
    QString m_currentPath;
    QString m_lastResourcePath;
    QList<FileResourcesItem> m_model;
};

QML_DECLARE_TYPE(FileResourcesModel)
