// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace Core {

namespace Internal { class CommandMappingsPrivate; }

class CORE_EXPORT CommandMappings : public QWidget
{
    Q_OBJECT

public:
    CommandMappings(QWidget *parent = nullptr);
    ~CommandMappings() override;

signals:
    void currentCommandChanged(QTreeWidgetItem *current);
    void resetRequested();

protected:
    virtual void defaultAction() = 0;

    virtual void exportAction() {}
    virtual void importAction() {}

    virtual bool filterColumn(const QString &filterString, QTreeWidgetItem *item, int column) const;

    void filterChanged(const QString &f);

    void setImportExportEnabled(bool enabled);
    void setResetVisible(bool visible);

    QTreeWidget *commandList() const;
    QString filterText() const;
    void setFilterText(const QString &text);
    void setPageTitle(const QString &s);
    void setTargetHeader(const QString &s);
    static void setModified(QTreeWidgetItem *item, bool modified);

private:
    bool filter(const QString &filterString, QTreeWidgetItem *item);

    friend class Internal::CommandMappingsPrivate;
    Internal::CommandMappingsPrivate *d;
};

} // namespace Core
