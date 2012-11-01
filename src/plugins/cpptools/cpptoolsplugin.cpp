/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cpptoolsplugin.h"
#include "completionsettingspage.h"
#include "cppfilesettingspage.h"
#include "cppcodestylesettingspage.h"
#include "cppclassesfilter.h"
#include "cppfunctionsfilter.h"
#include "cppcurrentdocumentfilter.h"
#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"
#include "cpplocatorfilter.h"
#include "symbolsfindfilter.h"
#include "cpptoolssettings.h"
#include "cpptoolsreuse.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/documentmanager.h>
#include <cppeditor/cppeditorconstants.h>
#include <texteditor/basetexteditor.h>

#include <QtConcurrentRun>
#include <QFutureSynchronizer>
#include <utils/runextensions.h>

#include <find/ifindfilter.h>
#include <find/searchresultwindow.h>
#include <utils/filesearch.h>
#include <utils/qtcassert.h>

#include <QtPlugin>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QMenu>
#include <QAction>

#include <sstream>

using namespace CPlusPlus;

namespace CppTools {
namespace Internal {

enum { debug = 0 };

static CppToolsPlugin *m_instance = 0;
static QHash<QString, QString> m_headerSourceMapping;

CppToolsPlugin::CppToolsPlugin() :
    m_fileSettings(new CppFileSettings)
{
    m_instance = this;
}

CppToolsPlugin::~CppToolsPlugin()
{
    m_instance = 0;
    delete CppModelManager::instance();
}

bool CppToolsPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error)

    m_settings = new CppToolsSettings(this); // force registration of cpp tools settings

    // Objects
    CppModelManager *modelManager = CppModelManager::instance();
    Core::VcsManager *vcsManager = Core::ICore::vcsManager();
    connect(vcsManager, SIGNAL(repositoryChanged(QString)),
            modelManager, SLOT(updateModifiedSourceFiles()));
    connect(Core::DocumentManager::instance(), SIGNAL(filesChangedInternally(QStringList)),
            modelManager, SLOT(updateSourceFiles(QStringList)));

    addAutoReleasedObject(new CppLocatorFilter(modelManager));
    addAutoReleasedObject(new CppClassesFilter(modelManager));
    addAutoReleasedObject(new CppFunctionsFilter(modelManager));
    addAutoReleasedObject(new CppCurrentDocumentFilter(modelManager, Core::ICore::editorManager()));
    addAutoReleasedObject(new CppFileSettingsPage(m_fileSettings));
    addAutoReleasedObject(new SymbolsFindFilter(modelManager));
    addAutoReleasedObject(new CppCodeStyleSettingsPage);

    // Menus
    Core::ActionContainer *mtools = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    Core::ActionContainer *mcpptools = Core::ActionManager::createMenu(CppTools::Constants::M_TOOLS_CPP);
    QMenu *menu = mcpptools->menu();
    menu->setTitle(tr("&C++"));
    menu->setEnabled(true);
    mtools->addMenu(mcpptools);

    // Actions
    Core::Context context(CppEditor::Constants::C_CPPEDITOR);

    QAction *switchAction = new QAction(tr("Switch Header/Source"), this);
    Core::Command *command = Core::ActionManager::registerAction(switchAction, Constants::SWITCH_HEADER_SOURCE, context, true);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_F4));
    mcpptools->addAction(command);
    connect(switchAction, SIGNAL(triggered()), this, SLOT(switchHeaderSource()));

    return true;
}

void CppToolsPlugin::extensionsInitialized()
{
    // The Cpp editor plugin, which is loaded later on, registers the Cpp mime types,
    // so, apply settings here
    m_fileSettings->fromSettings(Core::ICore::settings());
    if (!m_fileSettings->applySuffixesToMimeDB())
        qWarning("Unable to apply cpp suffixes to mime database (cpp mime types not found).\n");
}

ExtensionSystem::IPlugin::ShutdownFlag CppToolsPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

void CppToolsPlugin::switchHeaderSource()
{
    Core::IEditor *editor = Core::EditorManager::currentEditor();
    QString otherFile = correspondingHeaderOrSource(editor->document()->fileName());
    if (otherFile.isEmpty())
        return;

    Core::EditorManager* editorManager = Core::EditorManager::instance();
    editorManager->addCurrentPositionToNavigationHistory();
    TextEditor::BaseTextEditorWidget *ed = qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget());
    if (editorManager->hasSplitter()) {
        if (ed->forceOpenLinksInNextSplit()) {
            editorManager->gotoOtherSplit();
        } else if (ed->openLinksInNextSplit()) {
            bool isVisible = false;
            foreach (Core::IEditor *visEditor, editorManager->visibleEditors()) {
                if (visEditor->document() &&
                        (otherFile == visEditor->document()->fileName())) {
                    isVisible = true;
                    editorManager->activateEditor(visEditor);
                    break;
                }
            }

            if (!isVisible)
                editorManager->gotoOtherSplit();
        }
    }
    Core::EditorManager::openEditor(otherFile);
}

static QStringList findFilesInProject(const QString &name,
                                   const ProjectExplorer::Project *project)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << name << project;

    if (!project)
        return QStringList();

    QString pattern = QString(1, QLatin1Char('/'));
    pattern += name;
    const QStringList projectFiles = project->files(ProjectExplorer::Project::AllFiles);
    const QStringList::const_iterator pcend = projectFiles.constEnd();
    QStringList candidateList;
    for (QStringList::const_iterator it = projectFiles.constBegin(); it != pcend; ++it) {
        if (it->endsWith(pattern))
            candidateList.append(*it);
    }
    return candidateList;
}

// Figure out file type
enum FileType {
    HeaderFile,
    C_SourceFile,
    CPP_SourceFile,
    ObjectiveCPP_SourceFile,
    UnknownType
};

