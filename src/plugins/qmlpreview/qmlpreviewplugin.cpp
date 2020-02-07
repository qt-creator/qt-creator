/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qmlpreviewplugin.h"
#include "qmlpreviewruncontrol.h"

#ifdef WITH_TESTS
#include "tests/qmlpreviewclient_test.h"
#include "tests/qmlpreviewplugin_test.h"
#endif

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <QAction>

using namespace ProjectExplorer;

namespace QmlPreview {
namespace Internal {

class QmlPreviewParser : public QObject
{
    Q_OBJECT
public:
    QmlPreviewParser();
    void parse(const QString &name, const QByteArray &contents,
               QmlJS::Dialect::Enum dialect);

signals:
    void success(const QString &changedFile, const QByteArray &contents);
    void failure();
};

static QByteArray defaultFileLoader(const QString &filename, bool *success)
{
    if (Core::DocumentModel::Entry *entry
            = Core::DocumentModel::entryForFilePath(Utils::FilePath::fromString(filename))) {
        if (!entry->isSuspended) {
            *success = true;
            return entry->document->contents();
        }
    }

    QFile file(filename);
    *success = file.open(QIODevice::ReadOnly);
    return (*success) ? file.readAll() : QByteArray();
}

static bool defaultFileClassifier(const QString &filename)
{
    // We cannot dynamically load changes in qtquickcontrols2.conf
    return !filename.endsWith("qtquickcontrols2.conf");
}

static void defaultFpsHandler(quint16 frames[8])
{
    Core::MessageManager::write(QString::fromLatin1("QML preview: %1 fps").arg(frames[0]));
}

class QmlPreviewPluginPrivate : public QObject
{
public:
    explicit QmlPreviewPluginPrivate(QmlPreviewPlugin *parent);

    void previewCurrentFile();
    void onEditorChanged(Core::IEditor *editor);
    void onEditorAboutToClose(Core::IEditor *editor);
    void setDirty();
    void addPreview(ProjectExplorer::RunControl *preview);
    void removePreview(ProjectExplorer::RunControl *preview);
    void attachToEditor();
    void checkEditor();
    void checkFile(const QString &fileName);
    void triggerPreview(const QString &changedFile, const QByteArray &contents);

    QString previewedFile() const;
    void setPreviewedFile(const QString &previewedFile);
    QmlPreviewRunControlList runningPreviews() const;

    QmlPreviewFileClassifier fileClassifier() const;
    void setFileClassifier(QmlPreviewFileClassifier fileClassifer);

    float zoomFactor() const;
    void setZoomFactor(float zoomFactor);

    QmlPreview::QmlPreviewFpsHandler fpsHandler() const;
    void setFpsHandler(QmlPreview::QmlPreviewFpsHandler fpsHandler);

    QString locale() const;
    void setLocale(const QString &locale);

    QmlPreviewPlugin *q = nullptr;
    QThread m_parseThread;
    QString m_previewedFile;
    QmlPreviewFileLoader m_fileLoader = nullptr;
    Core::IEditor *m_lastEditor = nullptr;
    QmlPreviewRunControlList m_runningPreviews;
    bool m_dirty = false;
    QmlPreview::QmlPreviewFileClassifier m_fileClassifer = nullptr;
    float m_zoomFactor = -1.0;
    QmlPreview::QmlPreviewFpsHandler m_fpsHandler = nullptr;
    QString m_locale;

    RunWorkerFactory localRunWorkerFactory{
        RunWorkerFactory::make<LocalQmlPreviewSupport>(),
        {Constants::QML_PREVIEW_RUN_MODE},
        {}, // All runconfig.
        {Constants::DESKTOP_DEVICE_TYPE}
    };

