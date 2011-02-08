/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef EXTERNALTOOLCONFIG_H
#define EXTERNALTOOLCONFIG_H

#include "coreplugin/externaltool.h"

#include <QtGui/QWidget>
#include <QtCore/QAbstractItemModel>

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
    QString categoryForIndex(const QModelIndex &modelIndex) const;
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
    void add();
    void removeTool();
    void addCategory();

private:
    Ui::ExternalToolConfig *ui;
    ExternalToolModel *m_model;
};

} // Internal
} // Core


#endif // EXTERNALTOOLCONFIG_H
