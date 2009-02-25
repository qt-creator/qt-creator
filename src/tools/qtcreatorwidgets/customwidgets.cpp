/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "customwidgets.h"

#include <QtGui/QMenu>
#include <QtGui/QAction>

static const char *groupC = "QtCreator";

NewClassCustomWidget::NewClassCustomWidget(QObject *parent) :
   QObject(parent),
   CustomWidget<Core::Utils::NewClassWidget>
       (QLatin1String("<utils/newclasswidget.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        tr("Widget to enter classes and source files"))
{
}

ClassNameValidatingLineEdit_CW::ClassNameValidatingLineEdit_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Core::Utils::ClassNameValidatingLineEdit>
       (QLatin1String("<utils/classnamevalidatinglineedit.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        tr("Line Edit that validates a class name"))
{
}

FileNameValidatingLineEdit_CW::FileNameValidatingLineEdit_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Core::Utils::FileNameValidatingLineEdit>
       (QLatin1String("<utils/filenamevalidatinglineedit.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        tr("Line Edit that validates a file name"))
{
}

ProjectNameValidatingLineEdit_CW::ProjectNameValidatingLineEdit_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Core::Utils::ProjectNameValidatingLineEdit>
       (QLatin1String("<utils/projectnamevalidatinglineedit.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        tr("Line Edit that validates a project name"))
{
}

LineColumnLabel_CW::LineColumnLabel_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Core::Utils::LineColumnLabel>
       (QLatin1String("<utils/linecolumnlabel.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        tr("Label suited for displaying line numbers with a fixed with depending on the font size"),
        QSize(100, 20))
{
}

PathChooser_CW::PathChooser_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Core::Utils::PathChooser>
       (QLatin1String("<utils/pathchooser.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        tr("Input widget for paths with a browse button"))
{
}

FancyLineEdit_CW::FancyLineEdit_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Core::Utils::FancyLineEdit>
       (QLatin1String("<utils/fancylineedit.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        tr("A Line edit with a clickable menu pixmap"))
{
}

QtColorButton_CW::QtColorButton_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Core::Utils::QtColorButton>
       (QLatin1String("<utils/qtcolorbutton.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        tr("A color button that spawns a QColorDialog on click"))
{
}

QWidget *FancyLineEdit_CW::createWidget(QWidget *parent)
{
    Core::Utils::FancyLineEdit *fle = new Core::Utils::FancyLineEdit(parent);
    QMenu *menu = new QMenu(fle);
    menu->addAction("Test");
    fle->setMenu(menu);
    return fle;
}

SubmitEditorWidget_CW::SubmitEditorWidget_CW(QObject *parent) :
    QObject(parent),
    CustomWidget<Core::Utils::SubmitEditorWidget>
    (QLatin1String("<utils/submiteditorwidget.h>"),
    false,
    QLatin1String(groupC),
    QIcon(),
    tr("Submit editor showing message and file list"))
{
}

// -------------- WidgetCollection
WidgetCollection::WidgetCollection(QObject *parent) :
    QObject(parent)
{

    m_plugins.push_back(new NewClassCustomWidget(this));
    m_plugins.push_back(new ClassNameValidatingLineEdit_CW(this));
    m_plugins.push_back(new FileNameValidatingLineEdit_CW(this));
    m_plugins.push_back(new ProjectNameValidatingLineEdit_CW(this));
    m_plugins.push_back(new LineColumnLabel_CW(this));
    m_plugins.push_back(new PathChooser_CW(this));
    m_plugins.push_back(new FancyLineEdit_CW(this));
    m_plugins.push_back(new QtColorButton_CW(this));
    m_plugins.push_back(new SubmitEditorWidget_CW(this));
}

QList<QDesignerCustomWidgetInterface*> WidgetCollection::customWidgets() const
{
    return m_plugins;
}

Q_EXPORT_PLUGIN(WidgetCollection)
