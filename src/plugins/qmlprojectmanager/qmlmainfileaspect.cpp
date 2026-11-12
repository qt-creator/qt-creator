// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlmainfileaspect.h"

#include "buildsystem/qmlbuildsystem.h"
#include "qmlprojectconstants.h"
#include "qmlprojectmanagertr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <projectexplorer/projectexplorer.h>

#include <utils/layoutbuilder.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>

#include <QComboBox>

#ifdef WITH_TESTS
#include "qmlproject.h"

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <utils/temporarydirectory.h>

#include <QTest>
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace QmlProjectManager {

const char M_CURRENT_FILE[] = "CurrentFile";
const char CURRENT_FILE[]  = QT_TRANSLATE_NOOP("QtC::QmlProjectManager", "<Current File>");

static bool caseInsensitiveLessThan(const FilePath &s1, const FilePath &s2)
{
    return s1.toUrlishString().toCaseFolded() < s2.toUrlishString().toCaseFolded();
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
    connect(ProjectExplorerPlugin::instance(),
            &ProjectExplorerPlugin::fileListChanged,
            this,
            &QmlMainFileAspect::updateFileComboBox);
}

QmlMainFileAspect::~QmlMainFileAspect() = default;

void QmlMainFileAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto fileListCombo = new QComboBox;
    fileListCombo->setModel(&m_fileListModel);
    const auto showSelection = [this, fileListCombo] {
        fileListCombo->setEnabled(m_fileListEnabled);
        fileListCombo->setCurrentIndex(m_currentFileRow);
    };
    connect(this, &QmlMainFileAspect::fileSelectionChanged, fileListCombo, showSelection);

    updateFileComboBox();
    showSelection();
    connect(fileListCombo, &QComboBox::activated, this, &QmlMainFileAspect::setMainScript);

    parent.addItems({Tr::tr("Main QML file:"), fileListCombo});
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
    auto buildSystem = qmlBuildSystem();
    QTC_ASSERT(buildSystem, return);
    const FilePath projectDir = buildSystem->projectDirectory();

    if (mainScriptSource() == FileInProjectFile) {
        const QString mainScriptInFilePath = mainScript().relativePathFromDir(projectDir);
        m_fileListModel.clear();
        m_fileListModel.appendRow(new QStandardItem(mainScriptInFilePath));
        m_fileListEnabled = false;
        m_currentFileRow = 0;
        emit fileSelectionChanged(QPrivateSignal());
        return;
    }

    m_fileListEnabled = true;
    m_fileListModel.clear();
    m_fileListModel.appendRow(new QStandardItem(CURRENT_FILE));
    QModelIndex currentIndex;

    FilePaths sortedFiles = buildSystem->project()->files(Project::SourceFiles);

    // make paths relative to project directory
    FilePaths relativeFiles;
    for (const FilePath &fn : std::as_const(sortedFiles))
        relativeFiles += fn.relativeChildPath(projectDir);
    sortedFiles = relativeFiles;

    std::stable_sort(sortedFiles.begin(), sortedFiles.end(), caseInsensitiveLessThan);

    FilePath mainScriptPath;
    if (mainScriptSource() != FileInEditor)
        mainScriptPath = mainScript().relativeChildPath(projectDir);

    for (const FilePath &fn : std::as_const(sortedFiles)) {
        if (fn.suffixView() != u"qml")
            continue;

        auto item = new QStandardItem(fn.toUrlishString());
        m_fileListModel.appendRow(item);

        if (mainScriptPath == fn)
            currentIndex = item->index();
    }

    m_currentFileRow = currentIndex.isValid() ? currentIndex.row() : 0;
    emit fileSelectionChanged(QPrivateSignal());
}

QmlMainFileAspect::MainScriptSource QmlMainFileAspect::mainScriptSource() const
{
    QTC_ASSERT(qmlBuildSystem(), return FileInEditor);
    if (!qmlBuildSystem()->mainFile().isEmpty())
        return FileInProjectFile;
    if (!m_mainScriptFilename.isEmpty())
        return FileInSettings;
    return FileInEditor;
}

void QmlMainFileAspect::setMainScript(int index)
{
    m_currentFileRow = index;
    emit fileSelectionChanged(QPrivateSignal());
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
        if (QTC_GUARD(qmlBuildSystem()))
            m_mainScriptFilename = qmlBuildSystem()->projectDirectory() / m_scriptFile;
    }

    emit changed();
    updateFileComboBox();
}

/**
  Returns absolute path to main script file.
  */
