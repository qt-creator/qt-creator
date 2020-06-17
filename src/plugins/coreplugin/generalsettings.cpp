/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "generalsettings.h"
#include "coreconstants.h"
#include "icore.h"

#include "ui_generalsettings.h"

#include <coreplugin/dialogs/restartdialog.h>

#include <utils/checkablemessagebox.h>
#include <utils/hostosinfo.h>
#include <utils/infobar.h>
#include <utils/stylehelper.h>

#include <QCoreApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QSettings>
#include <QStyleHints>

using namespace Utils;

namespace Core {
namespace Internal {

const char settingsKeyDPI[] = "Core/EnableHighDpiScaling";
const char settingsKeyShortcutsInContextMenu[] = "General/ShowShortcutsInContextMenu";

class GeneralSettingsWidget final : public IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Core::Internal::GeneralSettings)

public:
    explicit GeneralSettingsWidget(GeneralSettings *q);

    void apply() final;

    void resetInterfaceColor();
    void resetWarnings();
    void resetLanguage();

    bool canResetWarnings() const;
    void fillLanguageBox() const;
    QString language() const;
    void setLanguage(const QString&);

    GeneralSettings *q;
    Ui::GeneralSettings m_ui;
};

GeneralSettingsWidget::GeneralSettingsWidget(GeneralSettings *q)
    : q(q)
{
    m_ui.setupUi(this);

    fillLanguageBox();

    m_ui.colorButton->setColor(StyleHelper::requestedBaseColor());
    m_ui.resetWarningsButton->setEnabled(canResetWarnings());

    m_ui.showShortcutsInContextMenus->setText(
                tr("Show keyboard shortcuts in context menus (default: %1)")
                .arg(q->m_defaultShowShortcutsInContextMenu ? tr("on") : tr("off")));
    m_ui.showShortcutsInContextMenus->setChecked(q->showShortcutsInContextMenu());
#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
    m_ui.showShortcutsInContextMenus->setVisible(false);
#endif

    if (Utils::HostOsInfo().isMacHost()) {
        m_ui.dpiCheckbox->setVisible(false);
    } else {
        const bool defaultValue = Utils::HostOsInfo().isWindowsHost();
        m_ui.dpiCheckbox->setChecked(ICore::settings()->value(settingsKeyDPI, defaultValue).toBool());
        connect(m_ui.dpiCheckbox, &QCheckBox::toggled, this, [](bool checked) {
            ICore::settings()->setValue(settingsKeyDPI, checked);
            QMessageBox::information(ICore::dialogParent(),
                                     tr("Restart Required"),
                                     tr("The high DPI settings will take effect after restart."));
        });
    }

    connect(m_ui.resetColorButton, &QAbstractButton::clicked,
            this, &GeneralSettingsWidget::resetInterfaceColor);
    connect(m_ui.resetWarningsButton, &QAbstractButton::clicked,
            this, &GeneralSettingsWidget::resetWarnings);
}

static bool hasQmFilesForLocale(const QString &locale, const QString &creatorTrPath)
{
    static const QString qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    const QString trFile = QLatin1String("/qt_") + locale + QLatin1String(".qm");
    return QFile::exists(qtTrPath + trFile) || QFile::exists(creatorTrPath + trFile);
}

