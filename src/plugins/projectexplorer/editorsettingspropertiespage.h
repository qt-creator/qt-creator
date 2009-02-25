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

#ifndef EDITORSETTINGSPROPERTIESPAGE_H
#define EDITORSETTINGSPROPERTIESPAGE_H

#include "iprojectproperties.h"
#include "ui_editorsettingspropertiespage.h"

namespace ProjectExplorer {

namespace Internal {

class EditorSettingsPanelFactory : public IPanelFactory
{
public:
    bool supports(Project *project);
    PropertiesPanel *createPanel(Project *project);
};

class EditorSettingsWidget;

class EditorSettingsPanel : public PropertiesPanel
{
    Q_OBJECT
public:
    EditorSettingsPanel(Project *project);
    ~EditorSettingsPanel();
    QString name() const;
    QWidget *widget();

private:
    EditorSettingsWidget *m_widget;
};

class EditorSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    EditorSettingsWidget(Project *project);

private slots:
    void currentEncodingChanged(int index);

private:
    Ui::EditorSettingsPropertiesPage m_ui;
    Project *m_project;
    QList<QTextCodec *> m_codecs;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // EDITORSETTINGSPROPERTIESPAGE_H
