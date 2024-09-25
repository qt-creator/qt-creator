// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <QAbstractListModel>
#include <QComboBox>
#include <QCoreApplication>
#include <QWidgetAction>

namespace QmlDesigner {

class CameraViewWidgetAction : public QWidgetAction
{
    Q_OBJECT

public:
    explicit CameraViewWidgetAction(QObject *parent = nullptr);
    QString currentMode() const;
    void setMode(const QString &mode);

protected:
    QWidget *createWidget(QWidget *parent) override;

private slots:
    void onWidgetHovered();

signals:
    void currentModeChanged(QString);
};

class CameraActionsModel : public QAbstractListModel
{
    Q_DECLARE_TR_FUNCTIONS(CameraActionsModel)

public:
    enum ExtendedDataRoles { ModeRole = Qt::UserRole + 1 };

    explicit CameraActionsModel(QObject *parent);
    virtual ~CameraActionsModel();

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    static int modeIndex(const QString &mode);

private:
    struct DataItem;
    static const QList<DataItem> m_data;
};

class ComboBoxAction : public QComboBox
{
    Q_OBJECT

public:
    explicit ComboBoxAction(QWidget *parent = nullptr);

protected:
    void enterEvent(QEnterEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

signals:
    void hovered();
};

} // namespace QmlDesigner
