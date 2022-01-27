/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QAbstractListModel>
#include <QStandardItemModel>
#include <QRegularExpression>

namespace StudioWelcome {

class ScreenSizeModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ScreenSizeModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {}

    Q_INVOKABLE QSize screenSizes(int index) const
    {
        constexpr auto invalid = QSize{0, 0};
        if (!m_backendModel)
            return invalid;

        auto *item = m_backendModel->item(index, 0);
        // Matches strings like "1024 x 768" or "1080 x 1920 (FullHD)"
        QRegularExpression re{R"__(^(\d+)\s*x\s*(\d+).*)__"};

        if (!item)
            return invalid;

        auto m = re.match(item->text());
        if (!m.hasMatch())
            return invalid;

        bool ok = false;
        int width = m.captured(1).toInt(&ok);
        if (!ok)
            return invalid;

        int height = m.captured(2).toInt(&ok);
        if (!ok)
            return invalid;

        return QSize{width, height};
    }

    int rowCount(const QModelIndex &/*parent*/) const override
    {
        if (m_backendModel)
            return m_backendModel->rowCount();

        return 0;
    }

    QVariant data(const QModelIndex &index, int /*role*/) const override
    {
        if (m_backendModel) {
            auto *item = m_backendModel->item(index.row(), index.column());
            return item->text();
        }

        return {};
    }

    int appendItem(const QString &text)
    {
        m_backendModel->appendRow(new QStandardItem{text});
        return rowCount(QModelIndex{}) - 1;
    }

    QHash<int, QByteArray> roleNames() const override
    {
        if (m_backendModel)
            return m_backendModel->roleNames();

        QHash<int, QByteArray> roleNames;
        roleNames[Qt::UserRole] = "name";
        return roleNames;
    }

    void reset() {
        beginResetModel();
        endResetModel();
    }

    void setBackendModel(QStandardItemModel *model)
    {
        m_backendModel = model;
    }

private:
    QStandardItemModel *m_backendModel = nullptr;
};

} // namespace StudioWelcome
