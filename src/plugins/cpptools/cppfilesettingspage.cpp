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

#include "cppfilesettingspage.h"

#include "cpptoolsconstants.h"
#include "cpptoolsplugin.h"
#include <ui_cppfilesettingspage.h>

#include <app/app_version.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <cppeditor/cppeditorconstants.h>

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/stringutils.h>

#include <QCoreApplication>
#include <QDate>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QLocale>
#include <QSettings>
#include <QTextCodec>
#include <QTextStream>

namespace CppTools {
namespace Internal {

const char headerPrefixesKeyC[] = "HeaderPrefixes";
const char sourcePrefixesKeyC[] = "SourcePrefixes";
const char headerSuffixKeyC[] = "HeaderSuffix";
const char sourceSuffixKeyC[] = "SourceSuffix";
const char headerSearchPathsKeyC[] = "HeaderSearchPaths";
const char sourceSearchPathsKeyC[] = "SourceSearchPaths";
const char headerPragmaOnceC[] = "HeaderPragmaOnce";
const char licenseTemplatePathKeyC[] = "LicenseTemplate";

const char *licenseTemplateTemplate = QT_TRANSLATE_NOOP("CppTools::Internal::CppFileSettingsWidget",
"/**************************************************************************\n"
"** %1 license header template\n"
"**   Special keywords: %USER% %DATE% %YEAR%\n"
"**   Environment variables: %$VARIABLE%\n"
"**   To protect a percent sign, use '%%'.\n"
"**************************************************************************/\n");

void CppFileSettings::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));
    s->setValue(QLatin1String(headerPrefixesKeyC), headerPrefixes);
    s->setValue(QLatin1String(sourcePrefixesKeyC), sourcePrefixes);
    s->setValue(QLatin1String(headerSuffixKeyC), headerSuffix);
    s->setValue(QLatin1String(sourceSuffixKeyC), sourceSuffix);
    s->setValue(QLatin1String(headerSearchPathsKeyC), headerSearchPaths);
    s->setValue(QLatin1String(sourceSearchPathsKeyC), sourceSearchPaths);
    s->setValue(QLatin1String(Constants::LOWERCASE_CPPFILES_KEY), lowerCaseFiles);
    s->setValue(QLatin1String(headerPragmaOnceC), headerPragmaOnce);
    s->setValue(QLatin1String(licenseTemplatePathKeyC), licenseTemplatePath);
    s->endGroup();
}

void CppFileSettings::fromSettings(QSettings *s)
{
    const QStringList defaultHeaderSearchPaths
            = QStringList({"include", "Include", QDir::toNativeSeparators("../include"),
                           QDir::toNativeSeparators("../Include")});
    const QStringList defaultSourceSearchPaths
            = QStringList({QDir::toNativeSeparators("../src"), QDir::toNativeSeparators("../Src"),
                           ".."});
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));
    headerPrefixes = s->value(QLatin1String(headerPrefixesKeyC)).toStringList();
    sourcePrefixes = s->value(QLatin1String(sourcePrefixesKeyC)).toStringList();
    headerSuffix = s->value(QLatin1String(headerSuffixKeyC), QLatin1String("h")).toString();
    sourceSuffix = s->value(QLatin1String(sourceSuffixKeyC), QLatin1String("cpp")).toString();
    headerSearchPaths = s->value(QLatin1String(headerSearchPathsKeyC), defaultHeaderSearchPaths)
            .toStringList();
    sourceSearchPaths = s->value(QLatin1String(sourceSearchPathsKeyC), defaultSourceSearchPaths)
            .toStringList();
    const bool lowerCaseDefault = Constants::lowerCaseFilesDefault;
    lowerCaseFiles = s->value(QLatin1String(Constants::LOWERCASE_CPPFILES_KEY), QVariant(lowerCaseDefault)).toBool();
    headerPragmaOnce = s->value(headerPragmaOnceC, headerPragmaOnce).toBool();
    licenseTemplatePath = s->value(QLatin1String(licenseTemplatePathKeyC), QString()).toString();
    s->endGroup();
}

bool CppFileSettings::applySuffixesToMimeDB()
{
    Utils::MimeType mt;
    mt = Utils::mimeTypeForName(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE));
    if (!mt.isValid())
        return false;
    mt.setPreferredSuffix(sourceSuffix);
    mt = Utils::mimeTypeForName(QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE));
    if (!mt.isValid())
        return false;
    mt.setPreferredSuffix(headerSuffix);
    return true;
}

bool CppFileSettings::equals(const CppFileSettings &rhs) const
{
    return lowerCaseFiles == rhs.lowerCaseFiles
           && headerPragmaOnce == rhs.headerPragmaOnce
           && headerPrefixes == rhs.headerPrefixes
           && sourcePrefixes == rhs.sourcePrefixes
           && headerSuffix == rhs.headerSuffix
           && sourceSuffix == rhs.sourceSuffix
           && headerSearchPaths == rhs.headerSearchPaths
           && sourceSearchPaths == rhs.sourceSearchPaths
           && licenseTemplatePath == rhs.licenseTemplatePath;
}

