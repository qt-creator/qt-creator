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

#include "coreplugin/externaltool.h"

#include <QWidget>
#include <QAbstractItemModel>
#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)

namespace Utils { class EnvironmentItem; }

namespace Core {
namespace Internal {

namespace Ui { class ExternalToolConfig; }

class ExternalToolModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ExternalToolModel(QObject *parent);
    ~ExternalToolModel();

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &modelIndex, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &modelIndex) const override;
    bool setData(const QModelIndex &modelIndex, const QVariant &value, int role = Qt::EditRole) override;

    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex &parent) override;
    QStringList mimeTypes() const override;

    void setTools(const QMap<QString, QList<ExternalTool *> > &tools);
    QMap<QString, QList<ExternalTool *> > tools() const;

    ExternalTool *toolForIndex(const QModelIndex &modelIndex) const;
    QString categoryForIndex(const QModelIndex &modelIndex, bool *found) const;
    void revertTool(const QModelIndex &modelIndex);
    QModelIndex addCategory();
    QModelIndex addTool(const QModelIndex &atIndex);
    void removeTool(const QModelIndex &modelIndex);
    Qt::DropActions supportedDropActions() const override;
private:
    QVariant data(ExternalTool *tool, int role = Qt::DisplayRole) const;
    QVariant data(const QString &category, int role = Qt::DisplayRole) const;

    QMap<QString, QList<ExternalTool *> > m_tools;
};

class ExternalToolConfig : public QWidget
{
    Q_OBJECT

public:
    explicit ExternalToolConfig(QWidget *parent = 0);
    ~ExternalToolConfig();

    void setTools(const QMap<QString, QList<ExternalTool *> > &tools);
    QMap<QString, QList<ExternalTool *> > tools() const;
    void apply();

private:
    void handleCurrentChanged(const QModelIndex &now, const QModelIndex &previous);
    void showInfoForItem(const QModelIndex &index);
    void updateItem(const QModelIndex &index);
    void revertCurrentItem();
    void updateButtons(const QModelIndex &index);
    void updateCurrentItem();
    void addTool();
    void removeTool();
    void addCategory();
    void updateEffectiveArguments();
    void editEnvironmentChanges();
    void updateEnvironmentLabel();

    Ui::ExternalToolConfig *ui;
    QList<Utils::EnvironmentItem> m_environment;
    ExternalToolModel *m_model;
};

} // Internal
} // Core
