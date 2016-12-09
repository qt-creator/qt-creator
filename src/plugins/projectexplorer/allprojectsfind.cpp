/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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

AllProjectsFind::AllProjectsFind() :  m_configWidget(nullptr)
{
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::fileListChanged,
            this, &AllProjectsFind::handleFileListChanged);
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
                                            const QStringList &exclusionFilters,
                                            const QVariant &additionalParameters) const
{
    Q_UNUSED(additionalParameters)
    return filesForProjects(nameFilters, exclusionFilters, SessionManager::projects());
}

Utils::FileIterator *AllProjectsFind::filesForProjects(const QStringList &nameFilters,
                                                       const QStringList &exclusionFilters,
                                                       const QList<Project *> &projects) const
{
    std::function<QStringList(const QStringList &)> filterFiles =
            Utils::filterFilesFunction(nameFilters, exclusionFilters);
    const QMap<QString, QTextCodec *> openEditorEncodings = TextDocument::openedTextDocumentEncodings();
    QMap<QString, QTextCodec *> encodings;
    foreach (const Project *project, projects) {
        const EditorConfiguration *config = project->editorConfiguration();
        QTextCodec *projectCodec = config->useGlobalSettings()
            ? Core::EditorManager::defaultTextCodec()
            : config->textCodec();
        const QStringList filteredFiles = filterFiles(project->files(Project::AllFiles));
        for (const QString &fileName : filteredFiles) {
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
    // last arg is filled by BaseFileFind::runNewSearch
    return tr("Filter: %1\nExcluding: %2\n%3")
            .arg(fileNameFilters().join(','))
            .arg(fileExclusionFilters().join(','));
}

void AllProjectsFind::handleFileListChanged()
{
    emit enabledChanged(isEnabled());
}

QWidget *AllProjectsFind::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        auto gridLayout = new QGridLayout(m_configWidget);
        gridLayout->setMargin(0);
        m_configWidget->setLayout(gridLayout);
        const QList<QPair<QWidget *, QWidget *>> patternWidgets = createPatternWidgets();
        int row = 0;
        for (const QPair<QWidget *, QWidget *> &p : patternWidgets) {
            gridLayout->addWidget(p.first, row, 0, Qt::AlignRight);
            gridLayout->addWidget(p.second, row, 1);
            ++row;
        }
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
    readCommonSettings(settings, "*", "");
    settings->endGroup();
}
