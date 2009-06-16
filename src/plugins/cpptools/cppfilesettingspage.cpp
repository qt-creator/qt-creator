/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "cppfilesettingspage.h"
#include "cpptoolsconstants.h"
#include "ui_cppfilesettingspage.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

static const char *headerSuffixKeyC = "HeaderSuffix";
static const char *sourceSuffixKeyC = "SourceSuffix";

namespace CppTools {
namespace Internal {

CppFileSettings::CppFileSettings() :
    lowerCaseFiles(false)
{
}

void CppFileSettings::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));
    s->setValue(QLatin1String(headerSuffixKeyC), headerSuffix);
    s->setValue(QLatin1String(sourceSuffixKeyC), sourceSuffix);
    s->setValue(QLatin1String(Constants::LOWERCASE_CPPFILES_KEY), lowerCaseFiles);
    s->endGroup();
}

void CppFileSettings::fromSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));
    headerSuffix= s->value(QLatin1String(headerSuffixKeyC), QLatin1String("h")).toString();
    sourceSuffix = s->value(QLatin1String(sourceSuffixKeyC), QLatin1String("cpp")).toString();
    const bool lowerCaseDefault = Constants::lowerCaseFilesDefault;
    lowerCaseFiles = s->value(QLatin1String(Constants::LOWERCASE_CPPFILES_KEY), QVariant(lowerCaseDefault)).toBool();
    s->endGroup();
}

bool CppFileSettings::applySuffixesToMimeDB()
{
    Core::MimeDatabase *mdb = Core::ICore::instance()->mimeDatabase();
    return mdb->setPreferredSuffix(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE), sourceSuffix)
            && mdb->setPreferredSuffix(QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE), headerSuffix);
}

bool CppFileSettings::equals(const CppFileSettings &rhs) const
{
    return lowerCaseFiles == rhs.lowerCaseFiles
           && headerSuffix == rhs.headerSuffix
           && sourceSuffix == rhs.sourceSuffix;
}

// ------------------ CppFileSettingsWidget

CppFileSettingsWidget::CppFileSettingsWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::CppFileSettingsPage)
{
    m_ui->setupUi(this);
    const Core::MimeDatabase *mdb = Core::ICore::instance()->mimeDatabase();
    // populate suffix combos
    if (const Core::MimeType sourceMt = mdb->findByType(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)))
        foreach (const QString &suffix, sourceMt.suffixes())
            m_ui->sourceSuffixComboBox->addItem(suffix);

    if (const Core::MimeType headerMt = mdb->findByType(QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE)))
        foreach (const QString &suffix, headerMt.suffixes())
            m_ui->headerSuffixComboBox->addItem(suffix);
}

CppFileSettingsWidget::~CppFileSettingsWidget()
{
    delete m_ui;
}

CppFileSettings CppFileSettingsWidget::settings() const
{
    CppFileSettings rc;
    rc.lowerCaseFiles = m_ui->lowerCaseFileNamesCheckBox->isChecked();
    rc.headerSuffix = m_ui->headerSuffixComboBox->currentText();
    rc.sourceSuffix = m_ui->sourceSuffixComboBox->currentText();
    return rc;
}

static inline void setComboText(QComboBox *cb, const QString &text, int defaultIndex = 0)
{
    const int index = cb->findText(text);
    cb->setCurrentIndex(index == -1 ? defaultIndex: index);
}

void CppFileSettingsWidget::setSettings(const CppFileSettings &s)
{
    m_ui->lowerCaseFileNamesCheckBox->setChecked(s.lowerCaseFiles);
    setComboText(m_ui->headerSuffixComboBox, s.headerSuffix);
    setComboText(m_ui->sourceSuffixComboBox, s.sourceSuffix);
}

// --------------- CppFileSettingsPage
CppFileSettingsPage::CppFileSettingsPage(QSharedPointer<CppFileSettings> &settings,
                                         QObject *parent) :
    Core::IOptionsPage(parent),
    m_settings(settings)
{
}

CppFileSettingsPage::~CppFileSettingsPage()
{
}

QString CppFileSettingsPage::id() const
{
    return QLatin1String(Constants::CPP_SETTINGS_ID);
}

QString CppFileSettingsPage::trName() const
{
    return QCoreApplication::translate("CppTools", Constants::CPP_SETTINGS_ID);
}

QString CppFileSettingsPage::category() const
{
    return QLatin1String(Constants::CPP_SETTINGS_CATEGORY);
}

QString CppFileSettingsPage::trCategory() const
{
    return QCoreApplication::translate("CppTools", Constants::CPP_SETTINGS_CATEGORY);
}

QWidget *CppFileSettingsPage::createPage(QWidget *parent)
{

    m_widget = new CppFileSettingsWidget(parent);
    m_widget->setSettings(*m_settings);
    return m_widget;
}

void CppFileSettingsPage::apply()
{
    if (m_widget) {
        const CppFileSettings newSettings = m_widget->settings();
        if (newSettings != *m_settings) {
            *m_settings = newSettings;
            m_settings->toSettings(Core::ICore::instance()->settings());
            m_settings->applySuffixesToMimeDB();
        }
    }
}

}
}

