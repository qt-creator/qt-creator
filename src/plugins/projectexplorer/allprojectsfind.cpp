/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "allprojectsfind.h"

#include "project.h"
#include "session.h"
#include "projectexplorer.h"
#include "editorconfiguration.h"

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/filesearch.h>
#include <utils/algorithm.h>

#include <QSettings>
#include <QRegExp>

#include <QGridLayout>
#include <QLabel>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace TextEditor;

AllProjectsFind::AllProjectsFind()
    :  m_configWidget(0)
{
    connect(ProjectExplorerPlugin::instance(), SIGNAL(fileListChanged()), this, SLOT(handleFileListChanged()));
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
    return BaseFileFind::isEnabled() && SessionManager::hasProjects();
}

Utils::FileIterator *AllProjectsFind::files(const QStringList &nameFilters,
                                            const QVariant &additionalParameters) const
{
    Q_UNUSED(additionalParameters)
    return filesForProjects(nameFilters, SessionManager::projects());
}

Utils::FileIterator *AllProjectsFind::filesForProjects(const QStringList &nameFilters,
                                            const QList<Project *> &projects) const
{
    QList<QRegExp> filterRegs;
    foreach (const QString &filter, nameFilters) {
        filterRegs << QRegExp(filter, Qt::CaseInsensitive, QRegExp::Wildcard);
    }
    QMap<QString, QTextCodec *> openEditorEncodings = TextDocument::openedTextDocumentEncodings();
    QMap<QString, QTextCodec *> encodings;
    foreach (const Project *project, projects) {
        QStringList projectFiles = project->files(Project::AllFiles);
        QStringList filteredFiles;
        if (!filterRegs.isEmpty()) {
            foreach (const QString &file, projectFiles) {
                if (Utils::anyOf(filterRegs,
                        [&file](QRegExp reg) {
                            return (reg.exactMatch(file) || reg.exactMatch(Utils::FileName::fromString(file).fileName()));
                        })) {
                    filteredFiles.append(file);
                }
            }
        } else {
            filteredFiles = projectFiles;
        }
        const EditorConfiguration *config = project->editorConfiguration();
        QTextCodec *projectCodec = config->useGlobalSettings()
            ? Core::EditorManager::defaultTextCodec()
            : config->textCodec();
        foreach (const QString &fileName, filteredFiles) {
            QTextCodec *codec = openEditorEncodings.value(fileName);
            if (!codec)
                codec = projectCodec;
            encodings.insert(fileName, codec);
        }
    }
    return new Utils::FileListIterator(encodings.keys(), encodings.values());
}

QVariant AllProjectsFind::additionalParameters() const
{
    return QVariant();
}

QString AllProjectsFind::label() const
{
    return tr("All Projects:");
}

QString AllProjectsFind::toolTip() const
{
    // %2 is filled by BaseFileFind::runNewSearch
    return tr("Filter: %1\n%2").arg(fileNameFilters().join(QLatin1Char(',')));
}

void AllProjectsFind::handleFileListChanged()
{
    emit enabledChanged(isEnabled());
}

QWidget *AllProjectsFind::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        QGridLayout * const gridLayout = new QGridLayout(m_configWidget);
        gridLayout->setMargin(0);
        m_configWidget->setLayout(gridLayout);
        QLabel * const filePatternLabel = new QLabel(tr("Fi&le pattern:"));
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
