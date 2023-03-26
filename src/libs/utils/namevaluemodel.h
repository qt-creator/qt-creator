// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "environmentfwd.h"

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
    bool currentEntryIsPathList(const QModelIndex &current) const;

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
