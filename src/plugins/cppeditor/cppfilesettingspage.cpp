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

#include "cppeditorplugin.h"
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

using namespace Utils;

namespace CppEditor::Internal {

const char headerPrefixesKeyC[] = "HeaderPrefixes";
const char sourcePrefixesKeyC[] = "SourcePrefixes";
const char headerSuffixKeyC[] = "HeaderSuffix";
const char sourceSuffixKeyC[] = "SourceSuffix";
const char headerSearchPathsKeyC[] = "HeaderSearchPaths";
const char sourceSearchPathsKeyC[] = "SourceSearchPaths";
const char headerPragmaOnceC[] = "HeaderPragmaOnce";
const char licenseTemplatePathKeyC[] = "LicenseTemplate";

const char *licenseTemplateTemplate = QT_TRANSLATE_NOOP("CppEditor::Internal::CppFileSettingsWidget",
"/**************************************************************************\n"
"** %1 license header template\n"
"**   Special keywords: %USER% %DATE% %YEAR%\n"
"**   Environment variables: %$VARIABLE%\n"
"**   To protect a percent sign, use '%%'.\n"
"**************************************************************************/\n");

void CppFileSettings::toSettings(QSettings *s) const
{
    using Utils::QtcSettings;
    const CppFileSettings def;
    s->beginGroup(Constants::CPPEDITOR_SETTINGSGROUP);
    QtcSettings::setValueWithDefault(s, headerPrefixesKeyC, headerPrefixes, def.headerPrefixes);
    QtcSettings::setValueWithDefault(s, sourcePrefixesKeyC, sourcePrefixes, def.sourcePrefixes);
    QtcSettings::setValueWithDefault(s, headerSuffixKeyC, headerSuffix, def.headerSuffix);
    QtcSettings::setValueWithDefault(s, sourceSuffixKeyC, sourceSuffix, def.sourceSuffix);
    QtcSettings::setValueWithDefault(s,
                                     headerSearchPathsKeyC,
                                     headerSearchPaths,
                                     def.headerSearchPaths);
    QtcSettings::setValueWithDefault(s,
                                     sourceSearchPathsKeyC,
                                     sourceSearchPaths,
                                     def.sourceSearchPaths);
    QtcSettings::setValueWithDefault(s,
                                     Constants::LOWERCASE_CPPFILES_KEY,
                                     lowerCaseFiles,
                                     def.lowerCaseFiles);
    QtcSettings::setValueWithDefault(s, headerPragmaOnceC, headerPragmaOnce, def.headerPragmaOnce);
    QtcSettings::setValueWithDefault(s,
                                     licenseTemplatePathKeyC,
                                     licenseTemplatePath,
                                     def.licenseTemplatePath);
    s->endGroup();
}

void CppFileSettings::fromSettings(QSettings *s)
{
    const CppFileSettings def;
    s->beginGroup(Constants::CPPEDITOR_SETTINGSGROUP);
    headerPrefixes = s->value(headerPrefixesKeyC, def.headerPrefixes).toStringList();
    sourcePrefixes = s->value(sourcePrefixesKeyC, def.sourcePrefixes).toStringList();
    headerSuffix = s->value(headerSuffixKeyC, def.headerSuffix).toString();
    sourceSuffix = s->value(sourceSuffixKeyC, def.sourceSuffix).toString();
    headerSearchPaths = s->value(headerSearchPathsKeyC, def.headerSearchPaths).toStringList();
    sourceSearchPaths = s->value(sourceSearchPathsKeyC, def.sourceSearchPaths).toStringList();
    lowerCaseFiles = s->value(Constants::LOWERCASE_CPPFILES_KEY, def.lowerCaseFiles).toBool();
    headerPragmaOnce = s->value(headerPragmaOnceC, def.headerPragmaOnce).toBool();
    licenseTemplatePath = s->value(licenseTemplatePathKeyC, def.licenseTemplatePath).toString();
    s->endGroup();
}

bool CppFileSettings::applySuffixesToMimeDB()
{
    Utils::MimeType mt;
    mt = Utils::mimeTypeForName(QString(Constants::CPP_SOURCE_MIMETYPE));
    if (!mt.isValid())
        return false;
    mt.setPreferredSuffix(sourceSuffix);
    mt = Utils::mimeTypeForName(QString(Constants::CPP_HEADER_MIMETYPE));
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
    if (keyWord == QString("%YEAR%")) {
        *value = QString("%{CurrentDate:yyyy}");
        return true;
    }
    if (keyWord == QString("%MONTH%")) {
        *value = QString("%{CurrentDate:M}");
        return true;
    }
    if (keyWord == QString("%DAY%")) {
        *value = QString("%{CurrentDate:d}");
        return true;
    }
    if (keyWord == QString("%CLASS%")) {
        *value = QString("%{Cpp:License:ClassName}");
        return true;
    }
    if (keyWord == QString("%FILENAME%")) {
        *value = QString("%{Cpp:License:FileName}");
        return true;
    }
    if (keyWord == QString("%DATE%")) {
        static QString format;
        // ensure a format with 4 year digits. Some have locales have 2.
        if (format.isEmpty()) {
            QLocale loc;
            format = loc.dateFormat(QLocale::ShortFormat);
            const QChar ypsilon('y');
            if (format.count(ypsilon) == 2)
                format.insert(format.indexOf(ypsilon), QString(2, ypsilon));
            format.replace('/', "\\/");
        }
        *value = QString("%{CurrentDate:") + format + QChar('}');
        return true;
    }
    if (keyWord == QString("%USER%")) {
        *value = Utils::HostOsInfo::isWindowsHost() ? QString("%{Env:USERNAME}")
                                                    : QString("%{Env:USER}");
        return true;
    }
    // Environment variables (for example '%$EMAIL%').
    if (keyWord.startsWith(QString("%$"))) {
        const QString varName = keyWord.mid(2, keyWord.size() - 3);
        *value = QString("%{Env:") + varName + QChar('}');
        return true;
    }
    return false;
}

// Parse a license template, scan for %KEYWORD% and replace if known.
// Replace '%%' by '%'.
static void parseLicenseTemplatePlaceholders(QString *t)
{
    int pos = 0;
    const QChar placeHolder('%');
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
    QString key(Constants::CPPEDITOR_SETTINGSGROUP);
    key += QChar('/');
    key += QString(licenseTemplatePathKeyC);
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
    const QChar newLine = QChar('\n');
    if (!license.endsWith(newLine))
        license += newLine;

    return license;
}

// ------------------ CppFileSettingsWidget

class CppFileSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(CppEditor::Internal::CppFileSettingsWidget)

public:
    explicit CppFileSettingsWidget(CppFileSettings *settings);

