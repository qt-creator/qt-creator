/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef CUSTOMWIDGETS_H
#define CUSTOMWIDGETS_H

#include "customwidget.h"

#include <utils/newclasswidget.h>
#include <utils/classnamevalidatinglineedit.h>
#include <utils/filenamevalidatinglineedit.h>
#include <utils/linecolumnlabel.h>
#include <utils/pathchooser.h>
#include <utils/projectnamevalidatinglineedit.h>
#include <utils/fancylineedit.h>
#include <utils/qtcolorbutton.h>
#include <utils/submiteditorwidget.h>

#include <QtDesigner/QDesignerCustomWidgetCollectionInterface>

#include <QtCore/qplugin.h>
#include <QtCore/QList>

// Custom Widgets

class NewClassCustomWidget :
    public QObject,
    public CustomWidget<Core::Utils::NewClassWidget>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit NewClassCustomWidget(QObject *parent = 0);
};

class ClassNameValidatingLineEdit_CW :
    public QObject,
    public CustomWidget<Core::Utils::ClassNameValidatingLineEdit>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit ClassNameValidatingLineEdit_CW(QObject *parent = 0);
};

class FileNameValidatingLineEdit_CW :
    public QObject,
    public CustomWidget<Core::Utils::FileNameValidatingLineEdit>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit FileNameValidatingLineEdit_CW(QObject *parent = 0);
};

class ProjectNameValidatingLineEdit_CW :
    public QObject,
    public CustomWidget<Core::Utils::ProjectNameValidatingLineEdit>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit ProjectNameValidatingLineEdit_CW(QObject *parent = 0);
};

class LineColumnLabel_CW :
    public QObject,
    public CustomWidget<Core::Utils::LineColumnLabel>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit LineColumnLabel_CW(QObject *parent = 0);
};

class PathChooser_CW :
    public QObject,
    public CustomWidget<Core::Utils::PathChooser>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit PathChooser_CW(QObject *parent = 0);
};

class FancyLineEdit_CW :
    public QObject,
    public CustomWidget<Core::Utils::FancyLineEdit>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit FancyLineEdit_CW(QObject *parent = 0);

    virtual QWidget *createWidget(QWidget *parent);
};

class QtColorButton_CW :
    public QObject,
    public CustomWidget<Core::Utils::QtColorButton>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit QtColorButton_CW(QObject *parent = 0);
};

class SubmitEditorWidget_CW :
    public QObject,
    public CustomWidget<Core::Utils::SubmitEditorWidget>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit SubmitEditorWidget_CW(QObject *parent = 0);
};

// Collection

class WidgetCollection : public QObject, public QDesignerCustomWidgetCollectionInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)
public:
    explicit WidgetCollection(QObject *parent = 0);

    virtual QList<QDesignerCustomWidgetInterface*> customWidgets() const;

private:
    QList<QDesignerCustomWidgetInterface*> m_plugins;
};

#endif // CUSTOMWIDGETS_H
