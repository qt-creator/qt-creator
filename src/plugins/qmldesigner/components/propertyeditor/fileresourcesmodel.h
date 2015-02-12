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

#ifndef FILERESOURCESMODEL_H
#define FILERESOURCESMODEL_H

#include <qmlitemnode.h>

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QtQml>

class FileResourcesModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString fileName READ fileName WRITE setFileNameStr NOTIFY fileNameChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter)
    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(QUrl path READ path WRITE setPath)
    Q_PROPERTY(QStringList fileModel READ fileModel NOTIFY fileModelChanged)

public:
    explicit FileResourcesModel(QObject *parent = 0);

    void setModelNodeBackend(const QVariant &modelNodeBackend);
    QString fileName() const;
    void setFileName(const QUrl &fileName);
    void setFileNameStr(const QString &fileName);
    void setPath(const QUrl &url);
    QUrl path() const;
    void setFilter(const QString &filter);
    QString filter() const;
    QStringList fileModel() const;
    void setupModel();

    Q_INVOKABLE void openFileDialog();

    static void registerDeclarativeType();

signals:
    void fileNameChanged(const QUrl &fileName);
    void modelNodeBackendChanged();
    void fileModelChanged();

public slots:

private:
    QVariant modelNodeBackend() const;

private:
    QUrl m_fileName;
    QUrl m_path;
    QString m_filter;
    bool m_lock;
    QString m_currentPath;
    QString m_lastModelPath;
    QStringList m_model;

};

QML_DECLARE_TYPE(FileResourcesModel)

#endif // FILERESOURCESMODEL_H
