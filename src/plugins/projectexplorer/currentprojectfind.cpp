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

#include "currentprojectfind.h"

#include "projectexplorer.h"
#include "project.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QRegExp>
#include <QtGui/QGridLayout>

using namespace Find;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace TextEditor;

CurrentProjectFind::CurrentProjectFind(ProjectExplorerPlugin *plugin, SearchResultWindow *resultWindow)
  : BaseFileFind(resultWindow),
    m_plugin(plugin),
    m_configWidget(0)
{
    connect(m_plugin, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
            this, SIGNAL(changed()));
}

QString CurrentProjectFind::name() const
{
    return tr("Current Project");
}

bool CurrentProjectFind::isEnabled() const
{
    return m_plugin->currentProject() != 0 && BaseFileFind::isEnabled();
}

QKeySequence CurrentProjectFind::defaultShortcut() const
{
    return QKeySequence("Ctrl+Alt+F");
}

QStringList CurrentProjectFind::files()
{
    Project *project = m_plugin->currentProject();
    Q_ASSERT(project);
    QList<QRegExp> filterRegs;
    QStringList nameFilters = fileNameFilters();
    foreach (const QString &filter, nameFilters) {
        filterRegs << QRegExp(filter, Qt::CaseInsensitive, QRegExp::Wildcard);
    }
    QStringList files;
    if (!filterRegs.isEmpty()) {
        foreach (const QString &file, project->files(Project::AllFiles)) {
            foreach (const QRegExp &reg, filterRegs) {
                if (reg.exactMatch(file)) {
                    files.append(file);
                    break;
                }
            }
        }
    } else {
        files += project->files(Project::AllFiles);
    }
    files.removeDuplicates();
    return files;
}

QWidget *CurrentProjectFind::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        QGridLayout * const layout = new QGridLayout(m_configWidget);
        layout->setMargin(0);
        m_configWidget->setLayout(layout);
        layout->addWidget(createRegExpWidget(), 0, 1);
        QLabel * const filePatternLabel = new QLabel(tr("File &pattern:"));
        filePatternLabel->setMinimumWidth(80);
        filePatternLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        filePatternLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QWidget *patternWidget = createPatternWidget();
        filePatternLabel->setBuddy(patternWidget);
        layout->addWidget(filePatternLabel, 1, 0, Qt::AlignRight);
        layout->addWidget(patternWidget, 1, 1);
        m_configWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }
    return m_configWidget;
}

void CurrentProjectFind::writeSettings(QSettings *settings)
{
    settings->beginGroup("CurrentProjectFind");
    writeCommonSettings(settings);
    settings->endGroup();
}

void CurrentProjectFind::readSettings(QSettings *settings)
{
    settings->beginGroup("CurrentProjectFind");
    readCommonSettings(settings, "*");
    settings->endGroup();
}