    void apply() final;

    void setSettings(const CppFileSettings &s);

private:
    void slotEdit();
    FilePath licenseTemplatePath() const;
    void setLicenseTemplatePath(const FilePath &);

    Ui::CppFileSettingsPage m_ui;
    CppFileSettings *m_settings = nullptr;
};

CppFileSettingsWidget::CppFileSettingsWidget(CppFileSettings *settings)
    : m_settings(settings)
{
    m_ui.setupUi(this);
    // populate suffix combos
    const Utils::MimeType sourceMt = Utils::mimeTypeForName(QString(Constants::CPP_SOURCE_MIMETYPE));
    if (sourceMt.isValid()) {
        foreach (const QString &suffix, sourceMt.suffixes())
            m_ui.sourceSuffixComboBox->addItem(suffix);
    }

    const Utils::MimeType headerMt = Utils::mimeTypeForName(QString(Constants::CPP_HEADER_MIMETYPE));
    if (headerMt.isValid()) {
        foreach (const QString &suffix, headerMt.suffixes())
            m_ui.headerSuffixComboBox->addItem(suffix);
    }
    m_ui.licenseTemplatePathChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui.licenseTemplatePathChooser->setHistoryCompleter(QString("Cpp.LicenseTemplate.History"));
    m_ui.licenseTemplatePathChooser->addButton(tr("Edit..."), this, [this] { slotEdit(); });

    setSettings(*m_settings);
}

FilePath CppFileSettingsWidget::licenseTemplatePath() const
{
    return m_ui.licenseTemplatePathChooser->filePath();
}

void CppFileSettingsWidget::setLicenseTemplatePath(const FilePath &lp)
{
    m_ui.licenseTemplatePathChooser->setFilePath(lp);
}

static QStringList trimmedPaths(const QString &paths)
{
    QStringList res;
    foreach (const QString &path, paths.split(QChar(','), Qt::SkipEmptyParts))
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
    rc.licenseTemplatePath = licenseTemplatePath().toString();

    if (rc == *m_settings)
        return;

    *m_settings = rc;
    m_settings->toSettings(Core::ICore::settings());
    m_settings->applySuffixesToMimeDB();
    CppEditorPlugin::clearHeaderSourceCache();
}

static inline void setComboText(QComboBox *cb, const QString &text, int defaultIndex = 0)
{
    const int index = cb->findText(text);
    cb->setCurrentIndex(index == -1 ? defaultIndex: index);
}

void CppFileSettingsWidget::setSettings(const CppFileSettings &s)
{
    const QChar comma(',');
    m_ui.lowerCaseFileNamesCheckBox->setChecked(s.lowerCaseFiles);
    m_ui.headerPragmaOnceCheckBox->setChecked(s.headerPragmaOnce);
    m_ui.headerPrefixesEdit->setText(s.headerPrefixes.join(comma));
    m_ui.sourcePrefixesEdit->setText(s.sourcePrefixes.join(comma));
    setComboText(m_ui.headerSuffixComboBox, s.headerSuffix);
    setComboText(m_ui.sourceSuffixComboBox, s.sourceSuffix);
    m_ui.headerSearchPathsEdit->setText(s.headerSearchPaths.join(comma));
    m_ui.sourceSearchPathsEdit->setText(s.sourceSearchPaths.join(comma));
    setLicenseTemplatePath(FilePath::fromString(s.licenseTemplatePath));
}

void CppFileSettingsWidget::slotEdit()
{
    FilePath path = licenseTemplatePath();
    if (path.isEmpty()) {
        // Pick a file name and write new template, edit with C++
        path = FileUtils::getSaveFilePath(this, tr("Choose Location for New License Template File"));
        if (path.isEmpty())
            return;
        FileSaver saver(path, QIODevice::Text);
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
    setDisplayName(QCoreApplication::translate("CppEditor", Constants::CPP_FILE_SETTINGS_NAME));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setWidgetCreator([settings] { return new CppFileSettingsWidget(settings); });
}

} // namespace CppEditor::Internal