static inline FileType fileType(const Core::MimeDatabase *mimeDatase, const  QFileInfo & fi)
{
    const Core::MimeType mimeType = mimeDatase->findByFile(fi);
    if (!mimeType)
        return UnknownType;
    const QString typeName = mimeType.type();
    if (typeName == QLatin1String(CppTools::Constants::C_SOURCE_MIMETYPE))
        return C_SourceFile;
    if (typeName == QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE))
        return CPP_SourceFile;
    if (typeName == QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE))
        return ObjectiveCPP_SourceFile;
    if (typeName == QLatin1String(CppTools::Constants::C_HEADER_MIMETYPE)
        || typeName == QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE))
        return HeaderFile;
    return UnknownType;
}

// Return the suffixes that should be checked when trying to find a
// source belonging to a header and vice versa
static QStringList matchingCandidateSuffixes(const Core::MimeDatabase *mimeDatase, FileType type)
{
    switch (type) {
    case UnknownType:
        break;
    case HeaderFile: // Note that C/C++ headers are undistinguishable
        return mimeDatase->findByType(QLatin1String(CppTools::Constants::C_SOURCE_MIMETYPE)).suffixes()
               + mimeDatase->findByType(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)).suffixes()
               + mimeDatase->findByType(QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE)).suffixes();
    case C_SourceFile:
        return mimeDatase->findByType(QLatin1String(CppTools::Constants::C_HEADER_MIMETYPE)).suffixes();
    case CPP_SourceFile:
    case ObjectiveCPP_SourceFile:
        return mimeDatase->findByType(QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE)).suffixes();
    }
    return QStringList();
}

static QStringList baseNameWithAllSuffixes(const QString &baseName, const QStringList &suffixes)
{
    QStringList result;
    const QChar dot = QLatin1Char('.');
    foreach (const QString &suffix, suffixes) {
        QString fileName = baseName;
        fileName += dot;
        fileName += suffix;
        result += fileName;
    }
    return result;
}

static int commonStringLength(const QString &s1, const QString &s2)
{
    int length = qMin(s1.length(), s2.length());
    for (int i = 0; i < length; ++i)
        if (s1[i] != s2[i])
            return i;
    return length;
}

} // namespace Internal

QString correspondingHeaderOrSource(const QString &fileName, bool *wasHeader)
{
    using namespace Internal;

    const Core::MimeDatabase *mimeDatase = Core::ICore::mimeDatabase();
    const QFileInfo fi(fileName);
    if (m_headerSourceMapping.contains(fi.absoluteFilePath())) {
        if (wasHeader)
            *wasHeader = fileType(mimeDatase, fi) == HeaderFile;
        return m_headerSourceMapping.value(fi.absoluteFilePath());
    }

    FileType type = fileType(mimeDatase, fi);
    if (wasHeader)
        *wasHeader = type == HeaderFile;

    if (debug)
        qDebug() << Q_FUNC_INFO << fileName <<  type;

    if (type == UnknownType)
        return QString();

    const QString baseName = fi.completeBaseName();
    const QString privateHeaderSuffix = QLatin1String("_p");
    const QStringList suffixes = matchingCandidateSuffixes(mimeDatase, type);

    QStringList candidateFileNames = baseNameWithAllSuffixes(baseName, suffixes);
    if (type == HeaderFile) {
        if (baseName.endsWith(privateHeaderSuffix)) {
            QString sourceBaseName = baseName;
            sourceBaseName.truncate(sourceBaseName.size() - privateHeaderSuffix.size());
            candidateFileNames += baseNameWithAllSuffixes(sourceBaseName, suffixes);
        }
    } else {
        QString privateHeaderBaseName = baseName;
        privateHeaderBaseName.append(privateHeaderSuffix);
        candidateFileNames += baseNameWithAllSuffixes(privateHeaderBaseName, suffixes);
    }

    const QDir absoluteDir = fi.absoluteDir();

    // Try to find a file in the same directory first
    foreach (const QString &candidateFileName, candidateFileNames) {
        const QFileInfo candidateFi(absoluteDir, candidateFileName);
        if (candidateFi.isFile()) {
            m_headerSourceMapping[fi.absoluteFilePath()] = candidateFi.absoluteFilePath();
            if (type != HeaderFile || !baseName.endsWith(privateHeaderSuffix))
                m_headerSourceMapping[candidateFi.absoluteFilePath()] = fi.absoluteFilePath();
            return candidateFi.absoluteFilePath();
        }
    }

    // Find files in the project
    ProjectExplorer::Project *project = ProjectExplorer::ProjectExplorerPlugin::currentProject();
    if (project) {
        QString bestFileName;
        int compareValue = 0;
        foreach (const QString &candidateFileName, candidateFileNames) {
            const QStringList projectFiles = findFilesInProject(candidateFileName, project);
            // Find the file having the most common path with fileName
            foreach (const QString projectFile, projectFiles) {
                int value = commonStringLength(fileName, projectFile);
                if (value > compareValue) {
                    compareValue = value;
                    bestFileName = projectFile;
                }
            }
        }
        if (!bestFileName.isEmpty()) {
            const QFileInfo candidateFi(bestFileName);
            QTC_ASSERT(candidateFi.isFile(), return QString());
            m_headerSourceMapping[fi.absoluteFilePath()] = candidateFi.absoluteFilePath();
            m_headerSourceMapping[candidateFi.absoluteFilePath()] = fi.absoluteFilePath();
            return candidateFi.absoluteFilePath();
        }
    }

    return QString();
}

} // namespace CppTools

Q_EXPORT_PLUGIN(CppTools::Internal::CppToolsPlugin)