// Replacements of special license template keywords.
static bool keyWordReplacement(const QString &keyWord,
                               QString *value)
{
    if (keyWord == QLatin1String("%YEAR%")) {
        *value = QLatin1String("%{CurrentDate:yyyy}");
        return true;
    }
    if (keyWord == QLatin1String("%MONTH%")) {
        *value = QLatin1String("%{CurrentDate:M}");
        return true;
    }
    if (keyWord == QLatin1String("%DAY%")) {
        *value = QLatin1String("%{CurrentDate:d}");
        return true;
    }
    if (keyWord == QLatin1String("%CLASS%")) {
        *value = QLatin1String("%{Cpp:License:ClassName}");
        return true;
    }
    if (keyWord == QLatin1String("%FILENAME%")) {
        *value = QLatin1String("%{Cpp:License:FileName}");
        return true;
    }
    if (keyWord == QLatin1String("%DATE%")) {
        static QString format;
        // ensure a format with 4 year digits. Some have locales have 2.
        if (format.isEmpty()) {
            QLocale loc;
            format = loc.dateFormat(QLocale::ShortFormat);
            const QChar ypsilon = QLatin1Char('y');
            if (format.count(ypsilon) == 2)
                format.insert(format.indexOf(ypsilon), QString(2, ypsilon));
            format.replace('/', "\\/");
        }
        *value = QString::fromLatin1("%{CurrentDate:") + format + QLatin1Char('}');
        return true;
    }
    if (keyWord == QLatin1String("%USER%")) {
        *value = Utils::HostOsInfo::isWindowsHost() ? QLatin1String("%{Env:USERNAME}")
                                                    : QLatin1String("%{Env:USER}");
        return true;
    }
    // Environment variables (for example '%$EMAIL%').
    if (keyWord.startsWith(QLatin1String("%$"))) {
        const QString varName = keyWord.mid(2, keyWord.size() - 3);
        *value = QString::fromLatin1("%{Env:") + varName + QLatin1Char('}');
        return true;
    }
    return false;
}

// Parse a license template, scan for %KEYWORD% and replace if known.
// Replace '%%' by '%'.
static void parseLicenseTemplatePlaceholders(QString *t)
{
    int pos = 0;
    const QChar placeHolder = QLatin1Char('%');
    do {
        const int placeHolderPos = t->indexOf(placeHolder, pos);
        if (placeHolderPos == -1)
            break;
        const int endPlaceHolderPos = t->indexOf(placeHolder, placeHolderPos + 1);
        if (endPlaceHolderPos == -1)
            break;
        if (endPlaceHolderPos == placeHolderPos + 1) { // '%%' -> '%'
            t->remove(placeHolderPos, 1);
            pos = placeHolderPos + 1;
        } else {
            const QString keyWord = t->mid(placeHolderPos, endPlaceHolderPos + 1 - placeHolderPos);
            QString replacement;
            if (keyWordReplacement(keyWord, &replacement)) {
                t->replace(placeHolderPos, keyWord.size(), replacement);
                pos = placeHolderPos + replacement.size();
            } else {
                // Leave invalid keywords as is.
                pos = endPlaceHolderPos + 1;
            }
        }
    } while (pos < t->size());

}

// Convenience that returns the formatted license template.
QString CppFileSettings::licenseTemplate()
{
    const QSettings *s = Core::ICore::settings();
    QString key = QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP);
    key += QLatin1Char('/');
    key += QLatin1String(licenseTemplatePathKeyC);
    const QString path = s->value(key, QString()).toString();
    if (path.isEmpty())
        return QString();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        qWarning("Unable to open the license template %s: %s", qPrintable(path), qPrintable(file.errorString()));
        return QString();
    }

    QTextStream licenseStream(&file);
    licenseStream.setAutoDetectUnicode(true);
    QString license = licenseStream.readAll();

    parseLicenseTemplatePlaceholders(&license);

    // Ensure at least one newline at the end of the license template to separate it from the code
    const QChar newLine = QLatin1Char('\n');
    if (!license.endsWith(newLine))
        license += newLine;

    return license;
}

// ------------------ CppFileSettingsWidget

class CppFileSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(CppTools::Internal::CppFileSettingsWidget)

public:
    explicit CppFileSettingsWidget(CppFileSettings *settings);

    void apply() final;

    void setSettings(const CppFileSettings &s);

private:
    void slotEdit();
    QString licenseTemplatePath() const;
    void setLicenseTemplatePath(const QString &);

    Ui::CppFileSettingsPage m_ui;
    CppFileSettings *m_settings = nullptr;
};

