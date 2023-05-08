// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsplugin.h"

#include "commonvcssettings.h"
#include "nicknamedialog.h"
#include "vcsbaseconstants.h"
#include "vcsbasesubmiteditor.h"
#include "vcsbasetr.h"
#include "vcsoutputwindow.h"
#include "wizard/vcscommandpage.h"
#include "wizard/vcsconfigurationpage.h"
#include "wizard/vcsjsextension.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/jsexpander.h>
#include <coreplugin/vcsmanager.h>

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>

#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QDebug>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace VcsBase::Internal {

class VcsPluginPrivate
{
public:
    explicit VcsPluginPrivate(VcsPlugin *plugin)
        : q(plugin)
    {
        QObject::connect(&m_settingsPage.settings(), &CommonVcsSettings::settingsChanged,
                         [this] { slotSettingsChanged(); });
        slotSettingsChanged();
    }

    QStandardItemModel *nickNameModel()
    {
        if (!m_nickNameModel) {
            m_nickNameModel = NickNameDialog::createModel(q);
            populateNickNameModel();
        }
        return m_nickNameModel;
    }

    void populateNickNameModel()
    {
        QString errorMessage;
        if (!NickNameDialog::populateModelFromMailCapFile(m_settingsPage.settings().nickNameMailMap.filePath(),
                                                          m_nickNameModel,
                                                          &errorMessage)) {
            qWarning("%s", qPrintable(errorMessage));
        }
    }

    void slotSettingsChanged()
    {
        if (m_nickNameModel)
            populateNickNameModel();
    }

    VcsPlugin *q;
    CommonOptionsPage m_settingsPage;
    QStandardItemModel *m_nickNameModel = nullptr;
};

static VcsPlugin *m_instance = nullptr;

VcsPlugin::VcsPlugin()
{
    m_instance = this;
}

VcsPlugin::~VcsPlugin()
{
    QTC_ASSERT(d, return);
    VcsOutputWindow::destroy();
    m_instance = nullptr;
    delete d;
}

void VcsPlugin::initialize()
{
    d = new VcsPluginPrivate(this);

    EditorManager::addCloseEditorListener([this](IEditor *editor) -> bool {
        bool result = true;
        if (auto se = qobject_cast<VcsBaseSubmitEditor *>(editor))
            emit submitEditorAboutToClose(se, &result);
        return result;
    });

    connect(&d->m_settingsPage.settings(), &CommonVcsSettings::settingsChanged,
            this, &VcsPlugin::settingsChanged);

    JsonWizardFactory::registerPageFactory(new Internal::VcsConfigurationPageFactory);
    JsonWizardFactory::registerPageFactory(new Internal::VcsCommandPageFactory);

    JsExpander::registerGlobalObject<VcsJsExtension>("Vcs");

    MacroExpander *expander = globalMacroExpander();
    expander->registerVariable(Constants::VAR_VCS_NAME,
        Tr::tr("Name of the version control system in use by the current project."),
        []() -> QString {
            IVersionControl *vc = nullptr;
            if (Project *project = ProjectTree::currentProject())
                vc = VcsManager::findVersionControlForDirectory(project->projectDirectory());
            return vc ? vc->displayName() : QString();
        });

    expander->registerVariable(Constants::VAR_VCS_TOPIC,
        Tr::tr("The current version control topic (branch or tag) identification of the current project."),
        []() -> QString {
            IVersionControl *vc = nullptr;
            FilePath topLevel;
            if (Project *project = ProjectTree::currentProject())
                vc = VcsManager::findVersionControlForDirectory(project->projectDirectory(), &topLevel);
            return vc ? vc->vcsTopic(topLevel) : QString();
        });

    expander->registerVariable(Constants::VAR_VCS_TOPLEVELPATH,
        Tr::tr("The top level path to the repository the current project is in."),
        []() -> QString {
            if (Project *project = ProjectTree::currentProject())
                return VcsManager::findTopLevelForDirectory(project->projectDirectory()).toString();
            return QString();
        });

    // Just touch VCS Output Pane before initialization
    VcsOutputWindow::instance();
}

VcsPlugin *VcsPlugin::instance()
{
    return m_instance;
}

CommonVcsSettings &VcsPlugin::settings() const
{
    return d->m_settingsPage.settings();
}

/* Delayed creation/update of the nick name model. */
QStandardItemModel *VcsPlugin::nickNameModel()
{
    QTC_ASSERT(d, return nullptr);
    return d->nickNameModel();
}

} // VcsBase::Internal
