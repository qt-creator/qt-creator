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

#ifndef COMMANDMAPPINGS_H
#define COMMANDMAPPINGS_H

#include <coreplugin/dialogs/ioptionspage.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace Core {

namespace Internal {  namespace Ui { class CommandMappings; }  }

class CORE_EXPORT CommandMappings : public Core::IOptionsPage
{
    Q_OBJECT

public:
    CommandMappings(QObject *parent = 0);

protected slots:
    void commandChanged(QTreeWidgetItem *current);
    void filterChanged(const QString &f);
    virtual void importAction() {}
    virtual void exportAction() {}
    virtual void defaultAction() = 0;

protected:
    // IOptionsPage
    QWidget *createPage(QWidget *parent);
    virtual void apply() {}
    virtual void finish();

    virtual void initialize() = 0;
    bool filter(const QString &filterString, QTreeWidgetItem *item);

    // access to m_page
    void setImportExportEnabled(bool enabled);
    QTreeWidget *commandList() const;
    QLineEdit *targetEdit() const;
    QString filterText() const;
    void setPageTitle(const QString &s);
    void setTargetLabelText(const QString &s);
    void setTargetEditTitle(const QString &s);
    void setTargetHeader(const QString &s);
    void setModified(QTreeWidgetItem *item, bool modified);
    virtual void markPossibleCollisions(QTreeWidgetItem *) {}
    virtual void resetCollisionMarkers() {}
private:
    Internal::Ui::CommandMappings *m_page;
};

} // namespace Core

#endif // COMMANDMAPPINGS_H
