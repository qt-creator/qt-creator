/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
    
#ifndef OPENEDITORSVIEW_H
#define OPENEDITORSVIEW_H

#include "ui_openeditorsview.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/inavigationwidgetfactory.h>

#include <QtCore/QList>
#include <QtGui/QWidget>
#include <QtGui/QKeySequence>
#include <QtGui/QAbstractButton>
#include <QtGui/QTreeWidgetItem>

namespace Core {
namespace Internal {

class OpenEditorsWidget : public QWidget
{
    Q_OBJECT

public:
    OpenEditorsWidget();
    ~OpenEditorsWidget();

    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void registerEditor(Core::IEditor *editor);
    void unregisterEditors(QList<Core::IEditor *> editors);
    void updateEditorList();
    void selectEditor(QTreeWidgetItem *item = 0);
    void updateEditor();
    void closeEditors();
    void closeAllEditors();
    void updateCurrentItem(QTreeWidgetItem *currentItem = 0);
    void putFocusToEditorList();

private:
    static void updateItem(QTreeWidgetItem *item, Core::IEditor *editor);

    Ui::OpenEditorsView m_ui;
    QWidget *m_widget;
};

class OpenEditorsViewFactory : public Core::INavigationWidgetFactory
{
public:
    OpenEditorsViewFactory();
    virtual ~OpenEditorsViewFactory();
    QString displayName();
    virtual QKeySequence activationSequence();
    Core::NavigationView createWidget();
};

} // namespace Internal
} // namespace Core

#endif // OPENEDITORSVIEW_H
