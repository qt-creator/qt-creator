// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pipsupport.h"
#include "pythonbuildconfiguration.h"
#include "pythonconstants.h"
#include "pythoneditor.h"
#include "pythonhighlighter.h"
#include "pythonkitaspect.h"
#include "pythonproject.h"
#include "pythonrunconfiguration.h"
#include "pythonsettings.h"
#include "pythontr.h"
#include "pythonwizardpage.h"

#include <debugger/debuggerruncontrol.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/taskhub.h>

#include <texteditor/texteditorsettings.h>

#include <utils/fsengine/fileiconprovider.h>
#include <utils/theme/theme.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

static QFuture<QTextDocument *> highlightCode(const QString &code, const QString &mimeType)
{
    QTextDocument *document = new QTextDocument;
    document->setPlainText(code);

    std::shared_ptr<QPromise<QTextDocument *>> promise
        = std::make_shared<QPromise<QTextDocument *>>();

    promise->start();

    TextEditor::SyntaxHighlighter *highlighter = createPythonHighlighter();

    QObject::connect(
        highlighter, &TextEditor::SyntaxHighlighter::finished, document, [document, promise]() {
            promise->addResult(document);
            promise->finish();
        });

    QFutureWatcher<QTextDocument *> *watcher = new QFutureWatcher<QTextDocument *>(document);
    QObject::connect(watcher, &QFutureWatcher<QTextDocument *>::canceled, document, [document]() {
        document->deleteLater();
    });
    watcher->setFuture(promise->future());

    highlighter->setParent(document);
    highlighter->setFontSettings(TextEditor::TextEditorSettings::fontSettings());
    highlighter->setMimeType(mimeType);
    highlighter->setDocument(document);
    highlighter->rehighlight();

    return promise->future();
}

class PythonPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Python.json")

    void initialize() final
    {
        setupPythonEditorFactory(this);

        setupPySideBuildStep();
        setupPythonBuildConfiguration();

        setupPythonRunConfiguration();
        setupPythonRunWorker();
        setupPythonDebugWorker();
        setupPythonOutputParser();

        setupPythonSettings(this);
        setupPythonWizard();

        setupPipSupport(this);

        KitManager::setIrrelevantAspects(KitManager::irrelevantAspects()
                                         + QSet<Id>{PythonKitAspect::id()});

        ProjectManager::registerProjectType<PythonProject>(Constants::C_PY_PROJECT_MIME_TYPE);
        ProjectManager::registerProjectType<PythonProject>(Constants::C_PY_PROJECT_MIME_TYPE_LEGACY);

        auto oldHighlighter = Utils::Text::codeHighlighter();
        Utils::Text::setCodeHighlighter(
            [oldHighlighter](const QString &code, const QString &mimeType)
                -> QFuture<QTextDocument *> {
                if (mimeType == "text/python" || mimeType == "text/x-python"
                    || mimeType == "text/x-python3") {
                    return highlightCode(code, mimeType);
                }

                return oldHighlighter(code, mimeType);
            });
    }

    void extensionsInitialized() final
    {
        // Add MIME overlay icons (these icons displayed at Project dock panel)
        const QString imageFile = Utils::creatorTheme()->imageFile(Theme::IconOverlayPro,
                                                               ProjectExplorer::Constants::FILEOVERLAY_PY);
        FileIconProvider::registerIconOverlayForSuffix(imageFile, "py");

        TaskHub::addCategory({PythonErrorTaskCategory,
                              "Python",
                              Tr::tr("Issues parsed from Python runtime output."),
                              true});
    }
};

} // Python::Internal

#include "pythonplugin.moc"
