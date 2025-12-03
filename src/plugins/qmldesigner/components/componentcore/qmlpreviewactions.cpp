// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewactions.h"

#include <designeractionmanager.h>
#include <qmldesignertr.h>
#include <zoomaction.h>

#include <utils/id.h>
#include <utils/utilsicons.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <android/androidconstants.h>

#include <qmlpreview/qmlpreviewplugin.h>

#include <QLabel>
#include <QComboBox>
#include <QPointer>

namespace QmlDesigner {

using QmlPreview::QmlPreviewPlugin;
using QmlPreview::QmlPreviewRunControlList;

const Utils::Icon previewIcon({{":/componentcore/images/live_preview.png",
                                Utils::Theme::IconsBaseColor}});
const QByteArray livePreviewId = "LivePreview";

static void handleAction(const SelectionContext &context)
{
    if (context.isValid()) {
        if (context.toggled()) {
            bool skipDeploy = false;
            if (const auto startupTarget = ProjectExplorer::ProjectManager::startupTarget()) {
                const auto kit = startupTarget->kit();
                if (kit
                    && (kit->supportedPlatforms().contains(Android::Constants::ANDROID_DEVICE_TYPE)
                        || ProjectExplorer::RunDeviceTypeKitAspect::deviceTypeId(kit)
                               == Android::Constants::ANDROID_DEVICE_TYPE)) {
                    skipDeploy = true;
                    // In case of an android kit we don't want the live preview button to be toggled
                    // when the emulator is started as we don't have control over its run status.
                    DesignerActionManager &designerActionManager = QmlDesignerPlugin::instance()
                                                                       ->designerActionManager();
                    if (const ActionInterface *interface = designerActionManager.actionByMenuId(
                            livePreviewId))
                        interface->action()->setChecked(false);
                }
            }
            ProjectExplorer::ProjectExplorerPlugin::runStartupProject(
                ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE, skipDeploy);
        } else {
            const auto runControls = QmlPreviewPlugin::instance()->runningPreviews();
            for (ProjectExplorer::RunControl *runControl : runControls)
                runControl->initiateStop();
        }
    }
}

QmlPreviewAction::QmlPreviewAction()
    : ModelNodeAction(livePreviewId,
                      "Live Preview",
                      previewIcon.icon(),
                      Tr::tr("Show Live Preview"),
                      ComponentCoreConstants::qmlPreviewCategory,
                      QKeySequence("Alt+p"),
                      1,
                      &handleAction)
{
    action()->setCheckable(true);

    auto previewPlugin = QmlPreview::QmlPreviewPlugin::instance();
    QTC_ASSERT(previewPlugin, return);

    QObject::connect(previewPlugin,
                     &QmlPreviewPlugin::runningPreviewsChanged,
                     action(),
                     [this](const QmlPreviewRunControlList &runControls) {
                         action()->setChecked(!runControls.isEmpty());
                     });
}

void QmlPreviewAction::updateContext()
{
    if (selectionContext().view()->isAttached()) {
        const Utils::FilePath qmlFileName = QmlDesignerPlugin::instance()
                                                ->currentDesignDocument()
                                                ->fileName();
        QmlPreviewPlugin::instance()->setPreviewedFile(qmlFileName.toUrlishString());
    }

    pureAction()->setSelectionContext(selectionContext());
}

ActionInterface::Type QmlPreviewAction::type() const
{
    return ToolBarAction;
}

class ZoomPureAction : public PureActionInterface
{
    Q_DISABLE_COPY(ZoomPureAction)

public:
    ZoomPureAction()
        : PureActionInterface(ZoomPureAction::createAction())
    {}

