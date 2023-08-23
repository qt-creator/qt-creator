// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlmainfileaspect.h"

#include "qmlproject.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlprojectmanagertr.h"

#include <qmljstools/qmljstoolsconstants.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>

#include <QComboBox>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace QmlProjectManager {

const char M_CURRENT_FILE[] = "CurrentFile";
const char CURRENT_FILE[]  = QT_TRANSLATE_NOOP("QtC::QmlProjectManager", "<Current File>");

static bool caseInsensitiveLessThan(const FilePath &s1, const FilePath &s2)
{
    return s1.toString().toCaseFolded() < s2.toString().toCaseFolded();
}

QmlMainFileAspect::QmlMainFileAspect(AspectContainer *container)
    : BaseAspect(container)
    , m_scriptFile(M_CURRENT_FILE)
{
    addDataExtractor(this, &QmlMainFileAspect::mainScript, &Data::mainScript);
    addDataExtractor(this, &QmlMainFileAspect::currentFile, &Data::currentFile);

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &QmlMainFileAspect::changeCurrentFile);
    connect(EditorManager::instance(), &EditorManager::currentDocumentStateChanged,
            this, [this] { changeCurrentFile(); });
}

QmlMainFileAspect::~QmlMainFileAspect()
{
    delete m_fileListCombo;
}

void QmlMainFileAspect::addToLayout(Layouting::LayoutItem &parent)
{
    QTC_ASSERT(!m_fileListCombo, delete m_fileListCombo);
    m_fileListCombo = new QComboBox;
    m_fileListCombo->setModel(&m_fileListModel);

    updateFileComboBox();

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::fileListChanged,
            this, &QmlMainFileAspect::updateFileComboBox);
    connect(m_fileListCombo, &QComboBox::activated, this, &QmlMainFileAspect::setMainScript);

    parent.addItems({Tr::tr("Main QML file:"), m_fileListCombo.data()});
}

void QmlMainFileAspect::toMap(Store &map) const
{
    map.insert(Constants::QML_MAINSCRIPT_KEY, m_scriptFile);
}

void QmlMainFileAspect::fromMap(const Store &map)
{
    m_scriptFile = map.value(Constants::QML_MAINSCRIPT_KEY, M_CURRENT_FILE).toString();

    if (m_scriptFile == M_CURRENT_FILE)
        setScriptSource(FileInEditor);
    else if (m_scriptFile.isEmpty())
        setScriptSource(FileInProjectFile);
    else
        setScriptSource(FileInSettings, m_scriptFile);
}

void QmlMainFileAspect::updateFileComboBox()
{
    const FilePath projectDir = m_target->project()->projectDirectory();

    if (mainScriptSource() == FileInProjectFile) {
        const FilePath mainScriptInFilePath = projectDir.relativeChildPath(mainScript());
        m_fileListModel.clear();
        m_fileListModel.appendRow(new QStandardItem(mainScriptInFilePath.toString()));
        if (m_fileListCombo)
            m_fileListCombo->setEnabled(false);
        return;
    }

    if (m_fileListCombo)
        m_fileListCombo->setEnabled(true);
    m_fileListModel.clear();
    m_fileListModel.appendRow(new QStandardItem(CURRENT_FILE));
    QModelIndex currentIndex;

    FilePaths sortedFiles = m_target->project()->files(Project::SourceFiles);

    // make paths relative to project directory
    FilePaths relativeFiles;
    for (const FilePath &fn : std::as_const(sortedFiles))
        relativeFiles += projectDir.relativeChildPath(fn);
    sortedFiles = relativeFiles;

    std::stable_sort(sortedFiles.begin(), sortedFiles.end(), caseInsensitiveLessThan);

    FilePath mainScriptPath;
    if (mainScriptSource() != FileInEditor)
        mainScriptPath = projectDir.relativeChildPath(mainScript());

    for (const FilePath &fn : std::as_const(sortedFiles)) {
        if (fn.suffixView() != u"qml")
            continue;

        auto item = new QStandardItem(fn.toString());
        m_fileListModel.appendRow(item);

        if (mainScriptPath == fn)
            currentIndex = item->index();
    }

    if (m_fileListCombo) {
        if (currentIndex.isValid())
            m_fileListCombo->setCurrentIndex(currentIndex.row());
        else
            m_fileListCombo->setCurrentIndex(0);
    }
}

