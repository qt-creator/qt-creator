// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppfilesettingspage.h"

#include "cppeditorplugin.h"

#include <app/app_version.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeutils.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QFile>
#include <QLineEdit>
#include <QLocale>
#include <QSettings>
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
    mt = Utils::mimeTypeForName(QLatin1String(Constants::CPP_SOURCE_MIMETYPE));
    if (!mt.isValid())
        return false;
    mt.setPreferredSuffix(sourceSuffix);
    mt = Utils::mimeTypeForName(QLatin1String(Constants::CPP_HEADER_MIMETYPE));
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
    QString key = QLatin1String(Constants::CPPEDITOR_SETTINGSGROUP);
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
    Q_DECLARE_TR_FUNCTIONS(CppEditor::Internal::CppFileSettingsPage)

public:
    explicit CppFileSettingsWidget(CppFileSettings *settings);

    void apply() final;
    void setSettings(const CppFileSettings &s);

private:
    void slotEdit();
    FilePath licenseTemplatePath() const;
    void setLicenseTemplatePath(const FilePath &);

    CppFileSettings *m_settings = nullptr;

    QComboBox *m_headerSuffixComboBox = nullptr;
    QLineEdit *m_headerSearchPathsEdit = nullptr;
    QLineEdit *m_headerPrefixesEdit = nullptr;
    QCheckBox *m_headerPragmaOnceCheckBox = nullptr;
    QComboBox *m_sourceSuffixComboBox = nullptr;
    QLineEdit *m_sourceSearchPathsEdit = nullptr;
    QLineEdit *m_sourcePrefixesEdit = nullptr;
    QCheckBox *m_lowerCaseFileNamesCheckBox = nullptr;
    PathChooser *m_licenseTemplatePathChooser = nullptr;
};

CppFileSettingsWidget::CppFileSettingsWidget(CppFileSettings *settings)
    : m_settings(settings)
    , m_headerSuffixComboBox(new QComboBox)
    , m_headerSearchPathsEdit(new QLineEdit)
    , m_headerPrefixesEdit(new QLineEdit)
    , m_headerPragmaOnceCheckBox(new QCheckBox(tr("Use \"#pragma once\" instead of \"#ifndef\" guards")))
    , m_sourceSuffixComboBox(new QComboBox)
    , m_sourceSearchPathsEdit(new QLineEdit)
    , m_sourcePrefixesEdit(new QLineEdit)
    , m_lowerCaseFileNamesCheckBox(new QCheckBox(tr("&Lower case file names")))
    , m_licenseTemplatePathChooser(new PathChooser)
{
    m_headerSearchPathsEdit->setToolTip(tr("Comma-separated list of header paths.\n"
        "\n"
        "Paths can be absolute or relative to the directory of the current open document.\n"
        "\n"
        "These paths are used in addition to current directory on Switch Header/Source."));
    m_headerPrefixesEdit->setToolTip(tr("Comma-separated list of header prefixes.\n"
        "\n"
        "These prefixes are used in addition to current file name on Switch Header/Source."));
    m_headerPragmaOnceCheckBox->setToolTip(
        tr("Uses \"#pragma once\" instead of \"#ifndef\" include guards."));
    m_sourceSearchPathsEdit->setToolTip(tr("Comma-separated list of source paths.\n"
        "\n"
        "Paths can be absolute or relative to the directory of the current open document.\n"
        "\n"
        "These paths are used in addition to current directory on Switch Header/Source."));
    m_sourcePrefixesEdit->setToolTip(tr("Comma-separated list of source prefixes.\n"
        "\n"
        "These prefixes are used in addition to current file name on Switch Header/Source."));

    using namespace Layouting;

    Column {
        Group {
            title("Headers"),
            Form {
                tr("&Suffix:"), m_headerSuffixComboBox, st, br,
                tr("S&earch paths:"), m_headerSearchPathsEdit, br,
                tr("&Prefixes:"), m_headerPrefixesEdit, br,
                tr("Include guards"), m_headerPragmaOnceCheckBox
            }
        },
        Group {
            title("Sources"),
            Form {
                tr("S&uffix:"), m_sourceSuffixComboBox, st, br,
                tr("Se&arch paths:"), m_sourceSearchPathsEdit, br,
                tr("P&refixes:"), m_sourcePrefixesEdit
            }
        },
        m_lowerCaseFileNamesCheckBox,
        Form {
            tr("License &template:"), m_licenseTemplatePathChooser
        },
        st
    }.attachTo(this);

    // populate suffix combos
    const MimeType sourceMt = Utils::mimeTypeForName(QLatin1String(Constants::CPP_SOURCE_MIMETYPE));
    if (sourceMt.isValid()) {
        const QStringList suffixes = sourceMt.suffixes();
        for (const QString &suffix : suffixes)
            m_sourceSuffixComboBox->addItem(suffix);
    }

    const MimeType headerMt = Utils::mimeTypeForName(QLatin1String(Constants::CPP_HEADER_MIMETYPE));
    if (headerMt.isValid()) {
        const QStringList suffixes = headerMt.suffixes();
        for (const QString &suffix : suffixes)
            m_headerSuffixComboBox->addItem(suffix);
    }
    m_licenseTemplatePathChooser->setExpectedKind(PathChooser::File);
    m_licenseTemplatePathChooser->setHistoryCompleter(QLatin1String("Cpp.LicenseTemplate.History"));
    m_licenseTemplatePathChooser->addButton(tr("Edit..."), this, [this] { slotEdit(); });

    setSettings(*m_settings);
}

FilePath CppFileSettingsWidget::licenseTemplatePath() const
{
    return m_licenseTemplatePathChooser->filePath();
}

void CppFileSettingsWidget::setLicenseTemplatePath(const FilePath &lp)
{
    m_licenseTemplatePathChooser->setFilePath(lp);
}

static QStringList trimmedPaths(const QString &paths)
{
    QStringList res;
    for (const QString &path : paths.split(QLatin1Char(','), Qt::SkipEmptyParts))
        res << path.trimmed();
    return res;
}

void CppFileSettingsWidget::apply()
{
    CppFileSettings rc;
    rc.lowerCaseFiles = m_lowerCaseFileNamesCheckBox->isChecked();
    rc.headerPragmaOnce = m_headerPragmaOnceCheckBox->isChecked();
    rc.headerPrefixes = trimmedPaths(m_headerPrefixesEdit->text());
    rc.sourcePrefixes = trimmedPaths(m_sourcePrefixesEdit->text());
    rc.headerSuffix = m_headerSuffixComboBox->currentText();
    rc.sourceSuffix = m_sourceSuffixComboBox->currentText();
    rc.headerSearchPaths = trimmedPaths(m_headerSearchPathsEdit->text());
    rc.sourceSearchPaths = trimmedPaths(m_sourceSearchPathsEdit->text());
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
    const QChar comma = QLatin1Char(',');
    m_lowerCaseFileNamesCheckBox->setChecked(s.lowerCaseFiles);
    m_headerPragmaOnceCheckBox->setChecked(s.headerPragmaOnce);
    m_headerPrefixesEdit->setText(s.headerPrefixes.join(comma));
    m_sourcePrefixesEdit->setText(s.sourcePrefixes.join(comma));
    setComboText(m_headerSuffixComboBox, s.headerSuffix);
    setComboText(m_sourceSuffixComboBox, s.sourceSuffix);
    m_headerSearchPathsEdit->setText(s.headerSearchPaths.join(comma));
    m_sourceSearchPathsEdit->setText(s.sourceSearchPaths.join(comma));
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
