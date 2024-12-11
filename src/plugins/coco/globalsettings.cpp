// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "globalsettings.h"

#include "cocoinstallation.h"
#include "cocopluginconstants.h"
#include "cocotr.h"

#include <coreplugin/icore.h>

#include <utils/fancylineedit.h>
#include <utils/filepath.h>
#include <utils/layoutbuilder.h>

#include <QProcess>
#include <QSettings>

namespace Coco::Internal {
namespace GlobalSettings {

static const char DIRECTORY[] = "CocoDirectory";

void read()
{
    CocoInstallation coco;
    bool directoryInSettings = false;

    Utils::QtcSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::COCO_SETTINGS_GROUP);
    const QStringList keys = s->allKeys();
    for (const QString &keyString : keys) {
        Utils::Key key(keyString.toLatin1());
        if (key == DIRECTORY) {
            coco.setDirectory(Utils::FilePath::fromUserInput(s->value(key).toString()));
            directoryInSettings = true;
        } else
            s->remove(key);
    }
    s->endGroup();

    if (!directoryInSettings)
        coco.findDefaultDirectory();

    GlobalSettings::save();
}

void save()
{
    Utils::QtcSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::COCO_SETTINGS_GROUP);
    s->setValue(DIRECTORY, CocoInstallation().directory().toUserOutput());
    s->endGroup();
}

} // namespace GlobalSettings

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
