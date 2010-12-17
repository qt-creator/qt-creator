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
    return QCoreApplication::translate("EditorSettingsPanelFactory", "Editor Settings");
}

bool EditorSettingsPanelFactory::supports(Project *project)
{
    Q_UNUSED(project);
    return true;
}

IPropertiesPanel *EditorSettingsPanelFactory::createPanel(Project *project)
{
    return new EditorSettingsPanel(project);
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
    return QCoreApplication::translate("EditorSettingsPanel", "Editor Settings");
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
    QTextCodec *defaultTextCodec = 0;
    m_codecs += defaultTextCodec;
    m_ui.encodingComboBox->addItem(tr("Default"));

    defaultTextCodec = m_project->editorConfiguration()->defaultTextCodec();

    QList<int> mibs = QTextCodec::availableMibs();
    qSort(mibs);
    QList<int> sortedMibs;
    foreach (int mib, mibs)
        if (mib >= 0)
            sortedMibs += mib;
    foreach (int mib, mibs)
        if (mib < 0)
            sortedMibs += mib;
    int i = 1; // 0 is the default
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
    m_project->editorConfiguration()->setDefaultTextCodec(m_codecs.at(index));
}


