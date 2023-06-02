// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QAbstractItemModel>
#include <QStringList>

namespace Debugger::Internal {

class DebuggerEngine;

class SourceFilesHandler : public QAbstractItemModel
{
public:
    explicit SourceFilesHandler(DebuggerEngine *engine);

    int columnCount(const QModelIndex &parent) const override
        { return parent.isValid() ? 0 : 2; }
    int rowCount(const QModelIndex &parent) const override
        { return parent.isValid() ? 0 : m_shortNames.size(); }
    QModelIndex parent(const QModelIndex &) const override { return QModelIndex(); }
    QModelIndex index(int row, int column, const QModelIndex &) const override
        { return createIndex(row, column); }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &idx, const QVariant &data, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void clearModel();

    void setSourceFiles(const QMap<QString, Utils::FilePath> &sourceFiles);
    void removeAll();

    QAbstractItemModel *model() { return m_proxyModel; }

private:
    DebuggerEngine *m_engine;
    QStringList m_shortNames;
    Utils::FilePaths m_fullNames;
    QAbstractItemModel *m_proxyModel;
};

} // Debugger::Internal
