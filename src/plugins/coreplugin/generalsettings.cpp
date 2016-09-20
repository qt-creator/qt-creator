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
#include "infobar.h"

#include <utils/checkablemessagebox.h>
#include <utils/stylehelper.h>

#include <QCoreApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QSettings>

#include "ui_generalsettings.h"

using namespace Utils;

namespace Core {
namespace Internal {

GeneralSettings::GeneralSettings()
    : m_page(0), m_dialog(0)
{
    setId(Constants::SETTINGS_ID_INTERFACE);
    setDisplayName(tr("Interface"));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
    setDisplayCategory(QCoreApplication::translate("Core", Constants::SETTINGS_TR_CATEGORY_CORE));
    setCategoryIcon(Utils::Icon(Constants::SETTINGS_CATEGORY_CORE_ICON));
}

static bool hasQmFilesForLocale(const QString &locale, const QString &creatorTrPath)
{
    static const QString qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    const QString trFile = QLatin1String("/qt_") + locale + QLatin1String(".qm");
    return QFile::exists(qtTrPath + trFile) || QFile::exists(creatorTrPath + trFile);
}

void GeneralSettings::fillLanguageBox() const
{
    const QString currentLocale = language();

    m_page->languageBox->addItem(tr("<System Language>"), QString());
    // need to add this explicitly, since there is no qm file for English
    m_page->languageBox->addItem(QLatin1String("English"), QLatin1String("C"));
    if (currentLocale == QLatin1String("C"))
        m_page->languageBox->setCurrentIndex(m_page->languageBox->count() - 1);

    const QString creatorTrPath = ICore::resourcePath() + QLatin1String("/translations");
    const QStringList languageFiles = QDir(creatorTrPath).entryList(QStringList(QLatin1String("qtcreator*.qm")));

    foreach (const QString &languageFile, languageFiles) {
        int start = languageFile.indexOf(QLatin1Char('_'))+1;
        int end = languageFile.lastIndexOf(QLatin1Char('.'));
        const QString locale = languageFile.mid(start, end-start);
        // no need to show a language that creator will not load anyway
        if (hasQmFilesForLocale(locale, creatorTrPath)) {
            QLocale tmpLocale(locale);
            QString languageItem = QLocale::languageToString(tmpLocale.language()) + QLatin1String(" (")
                                   + QLocale::countryToString(tmpLocale.country()) + QLatin1Char(')');
            m_page->languageBox->addItem(languageItem, locale);
            if (locale == currentLocale)
                m_page->languageBox->setCurrentIndex(m_page->languageBox->count() - 1);
        }
    }
}

QWidget *GeneralSettings::widget()
{
    if (!m_widget) {
        m_page = new Ui::GeneralSettings();
        m_widget = new QWidget;
        m_page->setupUi(m_widget);

        fillLanguageBox();

        m_page->colorButton->setColor(StyleHelper::requestedBaseColor());
        m_page->resetWarningsButton->setEnabled(canResetWarnings());

        connect(m_page->resetColorButton, &QAbstractButton::clicked,
                this, &GeneralSettings::resetInterfaceColor);
        connect(m_page->resetWarningsButton, &QAbstractButton::clicked,
                this, &GeneralSettings::resetWarnings);
    }
    return m_widget;
}

void GeneralSettings::apply()
{
    if (!m_page) // wasn't shown, can't be changed
        return;
    int currentIndex = m_page->languageBox->currentIndex();
    setLanguage(m_page->languageBox->itemData(currentIndex, Qt::UserRole).toString());
    // Apply the new base color if accepted
    StyleHelper::setBaseColor(m_page->colorButton->color());
    m_page->themeChooser->apply();
}

void GeneralSettings::finish()
{
    delete m_widget;
    delete m_page;
    m_page = 0;
}

void GeneralSettings::resetInterfaceColor()
{
    m_page->colorButton->setColor(StyleHelper::DEFAULT_BASE_COLOR);
}

void GeneralSettings::resetWarnings()
{
    InfoBar::clearGloballySuppressed();
    CheckableMessageBox::resetAllDoNotAskAgainQuestions(ICore::settings());
    m_page->resetWarningsButton->setEnabled(false);
}

bool GeneralSettings::canResetWarnings() const
{
    return InfoBar::anyGloballySuppressed()
        || CheckableMessageBox::hasSuppressedQuestions(ICore::settings());
}

void GeneralSettings::resetLanguage()
{
    // system language is default
    m_page->languageBox->setCurrentIndex(0);
}

QString GeneralSettings::language() const
{
    QSettings *settings = ICore::settings();
    return settings->value(QLatin1String("General/OverrideLanguage")).toString();
}

void GeneralSettings::setLanguage(const QString &locale)
{
    QSettings *settings = ICore::settings();
    if (settings->value(QLatin1String("General/OverrideLanguage")).toString() != locale)
        QMessageBox::information(ICore::mainWindow(), tr("Restart Required"),
                                 tr("The language change will take effect after a restart of Qt Creator."));

    if (locale.isEmpty())
        settings->remove(QLatin1String("General/OverrideLanguage"));
    else
        settings->setValue(QLatin1String("General/OverrideLanguage"), locale);
}

} // namespace Internal
} // namespace Core