    RunWorkerFactory runWorkerFactory{
        [this](RunControl *runControl) {
            QmlPreviewRunner *runner = new QmlPreviewRunner(runControl, m_fileLoader, m_fileClassifer,
                                                            m_fpsHandler, m_zoomFactor, m_locale);
            connect(q, &QmlPreviewPlugin::updatePreviews,
                    runner, &QmlPreviewRunner::loadFile);
            connect(q, &QmlPreviewPlugin::rerunPreviews,
                    runner, &QmlPreviewRunner::rerun);
            connect(runner, &QmlPreviewRunner::ready,
                    this, &QmlPreviewPluginPrivate::previewCurrentFile);
            connect(q, &QmlPreviewPlugin::zoomFactorChanged,
                    runner, &QmlPreviewRunner::zoom);
            connect(q, &QmlPreviewPlugin::localeChanged,
                    runner, &QmlPreviewRunner::language);

            connect(runner, &RunWorker::started, this, [this, runControl] {
                addPreview(runControl);
            });
            connect(runner, &RunWorker::stopped, this, [this, runControl] {
                removePreview(runControl);
            });

            return runner;
        },
        {Constants::QML_PREVIEW_RUNNER}
    };
};

QmlPreviewPluginPrivate::QmlPreviewPluginPrivate(QmlPreviewPlugin *parent)
    : q(parent)
{
    m_fileLoader = &defaultFileLoader;
    m_fileClassifer = &defaultFileClassifier;
    m_fpsHandler = &defaultFpsHandler;

    Core::ActionContainer *menu = Core::ActionManager::actionContainer(
                Constants::M_BUILDPROJECT);
    QAction *action = new QAction(QmlPreviewPlugin::tr("QML Preview"), this);
    action->setToolTip(QLatin1String("Preview changes to QML code live in your application."));
    action->setEnabled(SessionManager::startupProject() != nullptr);
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged, action,
            &QAction::setEnabled);
    connect(action, &QAction::triggered, this, []() {
        ProjectExplorerPlugin::runStartupProject(Constants::QML_PREVIEW_RUN_MODE);
    });
    menu->addAction(Core::ActionManager::registerAction(action, "QmlPreview.Internal"),
                    Constants::G_BUILD_RUN);

    Core::Context projectTreeContext(Constants::C_PROJECT_TREE);
    menu = Core::ActionManager::actionContainer(Constants::M_FILECONTEXT);
    action = new QAction(QmlPreviewPlugin::tr("Preview File"), this);
    action->setEnabled(false);
    connect(q, &QmlPreviewPlugin::runningPreviewsChanged,
            action, [action](const QmlPreviewRunControlList &previews) {
        action->setEnabled(!previews.isEmpty());
    });
    connect(action, &QAction::triggered, this, &QmlPreviewPluginPrivate::previewCurrentFile);
    menu->addAction(Core::ActionManager::registerAction(action, "QmlPreview.Preview",
                                                        projectTreeContext),
                    Constants::G_FILE_OTHER);
    action->setVisible(false);
    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged, action, [action]() {
        const Node *node = ProjectTree::currentNode();
        const FileNode *fileNode = node ? node->asFileNode() : nullptr;
        action->setVisible(fileNode ? fileNode->fileType() == FileType::QML : false);
    });

    m_parseThread.start();
    QmlPreviewParser *parser = new QmlPreviewParser;
    parser->moveToThread(&m_parseThread);
    connect(&m_parseThread, &QThread::finished, parser, &QObject::deleteLater);
    connect(q, &QmlPreviewPlugin::checkDocument, parser, &QmlPreviewParser::parse);
    connect(q, &QmlPreviewPlugin::previewedFileChanged, this, &QmlPreviewPluginPrivate::checkFile);
    connect(parser, &QmlPreviewParser::success, this, &QmlPreviewPluginPrivate::triggerPreview);

    attachToEditor();
}

QmlPreviewPlugin::~QmlPreviewPlugin()
{
    delete d;
}

bool QmlPreviewPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    d = new QmlPreviewPluginPrivate(this);

    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag QmlPreviewPlugin::aboutToShutdown()
{
    d->m_parseThread.quit();
    d->m_parseThread.wait();
    return SynchronousShutdown;
}

QVector<QObject *> QmlPreviewPlugin::createTestObjects() const
{
    QVector<QObject *> tests;
#ifdef WITH_TESTS
    tests.append(new QmlPreviewClientTest);
    tests.append(new QmlPreviewPluginTest);
#endif
    return tests;
}

