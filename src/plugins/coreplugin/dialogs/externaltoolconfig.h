/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef EXTERNALTOOLCONFIG_H
#define EXTERNALTOOLCONFIG_H

#include "coreplugin/externaltool.h"

#include <QWidget>
#include <QAbstractItemModel>

namespace Core {
namespace Internal {

namespace Ui {
    class ExternalToolConfig;
}

class ExternalToolModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ExternalToolModel(QObject *parent);
    ~ExternalToolModel();

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &modelIndex, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &modelIndex) const;
    bool setData(const QModelIndex &modelIndex, const QVariant &value, int role = Qt::EditRole);

    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex &parent);
    QStringList mimeTypes() const;

    void setTools(const QMap<QString, QList<ExternalTool *> > &tools);
    QMap<QString, QList<ExternalTool *> > tools() const;

    ExternalTool *toolForIndex(const QModelIndex &modelIndex) const;
    QString categoryForIndex(const QModelIndex &modelIndex, bool *found) const;
    void revertTool(const QModelIndex &modelIndex);
    QModelIndex addCategory();
    QModelIndex addTool(const QModelIndex &atIndex);
    void removeTool(const QModelIndex &modelIndex);
    Qt::DropActions supportedDropActions() const;
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

    QString searchKeywords() const;

private slots:
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

private:
    Ui::ExternalToolConfig *ui;
    ExternalToolModel *m_model;
};

} // Internal
} // Core


#endif // EXTERNALTOOLCONFIG_H
