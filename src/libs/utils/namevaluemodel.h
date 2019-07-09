/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "environmentfwd.h"
#include "utils_global.h"

#include <QAbstractTableModel>

#include <memory>

namespace Utils {

namespace Internal {
class NameValueModelPrivate;
}

class QTCREATOR_UTILS_EXPORT NameValueModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit NameValueModel(QObject *parent = nullptr);
    ~NameValueModel() override;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex addVariable();
    QModelIndex addVariable(const NameValueItem &item);
    void resetVariable(const QString &name);
    void unsetVariable(const QString &name);
    void toggleVariable(const QModelIndex &index);
    bool isUnset(const QString &name);
    bool isEnabled(const QString &name) const;
    bool canReset(const QString &name);
    QString indexToVariable(const QModelIndex &index) const;
    QModelIndex variableToIndex(const QString &name) const;
    bool changes(const QString &key) const;
    const NameValueDictionary &baseNameValueDictionary() const;
    void setBaseNameValueDictionary(const NameValueDictionary &dictionary);
    NameValueItems userChanges() const;
    void setUserChanges(const NameValueItems &items);

signals:
    void userChangesChanged();
    /// Hint to the view where it should make sense to focus on next
    // This is a hack since there is no way for a model to suggest
    // the next interesting place to focus on to the view.
    void focusIndex(const QModelIndex &index);

private:
    std::unique_ptr<Internal::NameValueModelPrivate> d;
};

} // namespace Utils