    void setSelectionContext(const SelectionContext &) override {};

private:
    static ZoomAction *createAction()
    {
        ZoomAction *zoomAction = new ZoomAction(nullptr);
        QObject::connect(zoomAction,
                         &ZoomAction::zoomLevelChanged,
                         QmlPreviewPlugin::instance(),
                         &QmlPreviewPlugin::setZoomFactor);
        return zoomAction;
    }
};

ZoomPreviewAction::ZoomPreviewAction()
    : AbstractAction(new ZoomPureAction())
{}

ZoomPreviewAction::~ZoomPreviewAction() = default;

QByteArray ZoomPreviewAction::category() const
{
    return ComponentCoreConstants::qmlPreviewCategory;
}

QByteArray ZoomPreviewAction::menuId() const
{
    return "PreviewZoom";
}

int ZoomPreviewAction::priority() const
{
    return 19;
}

ActionInterface::Type ZoomPreviewAction::type() const
{
    return ToolBarAction;
}

bool ZoomPreviewAction::isVisible(const SelectionContext &) const
{
    return true;
}

bool ZoomPreviewAction::isEnabled(const SelectionContext &) const
{
    return true;
}

quint16 FpsLabelAction::lastValidFrames = 0;
QList<QPointer<QLabel>> FpsLabelAction::fpsHandlerLabelList;

FpsLabelAction::FpsLabelAction(QObject *parent)
    : QWidgetAction(parent)
{
    auto previewPlugin = QmlPreview::QmlPreviewPlugin::instance();
    QTC_ASSERT(previewPlugin, return);
    connect(previewPlugin,
            &QmlPreviewPlugin::runningPreviewsChanged,
            this,
            [](const QmlPreviewRunControlList &runControls) {
                if (runControls.isEmpty())
                    FpsLabelAction::cleanFpsCounter();
            });
}

void FpsLabelAction::fpsHandler(quint16 fpsValues[8])
{
    quint16 frames = fpsValues[0];
    if (frames != 0)
        lastValidFrames = frames;
    QString fpsText("%1 FPS");
    if (lastValidFrames == 0 || (frames == 0 && lastValidFrames < 2))
        fpsText = fpsText.arg("--");
    else
        fpsText = fpsText.arg(lastValidFrames);
    for (const QPointer<QLabel> &label : std::as_const(fpsHandlerLabelList)) {
        if (label)
            label->setText(fpsText);
    }
}

void FpsLabelAction::cleanFpsCounter()
{
    lastValidFrames = 0;
    quint16 nullInitialized[8] = {0};
    fpsHandler(nullInitialized);
}

QWidget *FpsLabelAction::createWidget(QWidget *parent)
{
    auto label = new QLabel(parent);
    auto originList = fpsHandlerLabelList;
    fpsHandlerLabelList.clear();
    fpsHandlerLabelList.append(label);
    for (const auto &labelPointer : originList) {
        if (labelPointer)
            fpsHandlerLabelList.append(labelPointer);
    }

    return label;
}

void FpsLabelAction::refreshFpsLabel(quint16 frames)
{
    for (const auto &labelPointer : std::as_const(fpsHandlerLabelList)) {
        if (labelPointer)
            labelPointer->setText(QString("%1 FPS").arg(frames));
    }
}

FpsAction::FpsAction() : m_fpsLabelAction(new FpsLabelAction(nullptr))
{}

QAction *FpsAction::action() const
{
    return m_fpsLabelAction.get();
}

QByteArray FpsAction::category() const
{
    return ComponentCoreConstants::qmlPreviewCategory;
}

QByteArray FpsAction::menuId() const
{
    return QByteArray();
}

int FpsAction::priority() const
{
    return 19;
}

ActionInterface::Type FpsAction::type() const
{
    return ToolBarAction;
}

void FpsAction::currentContextChanged(const SelectionContext &)
{}

SwitchLanguageComboboxAction::SwitchLanguageComboboxAction(QObject *parent)
    : QWidgetAction(parent)
{
}

QWidget *SwitchLanguageComboboxAction::createWidget(QWidget *parent)
{
    QPointer<QComboBox> comboBox = new QComboBox(parent);
    const QString toolTip(Tr::tr("Switch the language used by preview."));
    comboBox->setToolTip(toolTip);
    comboBox->addItem(Tr::tr("Default"));

    auto refreshComboBoxFunction = [this, comboBox, toolTip] (ProjectExplorer::Project *project) {
        if (comboBox && project) {
            comboBox->setDisabled(true);
            QString errorMessage;
            auto locales = project->availableQmlPreviewTranslations(&errorMessage);
            if (!errorMessage.isEmpty())
                comboBox->setToolTip(QString("%1<br/>(%2)").arg(toolTip, errorMessage));
            if (m_previousLocales != locales) {
                comboBox->clear();
                comboBox->addItem(Tr::tr("Default"));
                comboBox->addItems(locales);
                m_previousLocales = locales;
                comboBox->setEnabled(true);
            }
        }
    };
    connect(ProjectExplorer::ProjectManager::instance(),  &ProjectExplorer::ProjectManager::startupProjectChanged,
        comboBox, refreshComboBoxFunction);

    if (auto project = ProjectExplorer::ProjectManager::startupProject())
        refreshComboBoxFunction(project);

    // do this after refreshComboBoxFunction so we do not get currentLocaleChanged signals at initialization
    connect(comboBox, &QComboBox::currentIndexChanged, [this, comboBox](int index) {
        if (index == 0) // == Default
            emit currentLocaleChanged("");
        else
            emit currentLocaleChanged(comboBox->currentText());
    });

    return comboBox;
}

SwitchLanguageAction::SwitchLanguageAction()
    : m_switchLanguageAction(new SwitchLanguageComboboxAction(nullptr))
{
    QObject::connect(m_switchLanguageAction.get(),
                     &SwitchLanguageComboboxAction::currentLocaleChanged,
                     QmlPreviewPlugin::instance(),
                     &QmlPreviewPlugin::setLocaleIsoCode);
}

QAction *SwitchLanguageAction::action() const
{
    return m_switchLanguageAction.get();
}

QByteArray SwitchLanguageAction::category() const
{
    return ComponentCoreConstants::qmlPreviewCategory;
}

QByteArray SwitchLanguageAction::menuId() const
{
    return QByteArray();
}

int SwitchLanguageAction::priority() const
{
    return 10;
}

ActionInterface::Type SwitchLanguageAction::type() const
{
    return ToolBarAction;
}

void SwitchLanguageAction::currentContextChanged(const SelectionContext &)
{}

void setupQmlPreviewActions()
{
    auto previewPlugin = QmlPreview::QmlPreviewPlugin::instance();
    QTC_ASSERT(previewPlugin, return);

    Core::Context globalContext;
    auto registerCommand = [&globalContext](ActionInterface *action) {
        using Utils::Id;
        const Id id = Id("QmlPreview.").withSuffix(action->menuId());
        Core::Command *cmd = Core::ActionManager::registerAction(action->action(), id, globalContext);

        cmd->setDefaultKeySequence(action->action()->shortcut());
        cmd->setDescription(action->action()->toolTip());

        action->action()->setToolTip(cmd->action()->toolTip());
        action->action()->setShortcut(cmd->action()->shortcut());
    };

    DesignerActionManager &designerActionManager = QmlDesignerPlugin::instance()->designerActionManager();
    designerActionManager.addDesignerAction(
        new ActionGroup(QString(),
                        ComponentCoreConstants::qmlPreviewCategory,
                        {},
                        ComponentCoreConstants::Priorities::QmlPreviewCategory,
                        &SelectionContextFunctors::always));

    auto previewAction = new QmlPreviewAction();
    designerActionManager.addDesignerAction(previewAction);
    // Only register previewAction as others don't have keyboard shortcuts for them
    registerCommand(previewAction);

    auto zoomAction = new ZoomPreviewAction;
    designerActionManager.addDesignerAction(zoomAction);

    auto separator = new SeparatorDesignerAction(ComponentCoreConstants::qmlPreviewCategory, 0);
    designerActionManager.addDesignerAction(separator);

    auto fpsAction = new FpsAction;
    designerActionManager.addDesignerAction(fpsAction);
    previewPlugin->setFpsHandler(FpsLabelAction::fpsHandler);

    auto switchLanguageAction = new SwitchLanguageAction;
    designerActionManager.addDesignerAction(switchLanguageAction);
}

} // namespace QmlDesigner
