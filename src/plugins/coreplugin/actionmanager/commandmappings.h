/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef COMMANDMAPPINGS_H
#define COMMANDMAPPINGS_H

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtCore/QObject>
#include <QtGui/QTreeWidgetItem>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QTreeWidget;
class Ui_CommandMappings;
QT_END_NAMESPACE

namespace Core {

class Command;

namespace Internal {

class ActionManagerPrivate;
class MainWindow;

}

class CORE_EXPORT CommandMappings : public Core::IOptionsPage
{
    Q_OBJECT

public:
    CommandMappings(QObject *parent = 0);
    ~CommandMappings();

    // IOptionsPage
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual QString category() const = 0;
    virtual QString displayCategory() const = 0;

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
    bool filter(const QString &f, const QTreeWidgetItem *item);

    // access to m_page
    void setImportExportEnabled(bool enabled);
    QTreeWidget *commandList() const;
    QLineEdit *targetEdit() const;
    void setPageTitle(const QString &s);
    void setTargetLabelText(const QString &s);
    void setTargetEditTitle(const QString &s);
    void setTargetHeader(const QString &s);

private:
    Ui_CommandMappings *m_page;
};

} // namespace Core

#endif // COMMANDMAPPINGS_H
