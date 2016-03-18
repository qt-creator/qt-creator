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

#include <coreplugin/core_global.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

namespace Core {

namespace Internal { class CommandMappingsPrivate; }

class CORE_EXPORT CommandMappings : public QWidget
{
    Q_OBJECT

public:
    CommandMappings(QWidget *parent = 0);
    ~CommandMappings();

signals:
    void currentCommandChanged(QTreeWidgetItem *current);

protected:
    virtual void defaultAction() = 0;

    virtual void exportAction() {}
    virtual void importAction() {}

    virtual bool filterColumn(const QString &filterString, QTreeWidgetItem *item, int column) const;

    void filterChanged(const QString &f);

    // access to m_page
    void setImportExportEnabled(bool enabled);
    QTreeWidget *commandList() const;
    QString filterText() const;
    void setFilterText(const QString &text);
    void setPageTitle(const QString &s);
    void setTargetHeader(const QString &s);
    void setModified(QTreeWidgetItem *item, bool modified);

private:
    bool filter(const QString &filterString, QTreeWidgetItem *item);

    friend class Internal::CommandMappingsPrivate;
    Internal::CommandMappingsPrivate *d;
};

} // namespace Core
