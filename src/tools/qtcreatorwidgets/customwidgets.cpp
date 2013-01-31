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

#include "customwidgets.h"

#include <QMenu>
#include <QAction>
#include <QExtensionManager>
#include <QDesignerFormEditorInterface>

static const char groupC[] = "QtCreator";

NewClassCustomWidget::NewClassCustomWidget(QObject *parent) :
   QObject(parent),
   CustomWidget<Utils::NewClassWidget>
       (QLatin1String("<utils/newclasswidget.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        QLatin1String("Widget to enter classes and source files"))
{
}

ClassNameValidatingLineEdit_CW::ClassNameValidatingLineEdit_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Utils::ClassNameValidatingLineEdit>
       (QLatin1String("<utils/classnamevalidatinglineedit.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        QLatin1String("Line Edit that validates a class name"))
{
}

FileNameValidatingLineEdit_CW::FileNameValidatingLineEdit_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Utils::FileNameValidatingLineEdit>
       (QLatin1String("<utils/filenamevalidatinglineedit.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        QLatin1String("Line Edit that validates a file name"))
{
}

ProjectNameValidatingLineEdit_CW::ProjectNameValidatingLineEdit_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Utils::ProjectNameValidatingLineEdit>
       (QLatin1String("<utils/projectnamevalidatinglineedit.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        QLatin1String("Line Edit that validates a project name"))
{
}

LineColumnLabel_CW::LineColumnLabel_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Utils::LineColumnLabel>
       (QLatin1String("<utils/linecolumnlabel.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        QLatin1String("Label suited for displaying line numbers with a fixed with depending on the font size"),
        QSize(100, 20))
{
}

PathChooser_CW::PathChooser_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Utils::PathChooser>
       (QLatin1String("<utils/pathchooser.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        QLatin1String("Input widget for paths with a browse button"))
{
}

IconButton_CW::IconButton_CW(QObject *parent) :
    QObject(parent),
    CustomWidget<Utils::IconButton>
        (QLatin1String("<utils/fancylineedit.h>"),
         false,
         QLatin1String(groupC),
         QIcon(),
         QLatin1String("Icon button of FancyLineEdit"))
{
}

FancyLineEdit_CW::FancyLineEdit_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Utils::FancyLineEdit>
       (QLatin1String("<utils/fancylineedit.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        QLatin1String("A Line edit with a clickable menu pixmap"))
{
}

FilterLineEdit_CW::FilterLineEdit_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Utils::FilterLineEdit>
       (QLatin1String("<utils/filterlineedit.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        QLatin1String("A Line edit customized for filtering"))
{
}


QtColorButton_CW::QtColorButton_CW(QObject *parent) :
   QObject(parent),
   CustomWidget<Utils::QtColorButton>
       (QLatin1String("<utils/qtcolorbutton.h>"),
        false,
        QLatin1String(groupC),
        QIcon(),
        QLatin1String("A color button that spawns a QColorDialog on click"))
{
}

QWidget *FancyLineEdit_CW::createWidget(QWidget *parent)
{
    Utils::FancyLineEdit *fle = new Utils::FancyLineEdit(parent);
    fle->setButtonVisible(Utils::FancyLineEdit::Left, true);
    fle->setButtonPixmap(Utils::FancyLineEdit::Left,
            fle->style()->standardIcon(QStyle::SP_ArrowRight).pixmap(16));
    QMenu *menu = new QMenu(fle);
    menu->addAction(QLatin1String("Example"));
    fle->setButtonMenu(Utils::FancyLineEdit::Left, menu);
    return fle;
}

SubmitEditorWidget_CW::SubmitEditorWidget_CW(QObject *parent) :
    QObject(parent),
    CustomWidget<Utils::SubmitEditorWidget>
    (QLatin1String("<utils/submiteditorwidget.h>"),
    false,
    QLatin1String(groupC),
    QIcon(),
    QLatin1String("Submit editor showing message and file list"))
{
}

SubmitFieldWidget_CW::SubmitFieldWidget_CW(QObject *parent) :
    QObject(parent),
    CustomWidget<Utils::SubmitFieldWidget>
    (QLatin1String("<utils/submitfieldwidget.h>"),
    false,
    QLatin1String(groupC),
    QIcon(),
    QLatin1String("Show predefined fields of a submit message in a control based on mail address controls"))
{
}

PathListEditor_CW::PathListEditor_CW(QObject *parent) :
    QObject(parent),
    CustomWidget<Utils::PathListEditor>
    (QLatin1String("<utils/pathlisteditor.h>"),
    false,
    QLatin1String(groupC),
    QIcon(),
    QLatin1String("Edit a path list variable"))
{
}

DetailsButton_CW::DetailsButton_CW(QObject *parent) :
    QObject(parent),
    CustomWidget<Utils::DetailsButton>
    (QLatin1String("<utils/detailsbutton.h>"),
    false,
    QLatin1String(groupC),
    QIcon(),
    QLatin1String("Expandable button for 'Details'"))
{
}

StyledBar_CW::StyledBar_CW(QObject *parent) :
    QObject(parent),
    CustomWidget<Utils::StyledBar>
    (QLatin1String("<utils/styledbar.h>"),
    false,
    QLatin1String(groupC),
    QIcon(),
    QLatin1String("Styled bar"))
{
}

StyledSeparator_CW::StyledSeparator_CW(QObject *parent) :
    QObject(parent),
    CustomWidget<Utils::StyledSeparator>
    (QLatin1String("<utils/styledbar.h>"),
    false,
    QLatin1String(groupC),
    QIcon(),
    QLatin1String("Styled separator"))
{
}

Wizard_CW::Wizard_CW(QObject *parent) :
    QObject(parent),
    CustomWidget<Utils::Wizard>
    (QLatin1String("<utils/wizard.h>"),
    true,
    QLatin1String(groupC),
    QIcon(),
    QLatin1String("Wizard with progress indicator"))
{
}

CrumblePath_CW::CrumblePath_CW(QObject *parent) :
    QObject(parent),
    CustomWidget<Utils::CrumblePath>
    (QLatin1String("<utils/crumblepath.h>"),
    false,
    QLatin1String(groupC),
    QIcon(),
    QLatin1String("Crumble Path"))
{
}

DetailsWidget_CW::DetailsWidget_CW(QObject *parent) :
    QObject(parent),
    CustomWidget<Utils::DetailsWidget>
    (QLatin1String("<utils/detailswidget.h>"),
    true,
    QLatin1String(groupC),
    QIcon(),
    QLatin1String("Expandable widget for 'Details'. You might need an expandable spacer below."))
{
}

QString DetailsWidget_CW::domXml() const
{
    // Expanded from start, else child visibility is wrong
    const char *xmlC ="\
<ui language=\"c++\" displayname=\"DetailsWidget\">\
    <widget class=\"Utils::DetailsWidget\" name=\"detailsWidget\">\
        <property name=\"geometry\">\
            <rect><x>0</x><y>0</y><width>160</width><height>80</height></rect>\
        </property>\
        <property name=\"summaryText\">\
            <string>Summary</string>\
        </property>\
        <property name=\"expanded\">\
            <bool>true</bool>\
        </property>\
        <widget class=\"QWidget\" name=\"detailsContainer\" />\
    </widget>\
    <customwidgets>\
        <customwidget>\
            <class>Utils::DetailsWidget</class>\
            <addpagemethod>setWidget</addpagemethod>\
        </customwidget>\
    </customwidgets>\
</ui>";
    return QLatin1String(xmlC);
}

void DetailsWidget_CW::initialize(QDesignerFormEditorInterface *core)
{
    const bool firstTime = !initialized();
    CustomWidget<Utils::DetailsWidget>::initialize(core);
    if (firstTime)
        if (QExtensionManager *manager = core->extensionManager())
            manager->registerExtensions(new DetailsWidgetExtensionFactory(manager), Q_TYPEID(QDesignerContainerExtension));
}

DetailsWidgetContainerExtension::DetailsWidgetContainerExtension(Utils::DetailsWidget *widget, QObject *parent) :
    QObject(parent),
    m_detailsWidget(widget)
{
}

void DetailsWidgetContainerExtension::addWidget(QWidget *widget)
{
    if (m_detailsWidget->widget())
        qWarning("Cannot add 2nd child to DetailsWidget");
    else
        m_detailsWidget->setWidget(widget);
}

int DetailsWidgetContainerExtension::count() const
{
    return m_detailsWidget->widget() ? 1 : 0;
}

int DetailsWidgetContainerExtension::currentIndex() const
{
    return 0;
}

void DetailsWidgetContainerExtension::insertWidget(int /* index */, QWidget *widget)
{
    addWidget(widget);
}

void DetailsWidgetContainerExtension::remove(int /* index */)
{
    m_detailsWidget->setWidget(0);
}

void DetailsWidgetContainerExtension::setCurrentIndex(int /* index */)
{
}

QWidget *DetailsWidgetContainerExtension::widget(int  /* index */) const
{
    return m_detailsWidget->widget();
}

DetailsWidgetExtensionFactory::DetailsWidgetExtensionFactory(QExtensionManager *parent) :
    QExtensionFactory(parent)
{
}

QObject *DetailsWidgetExtensionFactory::createExtension(QObject *object, const QString &iid, QObject *parent) const
{
    if (Utils::DetailsWidget *dw = qobject_cast<Utils::DetailsWidget*>(object))
        if (iid == Q_TYPEID(QDesignerContainerExtension))
            return new DetailsWidgetContainerExtension(dw, parent);
    return 0;
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
    m_plugins.push_back(new IconButton_CW(this));
    m_plugins.push_back(new FancyLineEdit_CW(this));
    m_plugins.push_back(new FilterLineEdit_CW(this));
    m_plugins.push_back(new QtColorButton_CW(this));
    m_plugins.push_back(new SubmitEditorWidget_CW(this));
    m_plugins.push_back(new SubmitFieldWidget_CW(this));
    m_plugins.push_back(new PathListEditor_CW(this));
    m_plugins.push_back(new DetailsButton_CW(this));
    m_plugins.push_back(new DetailsWidget_CW(this));
    m_plugins.push_back(new StyledBar_CW(this));
    m_plugins.push_back(new StyledSeparator_CW(this));
    m_plugins.push_back(new Wizard_CW(this));
    m_plugins.push_back(new CrumblePath_CW(this));
}

QList<QDesignerCustomWidgetInterface*> WidgetCollection::customWidgets() const
{
    return m_plugins;
}

Q_EXPORT_PLUGIN(WidgetCollection)