QmlMainFileAspect::MainScriptSource QmlMainFileAspect::mainScriptSource() const
{
    if (!qmlBuildSystem()->mainFile().isEmpty())
        return FileInProjectFile;
    if (!m_mainScriptFilename.isEmpty())
        return FileInSettings;
    return FileInEditor;
}

void QmlMainFileAspect::setMainScript(int index)
{
    if (index == 0) {
        setScriptSource(FileInEditor);
    } else {
        const QString path = m_fileListModel.data(m_fileListModel.index(index, 0)).toString();
        setScriptSource(FileInSettings, path);
    }
}

void QmlMainFileAspect::setTarget(ProjectExplorer::Target *target)
{
    m_target = target;
}

void QmlMainFileAspect::setScriptSource(MainScriptSource source, const QString &settingsPath)
{
    if (source == FileInEditor) {
        m_scriptFile = M_CURRENT_FILE;
        m_mainScriptFilename.clear();
    } else if (source == FileInProjectFile) {
        m_scriptFile.clear();
        m_mainScriptFilename.clear();
    } else { // FileInSettings
        m_scriptFile = settingsPath;
        m_mainScriptFilename = m_target->project()->projectDirectory() / m_scriptFile;
    }

    emit changed();
    updateFileComboBox();
}

/**
  Returns absolute path to main script file.
  */
FilePath QmlMainFileAspect::mainScript() const
{
    if (!qmlBuildSystem()->mainFile().isEmpty()) {
        const FilePath pathInProject = qmlBuildSystem()->mainFilePath();
        return qmlBuildSystem()->canonicalProjectDir().resolvePath(pathInProject);
    }

    if (!m_mainScriptFilename.isEmpty())
        return m_mainScriptFilename;

    return m_currentFileFilename;
}

FilePath QmlMainFileAspect::currentFile() const
{
    return m_currentFileFilename;
}

void QmlMainFileAspect::changeCurrentFile(Core::IEditor *editor)
{
    if (!editor)
        editor = EditorManager::currentEditor();

    if (editor)
        m_currentFileFilename = editor->document()->filePath();

    emit changed();
}

bool QmlMainFileAspect::isQmlFilePresent()
{
    bool qmlFileFound = false;
    if (mainScriptSource() == FileInEditor && !mainScript().isEmpty()) {
        IDocument *document = EditorManager::currentDocument();
        const MimeType mainScriptMimeType = mimeTypeForFile(mainScript());
        if (document) {
            m_currentFileFilename = document->filePath();
            if (mainScriptMimeType.matchesName(ProjectExplorer::Constants::QML_MIMETYPE)
                    || mainScriptMimeType.matchesName(ProjectExplorer::Constants::QMLUI_MIMETYPE)) {
                qmlFileFound = true;
            }
        }
        if (!document
                || mainScriptMimeType.matchesName(QmlJSTools::Constants::QMLPROJECT_MIMETYPE)) {
            // find a qml file with lowercase filename. This is slow, but only done
            // in initialization/other border cases.
            const FilePaths files = m_target->project()->files(Project::SourceFiles);
            for (const FilePath &filename : files) {
                if (!filename.isEmpty() && filename.baseName().at(0).isLower()) {
                    const MimeType type = mimeTypeForFile(filename);
                    if (type.matchesName(ProjectExplorer::Constants::QML_MIMETYPE)
                            || type.matchesName(ProjectExplorer::Constants::QMLUI_MIMETYPE)) {
                        m_currentFileFilename = filename;
                        qmlFileFound = true;
                        break;
                    }
                }
            }
        }
    } else { // use default one
        qmlFileFound = !mainScript().isEmpty();
    }
    return qmlFileFound;
}

QmlBuildSystem *QmlMainFileAspect::qmlBuildSystem() const
{
    return static_cast<QmlBuildSystem *>(m_target->buildSystem());
}

} // QmlProjectManager
