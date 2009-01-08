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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "cpptoolsplugin.h"

#include "completionsettingspage.h"
#include "cppclassesfilter.h"
#include "cppcodecompletion.h"
#include "cppfunctionsfilter.h"
#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"
#include "cppquickopenfilter.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanagerinterface.h>
#include <coreplugin/editormanager/editormanager.h>
#include <cppeditor/cppeditorconstants.h>

#include <QtCore/qplugin.h>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QMenu>
#include <QtGui/QAction>

using namespace CppTools::Internal;

enum { debug = 0 };


CppToolsPlugin *CppToolsPlugin::m_instance = 0;

CppToolsPlugin::CppToolsPlugin() :
    m_core(0),
    m_context(-1),
    m_modelManager(0)
{
    m_instance = this;
}

CppToolsPlugin::~CppToolsPlugin()
{
    m_instance = 0;
    m_modelManager = 0; // deleted automatically
}

bool CppToolsPlugin::initialize(const QStringList & /*arguments*/, QString *)
{
    m_core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    Core::ActionManagerInterface *am = m_core->actionManager();

    // Objects
    m_modelManager = new CppModelManager(this);
    addAutoReleasedObject(m_modelManager);
    m_completion = new CppCodeCompletion(m_modelManager, m_core);
    addAutoReleasedObject(m_completion);
    CppQuickOpenFilter *quickOpenFilter = new CppQuickOpenFilter(m_modelManager,
                                                                 m_core->editorManager());
    addAutoReleasedObject(quickOpenFilter);
    addAutoReleasedObject(new CppClassesFilter(m_modelManager, m_core->editorManager()));
    addAutoReleasedObject(new CppFunctionsFilter(m_modelManager, m_core->editorManager()));
    addAutoReleasedObject(new CompletionSettingsPage(m_completion));

    // Menus
    Core::IActionContainer *mtools = am->actionContainer(Core::Constants::M_TOOLS);
    Core::IActionContainer *mcpptools = am->createMenu(CppTools::Constants::M_TOOLS_CPP);
    QMenu *menu = mcpptools->menu();
    menu->setTitle(tr("&C++"));
    menu->setEnabled(true);
    mtools->addMenu(mcpptools);

    // Actions
    m_context = m_core->uniqueIDManager()->uniqueIdentifier(CppEditor::Constants::C_CPPEDITOR);
    QList<int> context = QList<int>() << m_context;

    QAction *switchAction = new QAction(tr("Switch Header/Source"), this);
    Core::ICommand *command = am->registerAction(switchAction, Constants::SWITCH_HEADER_SOURCE, context);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_F4));
    mcpptools->addAction(command);
    connect(switchAction, SIGNAL(triggered()), this, SLOT(switchHeaderSource()));

    // Restore settings
    QSettings *settings = m_core->settings();
    settings->beginGroup(QLatin1String("CppTools"));
    settings->beginGroup(QLatin1String("Completion"));
    const bool caseSensitive = settings->value(QLatin1String("CaseSensitive"), true).toBool();
    m_completion->setCaseSensitivity(caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
    m_completion->setAutoInsertBraces(settings->value(QLatin1String("AutoInsertBraces"), true).toBool());
    m_completion->setPartialCompletionEnabled(settings->value(QLatin1String("PartiallyComplete"), true).toBool());
    settings->endGroup();
    settings->endGroup();

    return true;
}

void CppToolsPlugin::extensionsInitialized()
{
}

void CppToolsPlugin::shutdown()
{
    // Save settings
    QSettings *settings = m_core->settings();
    settings->beginGroup(QLatin1String("CppTools"));
    settings->beginGroup(QLatin1String("Completion"));
    settings->setValue(QLatin1String("CaseSensitive"), m_completion->caseSensitivity() == Qt::CaseSensitive);
    settings->setValue(QLatin1String("AutoInsertBraces"), m_completion->autoInsertBraces());
    settings->setValue(QLatin1String("PartiallyComplete"), m_completion->isPartialCompletionEnabled());
    settings->endGroup();
    settings->endGroup();
}

void CppToolsPlugin::switchHeaderSource()
{
    if (!m_core)
        return;

    Core::IEditor *editor = m_core->editorManager()->currentEditor();
    QString otherFile = correspondingHeaderOrSource(editor->file()->fileName());
    if (!otherFile.isEmpty()) {
        m_core->editorManager()->openEditor(otherFile);
        m_core->editorManager()->ensureEditorManagerVisible();
    }
}

