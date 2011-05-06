/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "allprojectsfind.h"

#include "project.h"
#include "session.h"
#include "projectexplorer.h"
#include "editorconfiguration.h"

#include <utils/qtcassert.h>
#include <texteditor/itexteditor.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QSettings>
#include <QtCore/QRegExp>

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

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

QString AllProjectsFind::id() const
{
    return QLatin1String("All Projects");
}

QString AllProjectsFind::displayName() const
{
    return tr("All Projects");
}

bool AllProjectsFind::isEnabled() const
{
    return BaseFileFind::isEnabled()
            && m_plugin->session() != 0
            && m_plugin->session()->projects().count() > 0;
}

QList<Project *> AllProjectsFind::projects() const
{
    Q_ASSERT(m_plugin->session());
    return m_plugin->session()->projects();
}

Utils::FileIterator *AllProjectsFind::files() const
{
    QList<QRegExp> filterRegs;
    QStringList nameFilters = fileNameFilters();
    foreach (const QString &filter, nameFilters) {
        filterRegs << QRegExp(filter, Qt::CaseInsensitive, QRegExp::Wildcard);
    }
    QMap<QString, QTextCodec *> openEditorEncodings = TextEditor::ITextEditor::openedTextEditorsEncodings();
    QMap<QString, QTextCodec *> encodings;
    foreach (const Project *project, projects()) {
        QStringList projectFiles = project->files(Project::AllFiles);
        QStringList filteredFiles;
        if (!filterRegs.isEmpty()) {
            foreach (const QString &file, projectFiles) {
                foreach (const QRegExp &reg, filterRegs) {
                    if (reg.exactMatch(file) || reg.exactMatch(QFileInfo(file).fileName())) {
                        filteredFiles.append(file);
                        break;
                    }
                }
            }
        } else {
            filteredFiles = projectFiles;
        }
        foreach (const QString &fileName, filteredFiles) {
            QTextCodec *codec = openEditorEncodings.value(fileName);
            if (!codec)
                codec = project->editorConfiguration()->textCodec();
            encodings.insert(fileName, codec);
        }
    }
    return new Utils::FileIterator(encodings.keys(), encodings.values());
}

QWidget *AllProjectsFind::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        QGridLayout * const gridLayout = new QGridLayout(m_configWidget);
        gridLayout->setMargin(0);
        m_configWidget->setLayout(gridLayout);
        QLabel * const filePatternLabel = new QLabel(tr("File &pattern:"));
        filePatternLabel->setMinimumWidth(80);
        filePatternLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        filePatternLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QWidget *patternWidget = createPatternWidget();
        filePatternLabel->setBuddy(patternWidget);
        gridLayout->addWidget(filePatternLabel, 0, 0, Qt::AlignRight);
        gridLayout->addWidget(patternWidget, 0, 1);
        m_configWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }
    return m_configWidget;
}

void AllProjectsFind::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("AllProjectsFind"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void AllProjectsFind::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("AllProjectsFind"));
    readCommonSettings(settings, QString(QLatin1Char('*')));
    settings->endGroup();
}
