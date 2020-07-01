/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#include "filepathmodel.h"

#include "exportnotification.h"

#include "projectexplorer/project.h"
#include "projectexplorer/projectnodes.h"
#include "utils/runextensions.h"

#include <QLoggingCategory>
#include <QTimer>

using namespace ProjectExplorer;

namespace  {
Q_LOGGING_CATEGORY(loggerError, "qtc.designer.assetExportPlugin.filePathModel", QtCriticalMsg)
Q_LOGGING_CATEGORY(loggerInfo, "qtc.designer.assetExportPlugin.filePathModel", QtInfoMsg)

void findQmlFiles(QFutureInterface<Utils::FilePath> &f, const Project *project)
{
    if (!project || f.isCanceled())
        return;

    int index = 0;
    Utils::FilePaths qmlFiles = project->files([&f, &index](const Node* node) ->bool {
        if (f.isCanceled())
            return false;
        Utils::FilePath path = node->filePath();
        bool isComponent = !path.fileName().isEmpty() && path.fileName().front().isUpper();
        if (isComponent && node->filePath().endsWith(".ui.qml"))
            f.reportResult(path, index++);
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
        ExportNotification::addInfo(tr("Canceling QML files preparation."));
        m_preprocessWatcher->cancel();
        m_preprocessWatcher->waitForFinished();
        qCDebug(loggerInfo) << "Canceling QML files preparation done.";
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
        return m_files.count();
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
    connect(m_preprocessWatcher.get(), &QFutureWatcher<Utils::FilePath>::resultReadyAt, this,
            [this](int resultIndex) {
        beginInsertRows(index(0, 0) , m_files.count(), m_files.count());
        m_files.append(m_preprocessWatcher->resultAt(resultIndex));
        endInsertRows();
    });

    connect(m_preprocessWatcher.get(), &QFutureWatcher<Utils::FilePath>::finished,
            this, &FilePathModel::endResetModel);

    QFuture<Utils::FilePath> f = Utils::runAsync(&findQmlFiles, m_project);
    m_preprocessWatcher->setFuture(f);
}


}