QFileInfo CppToolsPlugin::findFile(const QDir &dir, const QString &name,
                                   const ProjectExplorer::Project *project) const
{
    if (debug)
        qDebug() << Q_FUNC_INFO << dir << name;

    QFileInfo fileInSameDir(dir, name);
    if (project && !fileInSameDir.isFile()) {
        QString pattern = QString(1, QLatin1Char('/'));
        pattern += name;
        const QStringList projectFiles = project->files(ProjectExplorer::Project::AllFiles);
        const QStringList::const_iterator pcend = projectFiles.constEnd();
        for (QStringList::const_iterator it = projectFiles.constBegin(); it != pcend; ++it)
            if (it->endsWith(pattern))
                return QFileInfo(*it);
        return QFileInfo();
    }
    return fileInSameDir;
}

// Figure out file type
enum FileType {
    HeaderFile,
    C_SourceFile,
    CPP_SourceFile,
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
        return mimeDatase->findByType(QLatin1String(CppTools::Constants::C_SOURCE_MIMETYPE)).suffixes() +
               mimeDatase->findByType(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)).suffixes();
    case C_SourceFile:
        return mimeDatase->findByType(QLatin1String(CppTools::Constants::C_HEADER_MIMETYPE)).suffixes();
    case CPP_SourceFile:
        return mimeDatase->findByType(QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE)).suffixes();
    }
    return QStringList();
}

QString CppToolsPlugin::correspondingHeaderOrSourceI(const QString &fileName) const
{
    const Core::ICore *core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    const Core::MimeDatabase *mimeDatase = core->mimeDatabase();
    ProjectExplorer::ProjectExplorerPlugin *explorer =
        ExtensionSystem::PluginManager::instance()->getObject<ProjectExplorer::ProjectExplorerPlugin>();
    ProjectExplorer::Project *project = (explorer ? explorer->currentProject() : 0);

    const QFileInfo fi(fileName);
    const FileType type = fileType(mimeDatase, fi);

    if (debug)
        qDebug() << Q_FUNC_INFO << fileName <<  type;

    if (type == UnknownType)
        return QString();

    const QDir absoluteDir = fi.absoluteDir();
    const QString baseName = fi.baseName();
    const QStringList suffixes = matchingCandidateSuffixes(mimeDatase, type);

    const QString privateHeaderSuffix = QLatin1String("_p");
    const QChar dot = QLatin1Char('.');
    QStringList candidates;
    // Check base matches 'source.h'-> 'source.cpp' and vice versa
    const QStringList::const_iterator scend = suffixes.constEnd();
    for (QStringList::const_iterator it = suffixes.constBegin(); it != scend; ++it) {
        QString candidate = baseName;
        candidate += dot;
        candidate += *it;
        const QFileInfo candidateFi = findFile(absoluteDir, candidate, project);
        if (candidateFi.isFile())
            return candidateFi.absoluteFilePath();
    }
    if (type == HeaderFile) {
        // 'source_p.h': try 'source.cpp'
        if (baseName.endsWith(privateHeaderSuffix)) {
            QString sourceBaseName = baseName;
            sourceBaseName.truncate(sourceBaseName.size() - privateHeaderSuffix.size());
            for (QStringList::const_iterator it = suffixes.constBegin(); it != scend; ++it) {
                QString candidate = sourceBaseName;
                candidate += dot;
                candidate += *it;
                const QFileInfo candidateFi = findFile(absoluteDir, candidate, project);
                if (candidateFi.isFile())
                    return candidateFi.absoluteFilePath();
            }
        }
    } else {
        // 'source.cpp': try 'source_p.h'
        const QStringList::const_iterator scend = suffixes.constEnd();
        for (QStringList::const_iterator it = suffixes.constBegin(); it != scend; ++it) {
            QString candidate = baseName;
            candidate += privateHeaderSuffix;
            candidate += dot;
            candidate += *it;
            const QFileInfo candidateFi = findFile(absoluteDir, candidate, project);
            if (candidateFi.isFile())
                return candidateFi.absoluteFilePath();
        }
    }
    return QString();
}

QString CppToolsPlugin::correspondingHeaderOrSource(const QString &fileName) const
{
    const QString rc = correspondingHeaderOrSourceI(fileName);
    if (debug)
        qDebug() << Q_FUNC_INFO << fileName << rc;
    return rc;
}

Q_EXPORT_PLUGIN(CppToolsPlugin)
