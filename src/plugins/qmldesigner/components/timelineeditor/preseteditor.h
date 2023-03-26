// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QInputDialog>
#include <QListView>
#include <QSettings>
#include <QStackedWidget>
#include <QStyledItemDelegate>

QT_FORWARD_DECLARE_CLASS(QString)

namespace QmlDesigner {

class EasingCurve;
class NamedEasingCurve;

QString makeNameUnique(const QString& name, const QStringList& currentNames);

class PresetItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    PresetItemDelegate(const QColor& background);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &opt,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QColor m_background;
};

class PresetList : public QListView
{
    Q_OBJECT

public:
    enum ItemRoles {
        ItemRole_Undefined = Qt::UserRole,
        ItemRole_Data,
        ItemRole_Dirty,
        ItemRole_Modifiable
    };

signals:
    void presetChanged(const EasingCurve &curve);

public:
    PresetList(QSettings::Scope scope, QWidget *parent = nullptr);

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

    bool hasSelection() const;

    bool dirty(const QModelIndex &index) const;

    int index() const;

    bool isEditable(const QModelIndex &index) const;

    QColor backgroundColor() const;

    QColor curveColor() const;

    QStringList allNames() const;

    void initialize(int index);

    void readPresets();

    void writePresets();

    void revert(const QModelIndex &index);

    void updateCurve(const EasingCurve &curve);

    void createItem();

    void createItem(const QString &name, const EasingCurve &curve);

    void setItemData(const QModelIndex &index, const QVariant &curve, const QVariant &icon);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

    void dataChanged(const QModelIndex &topLeft,
                     const QModelIndex &bottomRight,
                     const QVector<int> &roles = QVector<int>()) override;

private:
    QList<NamedEasingCurve> storedCurves() const;

    void removeSelectedItem();

private:
    QSettings::Scope m_scope;

    int m_index;

    QString m_filename;

    QColor m_background;

    QColor m_curveColor;
};

class PresetEditor : public QStackedWidget
{
    Q_OBJECT

signals:
    void presetChanged(const EasingCurve &curve);

public:
    explicit PresetEditor(QWidget *parent = nullptr);

    void initialize(QTabBar *bar);

    void activate(int id);

    void update(const EasingCurve &curve);

    void readPresets();

    bool writePresets(const EasingCurve &curve);

private:
    bool isCurrent(PresetList *list);

private:
    PresetList *m_presets;

    PresetList *m_customs;
};

} // namespace QmlDesigner
