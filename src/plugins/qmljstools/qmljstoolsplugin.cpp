// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsbundleprovider.h"
#include "qmljscodestylesettingspage.h"
#include "qmljsfunctionfilter.h"
#include "qmljsmodelmanager.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolssettings.h"
#include "qmljstoolstr.h"
#include "qmljstools_test.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <extensionsystem/iplugin.h>

#include <qmljseditor/qmljseditorconstants.h>

#include <QMenu>

using namespace Core;

namespace QmlJSTools::Internal {

class QmlJSToolsPluginPrivate : public QObject
{
public:
    QmlJSToolsPluginPrivate();

    QmlJSToolsSettings settings;
    ModelManager modelManager;

    QAction resetCodeModelAction{Tr::tr("Reset Code Model"), nullptr};

    QmlJSCodeStyleSettingsPage codeStyleSettingsPage;
    BasicBundleProvider basicBundleProvider;
};

QmlJSToolsPluginPrivate::QmlJSToolsPluginPrivate()
{
    // Menus
    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    ActionContainer *mqmljstools = ActionManager::createMenu(Constants::M_TOOLS_QMLJS);
    QMenu *menu = mqmljstools->menu();
    menu->setTitle(Tr::tr("&QML/JS"));
    menu->setEnabled(true);
    mtools->addMenu(mqmljstools);

    // Update context in global context
    Command *cmd = ActionManager::registerAction(
                &resetCodeModelAction, Constants::RESET_CODEMODEL);
    connect(&resetCodeModelAction, &QAction::triggered,
            &modelManager, &ModelManager::resetCodeModel);
    mqmljstools->addAction(cmd);

    // Watch task progress
    connect(ProgressManager::instance(), &ProgressManager::taskStarted, this,
            [this](Utils::Id type) {
                  if (type == QmlJS::Constants::TASK_INDEX)
                      resetCodeModelAction.setEnabled(false);
            });

    connect(ProgressManager::instance(), &ProgressManager::allTasksFinished,
            [this](Utils::Id type) {
                  if (type == QmlJS::Constants::TASK_INDEX)
                      resetCodeModelAction.setEnabled(true);
            });
}

class QmlJSToolsPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlJSTools.json")

public:
    ~QmlJSToolsPlugin() final
    {
        delete d;
    }

private:
    void initialize() final
    {
        IOptionsPage::registerCategory(
            QmlJSEditor::Constants::SETTINGS_CATEGORY_QML,
            Tr::tr("Qt Quick"),
            ":/qmljstools/images/settingscategory_qml.png");

#ifdef WITH_TESTS
        addTestCreator(createQmlJSToolsTest);
#endif
        d = new QmlJSToolsPluginPrivate;

        setupQmlJSFunctionsFilter();
    }

    void extensionsInitialized() final
    {
        d->modelManager.delayedInitialization();
    }

    QmlJSToolsPluginPrivate *d = nullptr;
};

} // QmlJSTools::Internal

#include "qmljstoolsplugin.moc"
