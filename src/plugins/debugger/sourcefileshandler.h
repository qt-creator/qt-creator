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

#include <QAbstractItemModel>
#include <QStringList>

namespace Debugger {
namespace Internal {

class DebuggerEngine;

class SourceFilesHandler : public QAbstractItemModel
{
    Q_OBJECT

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

    void setSourceFiles(const QMap<QString, QString> &sourceFiles);
    void removeAll();

    QAbstractItemModel *model() { return m_proxyModel; }

private:
    DebuggerEngine *m_engine;
    QStringList m_shortNames;
    QStringList m_fullNames;
    QAbstractItemModel *m_proxyModel;
};

} // namespace Internal
} // namespace Debugger
