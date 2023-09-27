// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "allprojectsfind.h"

#include "editorconfiguration.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorertr.h"
#include "projectmanager.h"

#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/qtcsettings.h>

#include <QGridLayout>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace TextEditor;
using namespace Utils;

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
    return Tr::tr("All Projects");
}

bool AllProjectsFind::isEnabled() const
{
    return BaseFileFind::isEnabled() && ProjectManager::hasProjects();
}

FileContainerProvider AllProjectsFind::fileContainerProvider() const
{
    return [nameFilters = fileNameFilters(), exclusionFilters = fileExclusionFilters()] {
        return filesForProjects(nameFilters, exclusionFilters, ProjectManager::projects());
    };
}

FileContainer AllProjectsFind::filesForProjects(const QStringList &nameFilters,
                                                const QStringList &exclusionFilters,
                                                const QList<Project *> &projects)
{
    std::function<FilePaths(const FilePaths &)> filterFiles
        = Utils::filterFilesFunction(nameFilters, exclusionFilters);
    const QMap<FilePath, QTextCodec *> openEditorEncodings
        = TextDocument::openedTextDocumentEncodings();
    QMap<FilePath, QTextCodec *> encodings;
    for (const Project *project : projects) {
        const EditorConfiguration *config = project->editorConfiguration();
        QTextCodec *projectCodec = config->useGlobalSettings()
            ? Core::EditorManager::defaultTextCodec()
            : config->textCodec();
        const FilePaths filteredFiles = filterFiles(project->files(Project::SourceFiles));
        for (const FilePath &fileName : filteredFiles) {
            QTextCodec *codec = openEditorEncodings.value(fileName);
            if (!codec)
                codec = projectCodec;
            encodings.insert(fileName, codec);
        }
    }
    return FileListContainer(encodings.keys(), encodings.values());
}

QString AllProjectsFind::label() const
{
    return Tr::tr("All Projects:");
}

QString AllProjectsFind::toolTip() const
{
    // last arg is filled by BaseFileFind::runNewSearch
    return Tr::tr("Filter: %1\nExcluding: %2\n%3")
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
        gridLayout->setContentsMargins(0, 0, 0, 0);
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

void AllProjectsFind::writeSettings(QtcSettings *settings)
{
    settings->beginGroup("AllProjectsFind");
    writeCommonSettings(settings);
    settings->endGroup();
}

void AllProjectsFind::readSettings(QtcSettings *settings)
{
    settings->beginGroup("AllProjectsFind");
    readCommonSettings(settings, "*", "");
    settings->endGroup();
}
