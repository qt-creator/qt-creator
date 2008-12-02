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

#include "editorsettingspropertiespage.h"
#include "editorconfiguration.h"

#include <QtCore/QTextCodec>

#include <QDebug>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

bool EditorSettingsPanelFactory::supports(Project * /*project*/)
{
    return true;
}
PropertiesPanel *EditorSettingsPanelFactory::createPanel(Project *project)
{
    return new EditorSettingsPanel(project);
}

EditorSettingsPanel::EditorSettingsPanel(Project *project)
    : PropertiesPanel(),
      m_widget(new EditorSettingsWidget(project))
{
}

EditorSettingsPanel::~EditorSettingsPanel()
{
    delete m_widget;
}

QString EditorSettingsPanel::name() const
{
    return tr("Editor Settings");
}

QWidget *EditorSettingsPanel::widget()
{
    return m_widget;
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
        foreach (QByteArray alias, codec->aliases())
            name += QLatin1String(" / ") + alias;
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


