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

#include "allprojectsfind.h"

#include "project.h"
#include "projectexplorer.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QRegExp>
#include <QtGui/QGridLayout>

using namespace Find;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace TextEditor;

AllProjectsFind::AllProjectsFind(ProjectExplorerPlugin *plugin, SearchResultWindow *resultWindow)
    : BaseFileFind(resultWindow),
    m_plugin(plugin),
    m_configWidget(0)
{
    connect(m_plugin, SIGNAL(fileListChanged()), this, SIGNAL(changed()));
}

QString AllProjectsFind::name() const
{
    return tr("All Projects");
}

bool AllProjectsFind::isEnabled() const
{
    return BaseFileFind::isEnabled()
            && m_plugin->session() != 0
            && m_plugin->session()->projects().count() > 0;
}

QKeySequence AllProjectsFind::defaultShortcut() const
{
    return QKeySequence("Ctrl+Shift+F");
}

QStringList AllProjectsFind::files()
{
    Q_ASSERT(m_plugin->session());
    QList<QRegExp> filterRegs;
    QStringList nameFilters = fileNameFilters();
    foreach (const QString &filter, nameFilters) {
        filterRegs << QRegExp(filter, Qt::CaseInsensitive, QRegExp::Wildcard);
    }
    QStringList files;
    QStringList projectFiles;
    foreach (const Project *project, m_plugin->session()->projects()) {
        projectFiles = project->files(Project::AllFiles);
        if (!filterRegs.isEmpty()) {
            foreach (const QString &file, projectFiles) {
                foreach (const QRegExp &reg, filterRegs) {
                    if (reg.exactMatch(file)) {
                        files.append(file);
                        break;
                    }
                }
            }
        } else {
            files += projectFiles;
        }
    }
    files.removeDuplicates();
    return files;
}

QWidget *AllProjectsFind::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        QGridLayout * const gridLayout = new QGridLayout(m_configWidget);
        gridLayout->setMargin(0);
        m_configWidget->setLayout(gridLayout);
        gridLayout->addWidget(createRegExpWidget(), 0, 1);
        QLabel * const filePatternLabel = new QLabel(tr("File &pattern:"));
        filePatternLabel->setMinimumWidth(80);
        filePatternLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        filePatternLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QWidget *patternWidget = createPatternWidget();
        filePatternLabel->setBuddy(patternWidget);
        gridLayout->addWidget(filePatternLabel, 1, 0, Qt::AlignRight);
        gridLayout->addWidget(patternWidget, 1, 1);
        m_configWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }
    return m_configWidget;
}

void AllProjectsFind::writeSettings(QSettings *settings)
{
    settings->beginGroup("AllProjectsFind");
    writeCommonSettings(settings);
    settings->endGroup();
}

void AllProjectsFind::readSettings(QSettings *settings)
{
    settings->beginGroup("AllProjectsFind");
    readCommonSettings(settings, "*");
    settings->endGroup();
}
