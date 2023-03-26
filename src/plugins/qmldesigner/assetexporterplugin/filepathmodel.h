// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once
#include <QAbstractListModel>
#include <QFutureWatcher>

#include "utils/fileutils.h"

#include <memory>
#include <unordered_set>

namespace ProjectExplorer {
class Project;
}

namespace QmlDesigner {
class FilePathModel : public QAbstractListModel
{
    Q_DECLARE_TR_FUNCTIONS(QmlDesigner::FilePathModel)

public:
    FilePathModel(ProjectExplorer::Project *project, QObject *parent = nullptr);
    ~FilePathModel() override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    Utils::FilePaths files() const;
private:
    void processProject();

    ProjectExplorer::Project *m_project = nullptr;
    std::unique_ptr<QFutureWatcher<Utils::FilePath>> m_preprocessWatcher;
    std::unordered_set<Utils::FilePath> m_skipped;
    Utils::FilePaths m_files;
};

}
