/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "editorsettingspropertiespage.h"
#include "editorconfiguration.h"
#include "project.h"

#include <QtCore/QTextCodec>

#include <QDebug>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

QString EditorSettingsPanelFactory::id() const
{
    return QLatin1String(EDITORSETTINGS_PANEL_ID);
}

QString EditorSettingsPanelFactory::displayName() const
{
    return QApplication::tr("Editor Settings");
}

bool EditorSettingsPanelFactory::supports(Project *project)
{
    Q_UNUSED(project);
    return true;
}

bool EditorSettingsPanelFactory::supports(Target *target)
{
    Q_UNUSED(target);
    return false;
}

IPropertiesPanel *EditorSettingsPanelFactory::createPanel(Project *project)
{
    return new EditorSettingsPanel(project);
}

IPropertiesPanel *EditorSettingsPanelFactory::createPanel(Target *target)
{
    Q_UNUSED(target);
    return 0;
}

EditorSettingsPanel::EditorSettingsPanel(Project *project) :
    m_widget(new EditorSettingsWidget(project)),
    m_icon(":/projectexplorer/images/EditorSettings.png")
{
}

EditorSettingsPanel::~EditorSettingsPanel()
{
    delete m_widget;
}

QString EditorSettingsPanel::displayName() const
{
    return QApplication::tr("Editor Settings");
}

QWidget *EditorSettingsPanel::widget() const
{
    return m_widget;
}

QIcon EditorSettingsPanel::icon() const
{
    return m_icon;
}

EditorSettingsWidget::EditorSettingsWidget(Project *project)
    : QWidget(),
      m_project(project)
{
    m_ui.setupUi(this);
    QTextCodec *defaultTextCodec = m_project->editorConfiguration()->defaultTextCodec();
    QList<int> mibs = QTextCodec::availableMibs();
    qSort(mibs);
    QList<int> sortedMibs;
    foreach (int mib, mibs)
        if (mib >= 0)
            sortedMibs += mib;
    foreach (int mib, mibs)
        if (mib < 0)
            sortedMibs += mib;
    int i = 0;
    foreach (int mib, sortedMibs) {
        QTextCodec *codec = QTextCodec::codecForMib(mib);
        m_codecs += codec;
        QString name = codec->name();
        foreach (const QByteArray &alias, codec->aliases()) {
            name += QLatin1String(" / ");
            name += QString::fromLatin1(alias);
        }
        m_ui.encodingComboBox->addItem(name);
        if (defaultTextCodec == codec)
            m_ui.encodingComboBox->setCurrentIndex(i);
        i++;
    }

    connect(m_ui.encodingComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(currentEncodingChanged(int)));
}

void EditorSettingsWidget::currentEncodingChanged(int index)
{
    QList<int> codecs = QTextCodec::availableMibs();
    m_project->editorConfiguration()->setDefaultTextCodec(m_codecs.at(index));
}


