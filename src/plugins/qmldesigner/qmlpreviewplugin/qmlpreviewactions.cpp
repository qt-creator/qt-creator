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
#include "qmlpreviewactions.h"

#include <zoomaction.h>
#include <designersettings.h>

#include <utils/utilsicons.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>

#include <QLabel>
#include <QComboBox>
#include <QPointer>

namespace QmlDesigner {

using namespace ProjectExplorer;

const Utils::Icon previewIcon({
        {":/qmlpreviewplugin/images/live_preview.png", Utils::Theme::IconsBaseColor}});

static void handleAction(const SelectionContext &context)
{
    if (context.view()->isAttached()) {
        if (context.toggled())
            ProjectExplorerPlugin::runStartupProject(Constants::QML_PREVIEW_RUN_MODE);
         else
            QmlPreviewPlugin::stopAllRunControls();
    }
}

QmlPreviewAction::QmlPreviewAction() : ModelNodeAction("LivePreview",
                                                       "Live Preview",
                                                       previewIcon.icon(),
                                                       QmlPreviewPlugin::tr("Show Live Preview"),
                                                       ComponentCoreConstants::qmlPreviewCategory,
                                                       QKeySequence("Alt+p"),
                                                       20,
                                                       &handleAction,
                                                       &SelectionContextFunctors::always)
{
    if (!QmlPreviewPlugin::getPreviewPlugin())
        defaultAction()->setVisible(false);

    defaultAction()->setCheckable(true);
}

void QmlPreviewAction::updateContext()
{
    if (selectionContext().view()->isAttached())
        QmlPreviewPlugin::setQmlFile();

    defaultAction()->setSelectionContext(selectionContext());
}

ActionInterface::Type QmlPreviewAction::type() const
{
    return ToolBarAction;
}

ZoomPreviewAction::ZoomPreviewAction()
    : m_zoomAction(new ZoomAction(nullptr))
{
    QObject::connect(m_zoomAction.get(), &ZoomAction::zoomLevelChanged, [=](float d) {
        QmlPreviewPlugin::setZoomFactor(d);
    });
    if (!QmlPreviewPlugin::getPreviewPlugin())
        m_zoomAction->setVisible(false);
}

ZoomPreviewAction::~ZoomPreviewAction()
= default;

QAction *ZoomPreviewAction::action() const
{
    return m_zoomAction.get();
}

QByteArray ZoomPreviewAction::category() const
{
    return ComponentCoreConstants::qmlPreviewCategory;
}

QByteArray ZoomPreviewAction::menuId() const
{
    return QByteArray();
}

int ZoomPreviewAction::priority() const
{
    return 19;
}

ActionInterface::Type ZoomPreviewAction::type() const
{
    return ToolBarAction;
}

void ZoomPreviewAction::currentContextChanged(const SelectionContext &)
{}

quint16 FpsLabelAction::lastValidFrames = 0;
QList<QPointer<QLabel>> FpsLabelAction::fpsHandlerLabelList;

FpsLabelAction::FpsLabelAction(QObject *parent)
    : QWidgetAction(parent)
{
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
    for (const QPointer<QLabel> &label : qAsConst(fpsHandlerLabelList)) {
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
    for (const auto &labelPointer : fpsHandlerLabelList) {
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
    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::startupProjectChanged,
            this,
            &SwitchLanguageComboboxAction::updateProjectLocales);
}

QWidget *SwitchLanguageComboboxAction::createWidget(QWidget *parent)
{
    QPointer<QComboBox> comboBox = new QComboBox(parent);
    comboBox->setToolTip(tr("Switch the language used by preview."));
    comboBox->addItem(tr("Default"));

    auto refreshComboBoxFunction = [this, comboBox] (ProjectExplorer::Project *project) {
        if (comboBox) {
            if (updateProjectLocales(project)) {
                comboBox->clear();
                comboBox->addItem(tr("Default"));
                comboBox->addItems(m_localeStrings);
            }
        }
    };
    connect(ProjectExplorer::SessionManager::instance(),  &ProjectExplorer::SessionManager::startupProjectChanged,
        comboBox, refreshComboBoxFunction);

    if (auto project = SessionManager::startupProject())
        refreshComboBoxFunction(project);

    // do this after refreshComboBoxFunction so we do not get currentLocaleChanged signals at initialization
    connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, comboBox](int index) {
        if (index == 0) // == Default
            emit currentLocaleChanged("");
        else
            emit currentLocaleChanged(comboBox->currentText());
    });

    return comboBox;
}

bool SwitchLanguageComboboxAction::updateProjectLocales(Project *project)
{
    if (!project)
        return false;
    auto previousLocales = m_localeStrings;
    m_localeStrings.clear();
    const auto projectDirectory = project->rootProjectDirectory().toFileInfo().absoluteFilePath();
    const QDir languageDirectory(projectDirectory + "/i18n");
    const auto qmFiles = languageDirectory.entryList({"qml_*.qm"});
    m_localeStrings = Utils::transform(qmFiles, [](const QString &qmFile) {
        const int localeStartPosition = qmFile.lastIndexOf("_") + 1;
        const int localeEndPosition = qmFile.size() - QString(".qm").size();
        const QString locale = qmFile.left(localeEndPosition).mid(localeStartPosition);
        return locale;
    });
    return previousLocales != m_localeStrings;
}

SwitchLanguageAction::SwitchLanguageAction()
    : m_switchLanguageAction(new SwitchLanguageComboboxAction(nullptr))
{
    QObject::connect(m_switchLanguageAction.get(), &SwitchLanguageComboboxAction::currentLocaleChanged,
                     &QmlPreviewPlugin::setLanguageLocale);
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


} // namespace QmlDesigner