QString QmlPreviewPlugin::previewedFile() const
{
    return d->m_previewedFile;
}

void QmlPreviewPlugin::setPreviewedFile(const QString &previewedFile)
{
    if (d->m_previewedFile == previewedFile)
        return;

    d->m_previewedFile = previewedFile;
    emit previewedFileChanged(d->m_previewedFile);
}

QmlPreviewRunControlList QmlPreviewPlugin::runningPreviews() const
{
    return d->m_runningPreviews;
}

QmlPreviewFileLoader QmlPreviewPlugin::fileLoader() const
{
    return d->m_fileLoader;
}

QmlPreviewFileClassifier QmlPreviewPlugin::fileClassifier() const
{
    return d->m_fileClassifer;
}

void QmlPreviewPlugin::setFileClassifier(QmlPreviewFileClassifier fileClassifer)
{
    if (d->m_fileClassifer == fileClassifer)
        return;

    d->m_fileClassifer = fileClassifer;
    emit fileClassifierChanged(d->m_fileClassifer);
}

float QmlPreviewPlugin::zoomFactor() const
{
    return d->m_zoomFactor;
}

void QmlPreviewPlugin::setZoomFactor(float zoomFactor)
{
    if (d->m_zoomFactor == zoomFactor)
        return;

    d->m_zoomFactor = zoomFactor;
    emit zoomFactorChanged(d->m_zoomFactor);
}

QmlPreviewFpsHandler QmlPreviewPlugin::fpsHandler() const
{
    return d->m_fpsHandler;
}

void QmlPreviewPlugin::setFpsHandler(QmlPreviewFpsHandler fpsHandler)
{
    if (d->m_fpsHandler == fpsHandler)
        return;

    d->m_fpsHandler = fpsHandler;
    emit fpsHandlerChanged(d->m_fpsHandler);
}

QString QmlPreviewPlugin::locale() const
{
    return d->m_locale;
}

void QmlPreviewPlugin::setLocale(const QString &locale)
{
    if (d->m_locale == locale)
        return;

    d->m_locale = locale;
    emit localeChanged(d->m_locale);
}

void QmlPreviewPlugin::setFileLoader(QmlPreviewFileLoader fileLoader)
{
    if (d->m_fileLoader == fileLoader)
        return;

    d->m_fileLoader = fileLoader;
    emit fileLoaderChanged(d->m_fileLoader);
}

void QmlPreviewPluginPrivate::previewCurrentFile()
{
    const Node *currentNode = ProjectTree::currentNode();
    if (!currentNode || !currentNode->asFileNode()
            || currentNode->asFileNode()->fileType() != FileType::QML)
        return;

    const QString file = currentNode->filePath().toString();
    if (file != m_previewedFile)
        q->setPreviewedFile(file);
    else
        checkFile(file);
}

void QmlPreviewPluginPrivate::onEditorChanged(Core::IEditor *editor)
{
    if (m_lastEditor) {
        Core::IDocument *doc = m_lastEditor->document();
        disconnect(doc, &Core::IDocument::contentsChanged, this, &QmlPreviewPluginPrivate::setDirty);
        if (m_dirty) {
            m_dirty = false;
            checkEditor();
        }
    }

    m_lastEditor = editor;
    if (m_lastEditor) {
        // Handle new editor
        connect(m_lastEditor->document(), &Core::IDocument::contentsChanged,
                this, &QmlPreviewPluginPrivate::setDirty);
    }
}

void QmlPreviewPluginPrivate::onEditorAboutToClose(Core::IEditor *editor)
{
    if (m_lastEditor != editor)
        return;

    // Oh no our editor is going to be closed
    // get the content first
    Core::IDocument *doc = m_lastEditor->document();
    disconnect(doc, &Core::IDocument::contentsChanged, this, &QmlPreviewPluginPrivate::setDirty);
    if (m_dirty) {
        m_dirty = false;
        checkEditor();
    }
    m_lastEditor = nullptr;
}

void QmlPreviewPluginPrivate::setDirty()
{
    m_dirty = true;
    QTimer::singleShot(1000, this, [this](){
        if (m_dirty && m_lastEditor) {
            m_dirty = false;
            checkEditor();
        }
    });
}

