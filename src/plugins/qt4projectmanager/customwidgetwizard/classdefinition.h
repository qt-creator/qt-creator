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

#ifndef CLASSDEFINITION_H
#define CLASSDEFINITION_H

#include "ui_classdefinition.h"
#include "filenamingparameters.h"
#include "pluginoptions.h"

#include <QtGui/QTabWidget>

namespace Qt4ProjectManager {
namespace Internal {

class ClassDefinition : public QTabWidget, private Ui::ClassDefinition
{
    Q_OBJECT

public:
    ClassDefinition(QWidget *parent);
    void setClassName(const QString &name);

    FileNamingParameters fileNamingParameters() const { return m_fileNamingParameters; }
    void setFileNamingParameters(const FileNamingParameters &fnp) { m_fileNamingParameters = fnp; }

    PluginOptions::WidgetOptions widgetOptions(const QString &className) const;

public Q_SLOTS:
    void on_libraryRadio_toggled();
    void on_skeletonCheck_toggled();
    void on_widgetLibraryEdit_textChanged();
    void on_widgetHeaderEdit_textChanged();
    void on_pluginClassEdit_textChanged();
    void on_pluginHeaderEdit_textChanged();
    void on_domXmlEdit_textChanged();

private:
    FileNamingParameters m_fileNamingParameters;
    bool m_domXmlChanged;
};

}
}

#endif
