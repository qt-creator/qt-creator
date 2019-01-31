/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QInputDialog>
#include <QListView>
#include <QSettings>
#include <QStackedWidget>
#include <QStyledItemDelegate>

QT_FORWARD_DECLARE_CLASS(QString)

namespace QmlDesigner {

class EasingCurve;
class NamedEasingCurve;

class PresetItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    PresetItemDelegate();

    void paint(QPainter *painter,
               const QStyleOptionViewItem &opt,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
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
    QStringList allNames() const;

    QList<NamedEasingCurve> storedCurves() const;

    QString createUniqueName() const;

    void removeSelectedItem();

private:
    QSettings::Scope m_scope;

    int m_index;

    QString m_filename;
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
