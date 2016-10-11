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

#include <utils/icon.h>
#include <utils/itemviews.h>

#include <QDialog>
#include <QIcon>
#include <QStyledItemDelegate>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QComboBox;
class QStandardItemModel;
class QStandardItem;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

// A widget that lists SHA1 and subject of the changes
// Used for reset and interactive rebase

class LogChangeWidget : public Utils::TreeView
{
    Q_OBJECT

public:
    enum LogFlag
    {
        None = 0x00,
        IncludeRemotes = 0x01,
        Silent = 0x02
    };

    Q_DECLARE_FLAGS(LogFlags, LogFlag)

    explicit LogChangeWidget(QWidget *parent = 0);
    bool init(const QString &repository, const QString &commit = QString(), LogFlags flags = None);
    QString commit() const;
    int commitIndex() const;
    QString earliestCommit() const;
    void setItemDelegate(QAbstractItemDelegate *delegate);

signals:
    void commitActivated(const QString &commit);

private:
    void emitCommitActivated(const QModelIndex &index);

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    bool populateLog(const QString &repository, const QString &commit, LogFlags flags);
    const QStandardItem *currentItem(int column = 0) const;

    QStandardItemModel *m_model;
    bool m_hasCustomDelegate;
};

class LogChangeDialog : public QDialog
{
    Q_OBJECT

public:
    LogChangeDialog(bool isReset, QWidget *parent);

    bool runDialog(const QString &repository, const QString &commit = QString(),
                   LogChangeWidget::LogFlags flags = LogChangeWidget::None);

    QString commit() const;
    int commitIndex() const;
    QString resetFlag() const;
    LogChangeWidget *widget() const;

private:
    LogChangeWidget *m_widget;
    QDialogButtonBox *m_dialogButtonBox;
    QComboBox *m_resetTypeComboBox;
};

class LogItemDelegate : public QStyledItemDelegate
{
protected:
    LogItemDelegate(LogChangeWidget *widget);

    int currentRow() const;

private:
    LogChangeWidget *m_widget;
};

class IconItemDelegate : public LogItemDelegate
{
public:
    IconItemDelegate(LogChangeWidget *widget, const Utils::Icon &icon);

    virtual bool hasIcon(int row) const = 0;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

private:
    QIcon m_icon;
};

} // namespace Internal
} // namespace Git
