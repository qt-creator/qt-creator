// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewplugin.h"

#include "qmlpreviewruncontrol.h"
#include "qmlpreviewtr.h"

#ifdef WITH_TESTS
#include "tests/qmlpreviewclient_test.h"
#include "tests/qmlpreviewplugin_test.h"
#endif

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <qmlprojectmanager/qmlmultilanguageaspect.h>

#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/baseqtversion.h>

#include <android/androidconstants.h>

#include <QAction>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>

using namespace ProjectExplorer;

namespace QmlPreview {

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
    Core::MessageManager::writeSilently(QString::fromLatin1("QML preview: %1 fps").arg(frames[0]));
}

static std::unique_ptr<QmlDebugTranslationClient> defaultCreateDebugTranslationClientMethod(QmlDebug::QmlDebugConnection *connection)
{
    auto client = std::make_unique<QmlPreview::QmlDebugTranslationClient>(connection);
    return client;
}

static void defaultRefreshTranslationFunction()
{}

class QmlPreviewPluginPrivate : public QObject
{
public:
    explicit QmlPreviewPluginPrivate(QmlPreviewPlugin *parent);

    void onEditorChanged(Core::IEditor *editor);
    void onEditorAboutToClose(Core::IEditor *editor);
    void setDirty();
    void attachToEditor();
    void checkEditor();
    void checkFile(const QString &fileName);
    void triggerPreview(const QString &changedFile, const QByteArray &contents);

    QString previewedFile() const;
    void setPreviewedFile(const QString &previewedFile);
    QmlPreviewRunControlList runningPreviews() const;

    QmlPreviewFileClassifier fileClassifier() const;
    void setFileClassifier(QmlPreviewFileClassifier fileClassifier);

    float zoomFactor() const;
    void setZoomFactor(float zoomFactor);

    QmlPreview::QmlPreviewFpsHandler fpsHandler() const;
    void setFpsHandler(QmlPreview::QmlPreviewFpsHandler fpsHandler);

    QString locale() const;
    void setLocale(const QString &locale);

    QmlPreviewPlugin *q = nullptr;
    QThread m_parseThread;
    QString m_previewedFile;
    QPointer<Core::IEditor> m_lastEditor;
    QmlPreviewRunControlList m_runningPreviews;
    bool m_dirty = false;
    QString m_localeIsoCode;

    LocalQmlPreviewSupportFactory localRunWorkerFactory;

    QmlPreviewRunnerSetting m_settings;
    QmlPreviewRunWorkerFactory runWorkerFactory;
};

