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
#include <QtGui/QTreeWidgetItem>

namespace Core {
namespace Internal {

namespace Ui {
    class ExternalToolConfig;
}

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
    void handleCurrentItemChanged(QTreeWidgetItem *now, QTreeWidgetItem *previous);
    void showInfoForItem(QTreeWidgetItem *item);
    void updateItem(QTreeWidgetItem *item);
    void updateItemName(QTreeWidgetItem *item);

private:
    Ui::ExternalToolConfig *ui;
    QMap<QString, QList<ExternalTool *> > m_tools;
};

} // Internal
} // Core


#endif // EXTERNALTOOLCONFIG_H
