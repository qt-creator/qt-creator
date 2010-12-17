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

#ifndef CLASSDEFINITION_H
#define CLASSDEFINITION_H

#include "ui_classdefinition.h"
#include "filenamingparameters.h"
#include "pluginoptions.h"

#include <QtGui/QTabWidget>

namespace Qt4ProjectManager {
namespace Internal {

class ClassDefinition : public QTabWidget
{
    Q_OBJECT

public:
    explicit ClassDefinition(QWidget *parent = 0);
    void setClassName(const QString &name);

    FileNamingParameters fileNamingParameters() const { return m_fileNamingParameters; }
    void setFileNamingParameters(const FileNamingParameters &fnp) { m_fileNamingParameters = fnp; }

    PluginOptions::WidgetOptions widgetOptions(const QString &className) const;

    void enableButtons();

private Q_SLOTS:
    void on_libraryRadio_toggled();
    void on_skeletonCheck_toggled();
    void on_widgetLibraryEdit_textChanged();
    void on_widgetHeaderEdit_textChanged();
    void on_pluginClassEdit_textChanged();
    void on_pluginHeaderEdit_textChanged();
    void on_domXmlEdit_textChanged();

private:
    Ui::ClassDefinition m_ui;
    FileNamingParameters m_fileNamingParameters;
    bool m_domXmlChanged;
};

}
}

#endif