QmlPreviewPluginPrivate::QmlPreviewPluginPrivate(QmlPreviewPlugin *parent)
    : q(parent), runWorkerFactory(parent, &m_settings)
{
    m_settings.fileLoader = &defaultFileLoader;
    m_settings.fileClassifier = &defaultFileClassifier;
    m_settings.fpsHandler = &defaultFpsHandler;
    m_settings.createDebugTranslationClientMethod = &defaultCreateDebugTranslationClientMethod;
    m_settings.refreshTranslationsFunction = &defaultRefreshTranslationFunction;

    Core::ActionContainer *menu = Core::ActionManager::actionContainer(
                Constants::M_BUILDPROJECT);
    QAction *action = new QAction(Tr::tr("QML Preview"), this);
    action->setToolTip(Tr::tr("Preview changes to QML code live in your application."));
    action->setEnabled(ProjectManager::startupProject() != nullptr);
    connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged, action,
            &QAction::setEnabled);
    connect(action, &QAction::triggered, this, [this]() {
        if (auto multiLanguageAspect = QmlProjectManager::QmlMultiLanguageAspect::current())
            m_localeIsoCode = multiLanguageAspect->currentLocale();
        bool skipDeploy = false;
        const Kit *kit = ProjectManager::startupTarget()->kit();
        if (ProjectManager::startupTarget() && kit)
            skipDeploy = kit->supportedPlatforms().contains(Android::Constants::ANDROID_DEVICE_TYPE)
                || DeviceTypeKitAspect::deviceTypeId(kit) == Android::Constants::ANDROID_DEVICE_TYPE;
        ProjectExplorerPlugin::runStartupProject(Constants::QML_PREVIEW_RUN_MODE, skipDeploy);
    });
    menu->addAction(
        Core::ActionManager::registerAction(action, "QmlPreview.RunPreview"),
        Constants::G_BUILD_RUN);

    menu = Core::ActionManager::actionContainer(Constants::M_FILECONTEXT);
    action = new QAction(Tr::tr("Preview File"), this);
    connect(action, &QAction::triggered, q, &QmlPreviewPlugin::previewCurrentFile);
    menu->addAction(
        Core::ActionManager::registerAction(action, "QmlPreview.PreviewFile",  Core::Context(Constants::C_PROJECT_TREE)),
        Constants::G_FILE_OTHER);
    action->setVisible(false);
    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged, action, [action](Node *node) {
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

void QmlPreviewPlugin::initialize()
{
    d = new QmlPreviewPluginPrivate(this);

#ifdef WITH_TESTS
    addTest<QmlPreviewClientTest>();
    addTest<QmlPreviewPluginTest>();
#endif
}

ExtensionSystem::IPlugin::ShutdownFlag QmlPreviewPlugin::aboutToShutdown()
{
    d->m_parseThread.quit();
    d->m_parseThread.wait();
    return SynchronousShutdown;
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

void QmlPreviewPlugin::stopAllPreviews()
{
    for (auto &runningPreview : d->m_runningPreviews)
        runningPreview->initiateStop();
}

QmlPreviewFileLoader QmlPreviewPlugin::fileLoader() const
{
    return d->m_settings.fileLoader;
}

QmlPreviewFileClassifier QmlPreviewPlugin::fileClassifier() const
{
    return d->m_settings.fileClassifier;
}

void QmlPreviewPlugin::setFileClassifier(QmlPreviewFileClassifier fileClassifier)
{
    if (d->m_settings.fileClassifier == fileClassifier)
        return;

    d->m_settings.fileClassifier = fileClassifier;
    emit fileClassifierChanged(d->m_settings.fileClassifier);
}

float QmlPreviewPlugin::zoomFactor() const
{
    return d->m_settings.zoomFactor;
}

void QmlPreviewPlugin::setZoomFactor(float zoomFactor)
{
    if (d->m_settings.zoomFactor == zoomFactor)
        return;

    d->m_settings.zoomFactor = zoomFactor;
    emit zoomFactorChanged(d->m_settings.zoomFactor);
}

QmlPreviewFpsHandler QmlPreviewPlugin::fpsHandler() const
{
    return d->m_settings.fpsHandler;
}

void QmlPreviewPlugin::setFpsHandler(QmlPreviewFpsHandler fpsHandler)
{
    if (d->m_settings.fpsHandler == fpsHandler)
        return;

    d->m_settings.fpsHandler = fpsHandler;
    emit fpsHandlerChanged(d->m_settings.fpsHandler);
}

QString QmlPreviewPlugin::localeIsoCode() const
{
    return d->m_localeIsoCode;
}

void QmlPreviewPlugin::setLocaleIsoCode(const QString &localeIsoCode)
{
    if (auto multiLanguageAspect = QmlProjectManager::QmlMultiLanguageAspect::current())
        multiLanguageAspect->setCurrentLocale(localeIsoCode);
    if (d->m_localeIsoCode == localeIsoCode)
        return;

    d->m_localeIsoCode = localeIsoCode;
    emit localeIsoCodeChanged(d->m_localeIsoCode);
}

void QmlPreviewPlugin::setQmlDebugTranslationClientCreator(QmlDebugTranslationClientFactoryFunction creator)
{
    d->m_settings.createDebugTranslationClientMethod = creator;
}

void QmlPreviewPlugin::setRefreshTranslationsFunction(QmlPreviewRefreshTranslationFunction refreshTranslationsFunction)
{
    d->m_settings.refreshTranslationsFunction = refreshTranslationsFunction;
}

void QmlPreviewPlugin::setFileLoader(QmlPreviewFileLoader fileLoader)
{
    if (d->m_settings.fileLoader == fileLoader)
        return;

    d->m_settings.fileLoader = fileLoader;
    emit fileLoaderChanged(d->m_settings.fileLoader);
}

void QmlPreviewPlugin::previewCurrentFile()
{
    const Node *currentNode = ProjectTree::currentNode();
    if (!currentNode || !currentNode->asFileNode()
            || currentNode->asFileNode()->fileType() != FileType::QML)
        return;

    if (runningPreviews().isEmpty())
        QMessageBox::warning(Core::ICore::dialogParent(), Tr::tr("QML Preview Not Running"),
                             Tr::tr("Start the QML Preview for the project before selecting "
                                    "a specific file for preview."));

    const QString file = currentNode->filePath().toString();
    if (file != d->m_previewedFile)
        setPreviewedFile(file);
    else
        d->checkFile(file);
}

void QmlPreviewPluginPrivate::onEditorChanged(Core::IEditor *editor)
{
    if (m_lastEditor && m_lastEditor->document()) {
        disconnect(m_lastEditor->document(), &Core::IDocument::contentsChanged,
                   this, &QmlPreviewPluginPrivate::setDirty);
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

void QmlPreviewPlugin::addPreview(RunControl *preview)
{
    d->m_runningPreviews.append(preview);
    emit runningPreviewsChanged(d->m_runningPreviews);
}

void QmlPreviewPlugin::removePreview(RunControl *preview)
{
    d->m_runningPreviews.removeOne(preview);
    emit runningPreviewsChanged(d->m_runningPreviews);
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
    if (m_runningPreviews.isEmpty())
        return;
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
    if (!m_settings.fileLoader)
        return;

    bool success = false;
    const QByteArray contents = m_settings.fileLoader(fileName, &success);

    if (success) {
        emit q->checkDocument(fileName,
                              contents,
                              QmlJS::ModelManagerInterface::guessLanguageOfFile(
                                  Utils::FilePath::fromUserInput(fileName))
                                  .dialect());
    }
}

void QmlPreviewPluginPrivate::triggerPreview(const QString &changedFile, const QByteArray &contents)
{
    if (m_previewedFile.isEmpty())
        q->previewCurrentFile();
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

    QmlJS::Document::MutablePtr qmljsDoc = QmlJS::Document::create(Utils::FilePath::fromString(name),
                                                                   dialect);
    qmljsDoc->setSource(QString::fromUtf8(contents));
    if (qmljsDoc->parse())
        emit success(name, contents);
    else
        emit failure();
}

} // namespace QmlPreview

#include <qmlpreviewplugin.moc>