void GeneralSettingsWidget::fillLanguageBox() const
{
    const QString currentLocale = language();

    m_ui.languageBox->addItem(tr("<System Language>"), QString());
    // need to add this explicitly, since there is no qm file for English
    m_ui.languageBox->addItem(QLatin1String("English"), QLatin1String("C"));
    if (currentLocale == QLatin1String("C"))
        m_ui.languageBox->setCurrentIndex(m_ui.languageBox->count() - 1);

    const QString creatorTrPath = ICore::resourcePath() + QLatin1String("/translations");
    const QStringList languageFiles = QDir(creatorTrPath).entryList(QStringList(QLatin1String("qtcreator*.qm")));

    for (const QString &languageFile : languageFiles) {
        int start = languageFile.indexOf('_') + 1;
        int end = languageFile.lastIndexOf('.');
        const QString locale = languageFile.mid(start, end-start);
        // no need to show a language that creator will not load anyway
        if (hasQmFilesForLocale(locale, creatorTrPath)) {
            QLocale tmpLocale(locale);
            QString languageItem = QLocale::languageToString(tmpLocale.language()) + QLatin1String(" (")
                                   + QLocale::countryToString(tmpLocale.country()) + QLatin1Char(')');
            m_ui.languageBox->addItem(languageItem, locale);
            if (locale == currentLocale)
                m_ui.languageBox->setCurrentIndex(m_ui.languageBox->count() - 1);
        }
    }
}

void GeneralSettingsWidget::apply()
{
    int currentIndex = m_ui.languageBox->currentIndex();
    setLanguage(m_ui.languageBox->itemData(currentIndex, Qt::UserRole).toString());
    q->setShowShortcutsInContextMenu(m_ui.showShortcutsInContextMenus->isChecked());
    // Apply the new base color if accepted
    StyleHelper::setBaseColor(m_ui.colorButton->color());
    m_ui.themeChooser->apply();
}

bool GeneralSettings::showShortcutsInContextMenu() const
{
    return ICore::settings()
        ->value(settingsKeyShortcutsInContextMenu,
                QGuiApplication::styleHints()->showShortcutsInContextMenus())
        .toBool();
}

void GeneralSettingsWidget::resetInterfaceColor()
{
    m_ui.colorButton->setColor(StyleHelper::DEFAULT_BASE_COLOR);
}

void GeneralSettingsWidget::resetWarnings()
{
    InfoBar::clearGloballySuppressed();
    CheckableMessageBox::resetAllDoNotAskAgainQuestions(ICore::settings());
    m_ui.resetWarningsButton->setEnabled(false);
}

bool GeneralSettingsWidget::canResetWarnings() const
{
    return InfoBar::anyGloballySuppressed()
        || CheckableMessageBox::hasSuppressedQuestions(ICore::settings());
}

void GeneralSettingsWidget::resetLanguage()
{
    // system language is default
    m_ui.languageBox->setCurrentIndex(0);
}

QString GeneralSettingsWidget::language() const
{
    QSettings *settings = ICore::settings();
    return settings->value(QLatin1String("General/OverrideLanguage")).toString();
}

void GeneralSettingsWidget::setLanguage(const QString &locale)
{
    QSettings *settings = ICore::settings();
    if (settings->value(QLatin1String("General/OverrideLanguage")).toString() != locale) {
        RestartDialog dialog(ICore::dialogParent(),
                             tr("The language change will take effect after restart."));
        dialog.exec();
    }

    if (locale.isEmpty())
        settings->remove(QLatin1String("General/OverrideLanguage"));
    else
        settings->setValue(QLatin1String("General/OverrideLanguage"), locale);
}

void GeneralSettings::setShowShortcutsInContextMenu(bool show)
{
    if (show == m_defaultShowShortcutsInContextMenu)
        ICore::settings()->remove(settingsKeyShortcutsInContextMenu);
    else
        ICore::settings()->setValue(settingsKeyShortcutsInContextMenu, show);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
    QGuiApplication::styleHints()->setShowShortcutsInContextMenus(show);
#endif
}

GeneralSettings::GeneralSettings()
{
    setId(Constants::SETTINGS_ID_INTERFACE);
    setDisplayName(GeneralSettingsWidget::tr("Interface"));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
    setDisplayCategory(QCoreApplication::translate("Core", "Environment"));
    setCategoryIconPath(":/core/images/settingscategory_core.png");
    setWidgetCreator([this] { return new GeneralSettingsWidget(this); });

    m_defaultShowShortcutsInContextMenu = QGuiApplication::styleHints()
                                              ->showShortcutsInContextMenus();
}

} // namespace Internal
} // namespace Core
