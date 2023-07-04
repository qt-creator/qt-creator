// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "filepathmodel.h"

#include "exportnotification.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <utils/async.h>

#include <QLoggingCategory>
#include <QTimer>

using namespace ProjectExplorer;

namespace  {
Q_LOGGING_CATEGORY(loggerError, "qtc.designer.assetExportPlugin.filePathModel", QtCriticalMsg)
Q_LOGGING_CATEGORY(loggerInfo, "qtc.designer.assetExportPlugin.filePathModel", QtInfoMsg)

void findQmlFiles(QPromise<Utils::FilePath> &promise, const Project *project)
{
    if (!project || promise.isCanceled())
        return;

    int index = 0;
    project->files([&promise, &index](const Node* node) ->bool {
        if (promise.isCanceled())
            return false;
        Utils::FilePath path = node->filePath();
        bool isComponent = !path.fileName().isEmpty() && path.fileName().front().isUpper();
        if (isComponent && node->filePath().endsWith(".ui.qml"))
            promise.addResult(path, index++);
        return true;
    });
}
}

namespace QmlDesigner {

FilePathModel::FilePathModel(ProjectExplorer::Project *project, QObject *parent)
    : QAbstractListModel(parent),
      m_project(project)
{
    QTimer::singleShot(0, this, &FilePathModel::processProject);
}

FilePathModel::~FilePathModel()
{
    if (m_preprocessWatcher && !m_preprocessWatcher->isCanceled() &&
            !m_preprocessWatcher->isFinished()) {
        ExportNotification::addInfo(tr("Canceling file preparation."));
        m_preprocessWatcher->cancel();
        m_preprocessWatcher->waitForFinished();
        qCDebug(loggerInfo) << "Canceled file preparation.";
    }
}

Qt::ItemFlags FilePathModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags itemFlags = QAbstractListModel::flags(index);
    if (index.isValid())
        itemFlags |= Qt::ItemIsUserCheckable;
    return itemFlags;
}

int FilePathModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_files.size();
    return 0;
}

QVariant FilePathModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    case Qt::DisplayRole:
        return m_files[index.row()].toUserOutput();
    case Qt::CheckStateRole:
        return m_skipped.count(m_files[index.row()]) ? Qt::Unchecked : Qt::Checked;
    default:
        break;
    }

    return {};
}

bool FilePathModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::CheckStateRole)
        return false;

    const Utils::FilePath path = m_files[index.row()];
    if (value == Qt::Checked)
        m_skipped.erase(path);
    else
        m_skipped.insert(path);

    emit dataChanged(index, index);
    return true;
}

Utils::FilePaths FilePathModel::files() const
{
    Utils::FilePaths selectedPaths;
    std::copy_if(m_files.begin(), m_files.end(), std::back_inserter(selectedPaths),
                 [this](const Utils::FilePath &path) {
        return !m_skipped.count(path);
    });
    return selectedPaths;
}

void FilePathModel::processProject()
{
    if (m_preprocessWatcher && !m_preprocessWatcher->isCanceled() &&
            !m_preprocessWatcher->isFinished()) {
        qCDebug(loggerError) << "Previous model load not finished.";
        return;
    }

    beginResetModel();
    m_preprocessWatcher.reset(new QFutureWatcher<Utils::FilePath>(this));
    connect(m_preprocessWatcher.get(),
            &QFutureWatcher<Utils::FilePath>::resultReadyAt,
            this,
            [this](int resultIndex) {
                beginInsertRows(index(0, 0), m_files.size(), m_files.size());
                m_files.append(m_preprocessWatcher->resultAt(resultIndex));
                endInsertRows();
            });

    connect(m_preprocessWatcher.get(), &QFutureWatcher<Utils::FilePath>::finished,
            this, &FilePathModel::endResetModel);

    QFuture<Utils::FilePath> f = Utils::asyncRun(&findQmlFiles, m_project);
    m_preprocessWatcher->setFuture(f);
}


}