void QmlPreviewPluginPrivate::addPreview(ProjectExplorer::RunControl *preview)
{
    m_runningPreviews.append(preview);
    emit q->runningPreviewsChanged(m_runningPreviews);
}

void QmlPreviewPluginPrivate::removePreview(ProjectExplorer::RunControl *preview)
{
    m_runningPreviews.removeOne(preview);
    emit q->runningPreviewsChanged(m_runningPreviews);
}

void QmlPreviewPluginPrivate::attachToEditor()
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::currentEditorChanged,
            this, &QmlPreviewPluginPrivate::onEditorChanged);
    connect(editorManager, &Core::EditorManager::editorAboutToClose,
            this, &QmlPreviewPluginPrivate::onEditorAboutToClose);
}

void QmlPreviewPluginPrivate::checkEditor()
{
    QmlJS::Dialect::Enum dialect = QmlJS::Dialect::AnyLanguage;
    Core::IDocument *doc = m_lastEditor->document();
    const QString mimeType = doc->mimeType();
    if (mimeType == QmlJSTools::Constants::JS_MIMETYPE)
        dialect = QmlJS::Dialect::JavaScript;
    else if (mimeType == QmlJSTools::Constants::JSON_MIMETYPE)
        dialect = QmlJS::Dialect::Json;
    else if (mimeType == QmlJSTools::Constants::QML_MIMETYPE)
        dialect = QmlJS::Dialect::Qml;
//  --- Can we detect those via mime types?
//  else if (mimeType == ???)
//      dialect = QmlJS::Dialect::QmlQtQuick1;
//  else if (mimeType == ???)
//      dialect = QmlJS::Dialect::QmlQtQuick2;
    else if (mimeType == QmlJSTools::Constants::QBS_MIMETYPE)
        dialect = QmlJS::Dialect::QmlQbs;
    else if (mimeType == QmlJSTools::Constants::QMLPROJECT_MIMETYPE)
        dialect = QmlJS::Dialect::QmlProject;
    else if (mimeType == QmlJSTools::Constants::QMLTYPES_MIMETYPE)
        dialect = QmlJS::Dialect::QmlTypeInfo;
    else if (mimeType == QmlJSTools::Constants::QMLUI_MIMETYPE)
        dialect = QmlJS::Dialect::QmlQtQuick2Ui;
    else
        dialect = QmlJS::Dialect::NoLanguage;
    emit q->checkDocument(doc->filePath().toString(), doc->contents(), dialect);
}

void QmlPreviewPluginPrivate::checkFile(const QString &fileName)
{
    if (!m_fileLoader)
        return;

    bool success = false;
    const QByteArray contents = m_fileLoader(fileName, &success);

    if (success) {
        emit q->checkDocument(fileName, contents,
                              QmlJS::ModelManagerInterface::guessLanguageOfFile(fileName).dialect());
    }
}

void QmlPreviewPluginPrivate::triggerPreview(const QString &changedFile, const QByteArray &contents)
{
    if (m_previewedFile.isEmpty())
        previewCurrentFile();
    else
        emit q->updatePreviews(m_previewedFile, changedFile, contents);
}

QmlPreviewParser::QmlPreviewParser()
{
    static const int dialectMeta = qRegisterMetaType<QmlJS::Dialect::Enum>();
    Q_UNUSED(dialectMeta)
}

void QmlPreviewParser::parse(const QString &name, const QByteArray &contents,
                             QmlJS::Dialect::Enum dialect)
{
    if (!QmlJS::Dialect(dialect).isQmlLikeOrJsLanguage()) {
        emit success(name, contents);
        return;
    }

    QmlJS::Document::MutablePtr qmljsDoc = QmlJS::Document::create(name, dialect);
    qmljsDoc->setSource(QString::fromUtf8(contents));
    if (qmljsDoc->parse())
        emit success(name, contents);
    else
        emit failure();
}

} // namespace Internal
} // namespace QmlPreview

#include <qmlpreviewplugin.moc>
