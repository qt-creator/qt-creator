// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlmainfileaspect.h"

#include "qmlproject.h"
#include "qmlprojectmanagerconstants.h"

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
const char CURRENT_FILE[]  = QT_TRANSLATE_NOOP("QmlManager", "<Current File>");

static bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
{
    return s1.toLower() < s2.toLower();
}

QmlMainFileAspect::QmlMainFileAspect(Target *target)
    : m_target(target)
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

void QmlMainFileAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_ASSERT(!m_fileListCombo, delete m_fileListCombo);
    m_fileListCombo = new QComboBox;
    m_fileListCombo->setModel(&m_fileListModel);

    updateFileComboBox();

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::fileListChanged,
            this, &QmlMainFileAspect::updateFileComboBox);
    connect(m_fileListCombo, &QComboBox::activated, this, &QmlMainFileAspect::setMainScript);

    builder.addItems({tr("Main QML file:"), m_fileListCombo.data()});
}

void QmlMainFileAspect::toMap(QVariantMap &map) const
{
    map.insert(Constants::QML_MAINSCRIPT_KEY, m_scriptFile);
}

void QmlMainFileAspect::fromMap(const QVariantMap &map)
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
    QDir projectDir(m_target->project()->projectDirectory().toString());

    if (mainScriptSource() == FileInProjectFile) {
        const QString mainScriptInFilePath = projectDir.relativeFilePath(mainScript());
        m_fileListModel.clear();
        m_fileListModel.appendRow(new QStandardItem(mainScriptInFilePath));
        if (m_fileListCombo)
            m_fileListCombo->setEnabled(false);
        return;
    }

    if (m_fileListCombo)
        m_fileListCombo->setEnabled(true);
    m_fileListModel.clear();
    m_fileListModel.appendRow(new QStandardItem(CURRENT_FILE));
    QModelIndex currentIndex;

    QStringList sortedFiles = Utils::transform(m_target->project()->files(Project::SourceFiles),
                                               &Utils::FilePath::toString);

    // make paths relative to project directory
    QStringList relativeFiles;
    for (const QString &fn : std::as_const(sortedFiles))
        relativeFiles += projectDir.relativeFilePath(fn);
    sortedFiles = relativeFiles;

    std::stable_sort(sortedFiles.begin(), sortedFiles.end(), caseInsensitiveLessThan);

    QString mainScriptPath;
    if (mainScriptSource() != FileInEditor)
        mainScriptPath = projectDir.relativeFilePath(mainScript());

    for (const QString &fn : std::as_const(sortedFiles)) {
        QFileInfo fileInfo(fn);
        if (fileInfo.suffix() != "qml")
            continue;

        auto item = new QStandardItem(fn);
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
        m_mainScriptFilename = m_target->project()->projectDirectory().toString() + '/' + m_scriptFile;
    }

    emit changed();
    updateFileComboBox();
}

/**
  Returns absolute path to main script file.
  */
QString QmlMainFileAspect::mainScript() const
{
    if (!qmlBuildSystem()->mainFile().isEmpty()) {
        const QString pathInProject = qmlBuildSystem()->mainFile();
        if (QFileInfo(pathInProject).isAbsolute())
            return pathInProject;
        else
            return QDir(qmlBuildSystem()->canonicalProjectDir().toString()).absoluteFilePath(pathInProject);
    }

    if (!m_mainScriptFilename.isEmpty())
        return m_mainScriptFilename;

    return m_currentFileFilename;
}

QString QmlMainFileAspect::currentFile() const
{
    return m_currentFileFilename;
}

void QmlMainFileAspect::changeCurrentFile(Core::IEditor *editor)
{
    if (!editor)
        editor = EditorManager::currentEditor();

    if (editor)
        m_currentFileFilename = editor->document()->filePath().toString();

    emit changed();
}

bool QmlMainFileAspect::isQmlFilePresent()
{
    bool qmlFileFound = false;
    if (mainScriptSource() == FileInEditor) {
        IDocument *document = EditorManager::currentDocument();
        const MimeType mainScriptMimeType = mimeTypeForFile(mainScript());
        if (document) {
            m_currentFileFilename = document->filePath().toString();
            if (mainScriptMimeType.matchesName(ProjectExplorer::Constants::QML_MIMETYPE)
                    || mainScriptMimeType.matchesName(ProjectExplorer::Constants::QMLUI_MIMETYPE)) {
                qmlFileFound = true;
            }
        }
        if (!document
                || mainScriptMimeType.matchesName(QmlJSTools::Constants::QMLPROJECT_MIMETYPE)) {
            // find a qml file with lowercase filename. This is slow, but only done
            // in initialization/other border cases.
            const auto files = m_target->project()->files(Project::SourceFiles);
            for (const FilePath &filename : files) {
                if (!filename.isEmpty() && filename.baseName().at(0).isLower()) {
                    const MimeType type = mimeTypeForFile(filename);
                    if (type.matchesName(ProjectExplorer::Constants::QML_MIMETYPE)
                            || type.matchesName(ProjectExplorer::Constants::QMLUI_MIMETYPE)) {
                        m_currentFileFilename = filename.toString();
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
} // namespace QmlProjectManager
