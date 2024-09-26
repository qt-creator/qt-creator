// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <QAbstractListModel>
#include <QCoreApplication>
#include <QWidgetAction>

namespace QmlDesigner {

class ComboBoxActionsModel : public QAbstractListModel
{
    Q_DECLARE_TR_FUNCTIONS(ComboBoxActionsModel)

public:
    struct DataItem
    {
        QString name;
        QString tooltip;
        QString mode;
    };

    enum ExtendedDataRoles { ModeRole = Qt::UserRole + 1 };

    explicit ComboBoxActionsModel(const QList<ComboBoxActionsModel::DataItem> &dataItems, QObject *parent);
    virtual ~ComboBoxActionsModel();

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int modeIndex(const QString &mode);
    QString defaultMode() const;

private:
    const QList<DataItem> m_data;
};

class ComboBoxAction : public QWidgetAction
{
    Q_OBJECT

public:
    explicit ComboBoxAction(const QList<ComboBoxActionsModel::DataItem> &dataItems, QObject *parent = nullptr);
    QString currentMode() const;
    void setMode(const QString &mode);
    void cycleMode();

protected:
    QWidget *createWidget(QWidget *parent) override;

signals:
    void currentModeChanged(QString);

private:
    ComboBoxActionsModel *m_model = {};
};

} // namespace QmlDesigner