CppFileSettingsWidget::CppFileSettingsWidget(CppFileSettings *settings)
    : m_settings(settings)
{
    m_ui.setupUi(this);
    // populate suffix combos
    const Utils::MimeType sourceMt = Utils::mimeTypeForName(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE));
    if (sourceMt.isValid()) {
        foreach (const QString &suffix, sourceMt.suffixes())
            m_ui.sourceSuffixComboBox->addItem(suffix);
    }

    const Utils::MimeType headerMt = Utils::mimeTypeForName(QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE));
    if (headerMt.isValid()) {
        foreach (const QString &suffix, headerMt.suffixes())
            m_ui.headerSuffixComboBox->addItem(suffix);
    }
    m_ui.licenseTemplatePathChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui.licenseTemplatePathChooser->setHistoryCompleter(QLatin1String("Cpp.LicenseTemplate.History"));
    m_ui.licenseTemplatePathChooser->addButton(tr("Edit..."), this, [this] { slotEdit(); });

    setSettings(*m_settings);
}

QString CppFileSettingsWidget::licenseTemplatePath() const
{
    return m_ui.licenseTemplatePathChooser->filePath().toString();
}

void CppFileSettingsWidget::setLicenseTemplatePath(const QString &lp)
{
    m_ui.licenseTemplatePathChooser->setPath(lp);
}

static QStringList trimmedPaths(const QString &paths)
{
    QStringList res;
    foreach (const QString &path, paths.split(QLatin1Char(','), Qt::SkipEmptyParts))
        res << path.trimmed();
    return res;
}

void CppFileSettingsWidget::apply()
{
    CppFileSettings rc;
    rc.lowerCaseFiles = m_ui.lowerCaseFileNamesCheckBox->isChecked();
    rc.headerPragmaOnce = m_ui.headerPragmaOnceCheckBox->isChecked();
    rc.headerPrefixes = trimmedPaths(m_ui.headerPrefixesEdit->text());
    rc.sourcePrefixes = trimmedPaths(m_ui.sourcePrefixesEdit->text());
    rc.headerSuffix = m_ui.headerSuffixComboBox->currentText();
    rc.sourceSuffix = m_ui.sourceSuffixComboBox->currentText();
    rc.headerSearchPaths = trimmedPaths(m_ui.headerSearchPathsEdit->text());
    rc.sourceSearchPaths = trimmedPaths(m_ui.sourceSearchPathsEdit->text());
    rc.licenseTemplatePath = licenseTemplatePath();

    if (rc == *m_settings)
        return;

    *m_settings = rc;
    m_settings->toSettings(Core::ICore::settings());
    m_settings->applySuffixesToMimeDB();
    CppToolsPlugin::clearHeaderSourceCache();
}

static inline void setComboText(QComboBox *cb, const QString &text, int defaultIndex = 0)
{
    const int index = cb->findText(text);
    cb->setCurrentIndex(index == -1 ? defaultIndex: index);
}

void CppFileSettingsWidget::setSettings(const CppFileSettings &s)
{
    const QChar comma = QLatin1Char(',');
    m_ui.lowerCaseFileNamesCheckBox->setChecked(s.lowerCaseFiles);
    m_ui.headerPragmaOnceCheckBox->setChecked(s.headerPragmaOnce);
    m_ui.headerPrefixesEdit->setText(s.headerPrefixes.join(comma));
    m_ui.sourcePrefixesEdit->setText(s.sourcePrefixes.join(comma));
    setComboText(m_ui.headerSuffixComboBox, s.headerSuffix);
    setComboText(m_ui.sourceSuffixComboBox, s.sourceSuffix);
    m_ui.headerSearchPathsEdit->setText(s.headerSearchPaths.join(comma));
    m_ui.sourceSearchPathsEdit->setText(s.sourceSearchPaths.join(comma));
    setLicenseTemplatePath(s.licenseTemplatePath);
}

void CppFileSettingsWidget::slotEdit()
{
    QString path = licenseTemplatePath();
    if (path.isEmpty()) {
        // Pick a file name and write new template, edit with C++
        path = QFileDialog::getSaveFileName(this, tr("Choose Location for New License Template File"));
        if (path.isEmpty())
            return;
        Utils::FileSaver saver(path, QIODevice::Text);
        saver.write(tr(licenseTemplateTemplate).arg(Core::Constants::IDE_DISPLAY_NAME).toUtf8());
        if (!saver.finalize(this))
            return;
        setLicenseTemplatePath(path);
    }
    // Edit (now) existing file with C++
    Core::EditorManager::openEditor(path, CppEditor::Constants::CPPEDITOR_ID);
}

// --------------- CppFileSettingsPage

CppFileSettingsPage::CppFileSettingsPage(CppFileSettings *settings)
{
    setId(Constants::CPP_FILE_SETTINGS_ID);
    setDisplayName(QCoreApplication::translate("CppTools", Constants::CPP_FILE_SETTINGS_NAME));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setWidgetCreator([settings] { return new CppFileSettingsWidget(settings); });
}

} // namespace Internal
} // namespace CppTools
