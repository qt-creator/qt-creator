// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
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

namespace Git::Internal {

class LogChangeModel;

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

    explicit LogChangeWidget(QWidget *parent = nullptr);
    bool init(const Utils::FilePath &repository, const QString &commit = {}, LogFlags flags = None);
    QString commit() const;
    int commitIndex() const;
    QString earliestCommit() const;
    void setItemDelegate(QAbstractItemDelegate *delegate);
    void setExcludedRemote(const QString &remote) { m_excludedRemote = remote; }

signals:
    void commitActivated(const QString &commit);

private:
    void emitCommitActivated(const QModelIndex &index);

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    bool populateLog(const Utils::FilePath &repository, const QString &commit, LogFlags flags);
    const QStandardItem *currentItem(int column = 0) const;

    LogChangeModel *m_model;
    bool m_hasCustomDelegate;
    QString m_excludedRemote;
};

class LogChangeDialog : public QDialog
{
public:
    LogChangeDialog(bool isReset, QWidget *parent);

    bool runDialog(const Utils::FilePath &repository, const QString &commit = QString(),
                   LogChangeWidget::LogFlags flags = LogChangeWidget::None);

    QString commit() const;
    int commitIndex() const;
    QString resetFlag() const;
    LogChangeWidget *widget() const;

private:
    LogChangeWidget *m_widget = nullptr;
    QDialogButtonBox *m_dialogButtonBox = nullptr;
    QComboBox *m_resetTypeComboBox = nullptr;
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

} // Git::Internal
