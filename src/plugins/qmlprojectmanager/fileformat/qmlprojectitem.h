/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <utils/environment.h>

#include <QObject>
#include <QSet>
#include <QStringList>

namespace QmlProjectManager {

class QmlProjectContentItem : public QObject {
    // base class for all elements that should be direct children of Project element
    Q_OBJECT

public:
    QmlProjectContentItem(QObject *parent = nullptr) : QObject(parent) {}
};

class QmlProjectItem : public QObject
{
    Q_OBJECT

public:
    QString sourceDirectory() const { return m_sourceDirectory; }
    void setSourceDirectory(const QString &directoryPath);
    QString targetDirectory() const { return m_targetDirectory; }
    void setTargetDirectory(const QString &directoryPath);

    bool qtForMCUs() const { return m_qtForMCUs; }
    void setQtForMCUs(bool qtForMCUs);

    QStringList importPaths() const { return m_importPaths; }
    void setImportPaths(const QStringList &paths);

    QStringList fileSelectors() const { return m_fileSelectors; }
    void setFileSelectors(const QStringList &selectors);

    QStringList files() const;
    bool matchesFile(const QString &filePath) const;

    bool forceFreeType() const { return m_forceFreeType; };
    void setForceFreeType(bool);

    QString mainFile() const { return m_mainFile; }
    void setMainFile(const QString &mainFilePath) { m_mainFile = mainFilePath; }

    void appendContent(QmlProjectContentItem *item) { m_content.append(item); }

    Utils::EnvironmentItems environment() const;
    void addToEnviroment(const QString &key, const QString &value);

signals:
    void qmlFilesChanged(const QSet<QString> &, const QSet<QString> &);

protected:
    QString m_sourceDirectory;
    QString m_targetDirectory;
    QStringList m_importPaths;
    QStringList m_fileSelectors;
    QString m_mainFile;
    Utils::EnvironmentItems m_environment;
    QVector<QmlProjectContentItem *> m_content; // content property
    bool m_forceFreeType = false;
    bool m_qtForMCUs = false;
};

} // namespace QmlProjectManager
