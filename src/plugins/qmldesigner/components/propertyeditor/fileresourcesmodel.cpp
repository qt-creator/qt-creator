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

#include "fileresourcesmodel.h"

#include <coreplugin/icore.h>

#include <model.h>

#include <utils/algorithm.h>
#include <documentmanager.h>

#include <QFileDialog>
#include <QDirIterator>
#include <qmlmodelnodeproxy.h>

static QString s_lastBrowserPath;

FileResourcesModel::FileResourcesModel(QObject *parent)
    : QObject(parent)
    , m_filter(QLatin1String("(*.*)"))
    , m_fileSystemWatcher(new Utils::FileSystemWatcher(this))
{
    connect(m_fileSystemWatcher, &Utils::FileSystemWatcher::directoryChanged,
            this, &FileResourcesModel::refreshModel);
}

void FileResourcesModel::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    auto modelNodeBackendObject = modelNodeBackend.value<QObject *>();

    const auto backendObjectCasted =
            qobject_cast<const QmlDesigner::QmlModelNodeProxy *>(modelNodeBackendObject);

    if (backendObjectCasted) {
        QmlDesigner::Model *model = backendObjectCasted->qmlObjectNode().modelNode().model();
        m_docPath = QDir{QFileInfo{model->fileUrl().toLocalFile()}.absolutePath()};
        m_path = QUrl::fromLocalFile(QmlDesigner::DocumentManager::currentResourcePath()
                                     .toFileInfo().absoluteFilePath());
    }

    setupModel();
    emit modelNodeBackendChanged();
}

QString FileResourcesModel::fileName() const
{
    return m_fileName.toString();
}

void FileResourcesModel::setFileNameStr(const QString &fileName)
{
    setFileName(QUrl(fileName));
}

void FileResourcesModel::setFileName(const QUrl &fileName)
{
    if (fileName == m_fileName)
        return;

    m_fileName = fileName;

    emit fileNameChanged(fileName);
}

void FileResourcesModel::setPath(const QUrl &url)
{
    m_path = url;
    setupModel();
}

QUrl FileResourcesModel::path() const
{
    return m_path;
}

QUrl FileResourcesModel::docPath() const
{
    return QUrl::fromLocalFile(m_docPath.path());
}

void FileResourcesModel::setFilter(const QString &filter)
{
    if (m_filter != filter) {
        m_filter = filter;
        setupModel();
    }
}

QString FileResourcesModel::filter() const
{
    return m_filter;
}

QStringList FileResourcesModel::fullPathModel() const
{
    return m_fullPathModel;
}

QStringList FileResourcesModel::fileNameModel() const
{
    return m_fileNameModel;
}

void FileResourcesModel::openFileDialog()
{
    QString resourcePath = m_path.toLocalFile();
    bool resourcePathChanged = m_lastResourcePath != resourcePath;
    m_lastResourcePath = resourcePath;

    // First we try the last path this browser widget was opened with within current project
    QString path = resourcePathChanged ? QString() : m_currentPath;

    // If that one is not valid we try the path for the current file
    if (path.isEmpty() && !m_fileName.isEmpty())
        path = QFileInfo(m_fileName.toString()).absolutePath();

    // Next we try to fall back to the path any file browser was opened with
    if (!QFileInfo::exists(path))
        path = s_lastBrowserPath;

    // The last fallback is to try the resource path
    if (!QFileInfo::exists(path))
        path = resourcePath;

    QString newFile = QFileDialog::getOpenFileName(Core::ICore::dialogParent(),
                                                   tr("Open File"),
                                                   path,
                                                   m_filter);

    if (!newFile.isEmpty()) {
        setFileNameStr(newFile);
        m_currentPath = QFileInfo(newFile).absolutePath();
        s_lastBrowserPath = m_currentPath;
    }
}

void FileResourcesModel::registerDeclarativeType()
{
    qmlRegisterType<FileResourcesModel>("HelperWidgets", 2, 0, "FileResourcesModel");
}

QVariant FileResourcesModel::modelNodeBackend() const
{
    return QVariant();
}

bool filterMetaIcons(const QString &fileName)
{
    QFileInfo info(fileName);

    if (info.dir().path().split('/').contains("designer")) {
        QDir currentDir = info.dir();

        int i = 0;
        while (!currentDir.isRoot() && i < 3) {
            if (currentDir.dirName() == "designer") {
                if (!currentDir.entryList({"*.metainfo"}).isEmpty())
                    return false;
            }

            currentDir.cdUp();
            ++i;
        }

        if (info.dir().dirName() == "designer")
            return false;
    }

    return true;
}

void FileResourcesModel::setupModel()
{
    m_dirPath = QDir(m_path.toLocalFile());

    refreshModel();

    m_fileSystemWatcher->removeDirectories(m_fileSystemWatcher->directories());
    m_fileSystemWatcher->addDirectory(m_dirPath.absolutePath(),
                                      Utils::FileSystemWatcher::WatchAllChanges);
}

void FileResourcesModel::refreshModel()
{
    m_fullPathModel.clear();
    m_fileNameModel.clear();

    QStringList filterList = m_filter.split(QLatin1Char(' '));

    QDirIterator it(m_dirPath.absolutePath(), filterList, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString absolutePath = it.next();
        if (filterMetaIcons(absolutePath)) {
            QString filePath = m_docPath.relativeFilePath(absolutePath);
            m_fullPathModel.append(filePath);
        }
    }

    Utils::sort(m_fullPathModel, [](const QString &s1, const QString &s2) {
        return s1.mid(s1.lastIndexOf('/') + 1).toLower() < s2.mid(s2.lastIndexOf('/') + 1).toLower();
    });

    for (const QString &fullPath : qAsConst(m_fullPathModel))
        m_fileNameModel.append(fullPath.mid(fullPath.lastIndexOf('/') + 1));

    emit fullPathModelChanged();
    emit fileNameModelChanged();
}