FilePath QmlMainFileAspect::mainScript() const
{
    if (QTC_GUARD(qmlBuildSystem()) && !qmlBuildSystem()->mainFile().isEmpty()) {
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
        using namespace Utils::Constants;
        IDocument *document = EditorManager::currentDocument();
        const MimeType mainScriptMimeType = mimeTypeForFile(mainScript());
        if (document) {
            m_currentFileFilename = document->filePath();
            if (mainScriptMimeType.matchesName(QML_MIMETYPE)
                    || mainScriptMimeType.matchesName(QMLUI_MIMETYPE)) {
                qmlFileFound = true;
            }
        }
        if (!document
                || mainScriptMimeType.matchesName(QMLPROJECT_MIMETYPE)) {
            // find a qml file with lowercase filename. This is slow, but only done
            // in initialization/other border cases.

            QTC_ASSERT(qmlBuildSystem(), return qmlFileFound);
            const FilePaths files = qmlBuildSystem()->project()->files(Project::SourceFiles);
            for (const FilePath &filename : files) {
                if (!filename.isEmpty() && filename.baseName().at(0).isLower()) {
                    const MimeType type = mimeTypeForFile(filename);
                    if (type.matchesName(QML_MIMETYPE) || type.matchesName(QMLUI_MIMETYPE)) {
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
    RunConfiguration *runConfig = qobject_cast<RunConfiguration *>(container());
    QTC_ASSERT(runConfig, return nullptr);
    return qobject_cast<QmlBuildSystem *>(runConfig->buildSystem());
}

#ifdef WITH_TESTS
namespace Internal {

class QmlMainFileAspectTest final : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_tempDir = std::make_unique<TemporaryDirectory>("qmlmainfileaspect");
        // The project lists its files canonically, so /var and /private/var on macOS differ.
        m_dir = m_tempDir->path().canonicalPath();
        const FilePath projectFile = m_dir / "test.qmlproject";
        QVERIFY(projectFile.writeFileContents(
            "import QmlProject 1.1\nProject {\n    QmlFiles { directory: \".\" }\n}\n"));
        QVERIFY((m_dir / "first.qml").writeFileContents("import QtQuick\nItem {}\n"));
        QVERIFY((m_dir / "second.qml").writeFileContents("import QtQuick\nItem {}\n"));

        m_kit = KitManager::registerKit([](Kit *k) {
            k->setUnexpandedDisplayName("QmlMainFileAspectTest Kit");
            RunDeviceTypeKitAspect::setDeviceTypeId(k, ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
        });
        QVERIFY(m_kit);
        m_project = std::make_unique<QmlProject>(projectFile);
        Target * const target = m_project->addTargetForKit(m_kit);
        QVERIFY(target);
        QTRY_VERIFY(target->activeRunConfiguration());
        m_aspect = target->activeRunConfiguration()->aspect<QmlMainFileAspect>();
        QVERIFY(m_aspect);
        QTRY_VERIFY(m_aspect->qmlBuildSystem()->hasParsingData());
    }

    void cleanup()
    {
        m_aspect = nullptr;
        m_project.reset();
        if (m_kit)
            KitManager::deregisterKit(m_kit);
        m_kit = nullptr;
        m_tempDir.reset();
    }

    void testOffersTheQmlFilesOfTheProject()
    {
        const std::unique_ptr<QWidget> widget(Layouting::Column{*m_aspect}.emerge());
        QComboBox *comboBox = widget->findChild<QComboBox *>();
        QVERIFY(comboBox);
        QCOMPARE(comboBox->count(), 3);
        QCOMPARE(comboBox->itemText(1), "first.qml");
        QCOMPARE(comboBox->itemText(2), "second.qml");
    }

    void testUpdatesEveryComboBox()
    {
        const std::unique_ptr<QWidget> first(Layouting::Column{*m_aspect}.emerge());
        const std::unique_ptr<QWidget> second(Layouting::Column{*m_aspect}.emerge());
        QComboBox *firstBox = first->findChild<QComboBox *>();
        QComboBox *secondBox = second->findChild<QComboBox *>();
        QVERIFY(firstBox);
        QVERIFY(secondBox);

        firstBox->setCurrentIndex(2);
        emit firstBox->activated(2);

        QCOMPARE(m_aspect->mainScript(), m_dir / "second.qml");
        QCOMPARE(secondBox->currentIndex(), 2);

        m_aspect->updateFileComboBox();
        QCOMPARE(firstBox->currentIndex(), 2);
        QCOMPARE(secondBox->currentIndex(), 2);
    }

private:
    std::unique_ptr<TemporaryDirectory> m_tempDir;
    FilePath m_dir;
    Kit *m_kit = nullptr;
    std::unique_ptr<QmlProject> m_project;
    QmlMainFileAspect *m_aspect = nullptr;
};

QObject *createQmlMainFileAspectTest()
{
    return new QmlMainFileAspectTest;
}

} // namespace Internal
#endif // WITH_TESTS

} // QmlProjectManager

#ifdef WITH_TESTS
#include <qmlmainfileaspect.moc>
#endif
