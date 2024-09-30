// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "globalsettingspage.h"

#include "cocoinstallation.h"
#include "cocopluginconstants.h"
#include "cocotr.h"
#include "globalsettings.h"

#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>

namespace Coco::Internal {

GlobalSettingsWidget::GlobalSettingsWidget(QFrame *parent)
    : QFrame(parent)
{
    m_cocoPathAspect.setDefaultPathValue(m_coco.directory());
    m_cocoPathAspect.setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_cocoPathAspect.setPromptDialogTitle(Tr::tr("Coco Installation Directory"));

    connect(
        &m_cocoPathAspect,
        &Utils::FilePathAspect::changed,
        this,
        &GlobalSettingsWidget::onCocoPathChanged);

    using namespace Layouting;
    Form{
        Column{
            Row{Tr::tr("Coco Directory"), m_cocoPathAspect},
            Row{m_messageLabel}}
    }.attachTo(this);
}

void GlobalSettingsWidget::onCocoPathChanged()
{
    if (!verifyCocoDirectory(m_cocoPathAspect()))
        m_cocoPathAspect.setValue(m_previousCocoDir, Utils::BaseAspect::BeQuiet);
}

bool GlobalSettingsWidget::verifyCocoDirectory(const Utils::FilePath &cocoDir)
{
    m_coco.setDirectory(cocoDir);
    m_messageLabel.setText(m_coco.errorMessage());
    if (m_coco.isValid())
        m_messageLabel.setIconType(Utils::InfoLabel::None);
    else
        m_messageLabel.setIconType(Utils::InfoLabel::Error);
    return m_coco.isValid();
}

void GlobalSettingsWidget::apply()
{
    if (!verifyCocoDirectory(widgetCocoDir()))
        return;

    m_coco.setDirectory(widgetCocoDir());
    GlobalSettings::save();

    emit updateCocoDir();
}

void GlobalSettingsWidget::cancel()
{
    m_coco.setDirectory(m_previousCocoDir);
}

void GlobalSettingsWidget::setVisible(bool visible)
{
    QFrame::setVisible(visible);
    m_previousCocoDir = m_coco.directory();
}

Utils::FilePath GlobalSettingsWidget::widgetCocoDir() const
{
    return Utils::FilePath::fromUserInput(m_cocoPathAspect.value());
}

GlobalSettingsPage::GlobalSettingsPage()
    : m_widget(nullptr)
{
    setId(Constants::COCO_SETTINGS_PAGE_ID);
    setDisplayName(QCoreApplication::translate("Coco", "Coco"));
    setCategory("I.Coco"); // Category I contains also the C++ settings.
    setDisplayCategory(QCoreApplication::translate("Coco", "Coco"));
    setCategoryIconPath(":/cocoplugin/images/SquishCoco_48x48.png");
}

GlobalSettingsPage &GlobalSettingsPage::instance()
{
    static GlobalSettingsPage instance;
    return instance;
}

GlobalSettingsWidget *GlobalSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new GlobalSettingsWidget;
    return m_widget;
}

void GlobalSettingsPage::apply()
{
    if (m_widget)
        m_widget->apply();
}

void GlobalSettingsPage::cancel()
{
    if (m_widget)
        m_widget->cancel();
}

void GlobalSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Coco::Internal
