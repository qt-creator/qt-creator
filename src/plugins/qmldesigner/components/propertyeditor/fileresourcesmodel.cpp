// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileresourcesmodel.h"

#include <coreplugin/icore.h>

#include <model.h>

#include <documentmanager.h>
#include <utils/algorithm.h>

#include <QDirIterator>
#include <QFileDialog>
#include <qmlmodelnodeproxy.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

static QString s_lastBrowserPath;

FileResourcesModel::FileResourcesModel(QObject *parent)
    : QObject(parent)
    , m_filter(QLatin1String("(*.*)"))
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::projectForFile(
        QmlDesigner::DocumentManager::currentFilePath());

    if (project) {
        connect(project,
                &ProjectExplorer::Project::fileListChanged,
                this,
                &FileResourcesModel::refreshModel);
    }
}

void FileResourcesModel::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    auto modelNodeBackendObject = modelNodeBackend.value<QObject *>();

    const auto backendObjectCasted = qobject_cast<const QmlDesigner::QmlModelNodeProxy *>(
        modelNodeBackendObject);

    if (backendObjectCasted) {
        QmlDesigner::Model *model = backendObjectCasted->qmlObjectNode().modelNode().model();
        if (!model)
            return;

        m_docPath = QDir{QFileInfo{model->fileUrl().toLocalFile()}.absolutePath()};
        m_path = QUrl::fromLocalFile(
            QmlDesigner::DocumentManager::currentProjectDirPath().toFileInfo().absoluteFilePath());
    }

    refreshModel();
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
    refreshModel();

    emit pathChanged(url);
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
    if (m_filter == filter)
        return;

    m_filter = filter;
    refreshModel();

    emit filterChanged(filter);
}

QString FileResourcesModel::filter() const
{
    return m_filter;
}

QList<FileResourcesItem> FileResourcesModel::model() const
{
    return m_model;
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

QString FileResourcesModel::resolve(const QString &relative) const
{
    if (relative.startsWith('#'))
        return relative;

    if (QDir::isAbsolutePath(relative))
        return relative;

    if (!QUrl::fromUserInput(relative, m_docPath.path()).isLocalFile())
        return relative;

    return QFileInfo(m_docPath, relative).absoluteFilePath();
}

bool FileResourcesModel::isLocal(const QString &path) const
{
    return QUrl::fromUserInput(path, m_docPath.path()).isLocalFile();
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

void FileResourcesModel::refreshModel()
{
    m_model.clear();

    if (m_path.isValid()) {
        const QDir dirPath = QDir(m_path.toLocalFile());
        const QStringList filterList = m_filter.split(QLatin1Char(' '));

        QDirIterator it(dirPath.absolutePath(), filterList, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString absolutePath = it.next();
            if (filterMetaIcons(absolutePath)) {
                const QString relativeFilePath = m_docPath.relativeFilePath(absolutePath);
                m_model.append(
                    FileResourcesItem(absolutePath,
                                      relativeFilePath,
                                      relativeFilePath.mid(relativeFilePath.lastIndexOf('/') + 1)));
            }
        }

        Utils::sort(m_model, [](const FileResourcesItem &i1, const FileResourcesItem &i2) {
            return i1.fileName().toLower() < i2.fileName().toLower();
        });
    }

    emit modelChanged();
}
