// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppfilesettingspage.h"

#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cppheadersource.h"

#include <extensionsystem/iplugin.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>

#include <utils/aspects.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcsettings.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QGuiApplication>
#include <QLineEdit>
#include <QLocale>
#include <QTextStream>
#include <QVBoxLayout>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor::Internal {
namespace {
class HeaderGuardExpander : public MacroExpander
{
public:
    HeaderGuardExpander(const FilePath &filePath) : m_filePath(filePath)
    {
        setDisplayName(Tr::tr("Header File Variables"));
        registerFileVariables("Header", Tr::tr("Header file"), [this] {
            return m_filePath;
        });
    }

private:
    const FilePath m_filePath;
};
} // namespace

const char projectSettingsKeyC[] = "CppEditorFileNames";
const char useGlobalKeyC[] = "UseGlobal";


const char *licenseTemplateTemplate = QT_TRANSLATE_NOOP("QtC::CppEditor",
"/**************************************************************************\n"
"** %1 license header template\n"
"**   Special keywords: %USER% %DATE% %YEAR%\n"
"**   Environment variables: %$VARIABLE%\n"
"**   To protect a percent sign, use '%%'.\n"
"**************************************************************************/\n");

static QStringList trimmedPaths(const QString &paths)
{
    QStringList res;
    for (const QString &path : paths.split(QLatin1Char(','), Qt::SkipEmptyParts))
        res << path.trimmed();
    return res;
}

CommaSeparatedStringsAspect::CommaSeparatedStringsAspect(AspectContainer *container)
    : StringAspect(container)
{
    setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
}

QStringList CommaSeparatedStringsAspect::operator()() const
{
    return trimmedPaths(StringAspect::value());
}

void CommaSeparatedStringsAspect::push(const QString &item)
{
    setValue(StringAspect::value() + ',' + item);
}

void CommaSeparatedStringsAspect::pop()
{
    QStringList list = operator()();
    if (!list.isEmpty())
        list.takeLast();
    setValue(list.join(','));
}

SuffixSelectionAspect::SuffixSelectionAspect(AspectContainer *container)
    : SelectionAspect(container)
{
    setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    setUseDataAsSavedValue();
}

QString SuffixSelectionAspect::operator()() const
{
    return stringValue();
}

void SuffixSelectionAspect::setMimeType(const QString &mimeType)
{
    const MimeType sourceMt = Utils::mimeTypeForName(mimeType);
    if (sourceMt.isValid()) {
        const QStringList suffixes = sourceMt.suffixes();
        for (const QString &suffix : suffixes)
           addOption({suffix, {}, suffix});
    }
}

CppFileSettings::CppFileSettings()
{
    setAutoApply(false);

    setSettingsGroup(Constants::CPPEDITOR_SETTINGSGROUP);

    headerPrefixes.setSettingsKey("HeaderPrefixes");
    headerPrefixes.setLabelText(Tr::tr("&Prefixes:"));
    headerPrefixes.setToolTip(Tr::tr("Comma-separated list of header prefixes.\n"
        "\n"
        "These prefixes are used in addition to current file name on Switch Header/Source."));

    headerSuffix.setSettingsKey("HeaderSuffix");
    headerSuffix.setMimeType(Utils::Constants::CPP_HEADER_MIMETYPE);
    headerSuffix.setDefaultValue("h");
    headerSuffix.setLabelText(Tr::tr("&Suffix:"));

    headerSearchPaths.setSettingsKey("HeaderSearchPaths");
    headerSearchPaths.setDefaultValue(
                QStringList{"include", "Include", QDir::toNativeSeparators("../include"),
                            QDir::toNativeSeparators("../Include")}.join(','));
    headerSearchPaths.setLabelText(Tr::tr("S&earch paths:"));
    headerSearchPaths.setToolTip(Tr::tr("Comma-separated list of header paths.\n"
        "\n"
        "Paths can be absolute or relative to the directory of the current open document.\n"
        "\n"
        "These paths are used in addition to current directory on Switch Header/Source."));

    sourcePrefixes.setSettingsKey("SourcePrefixes");
    sourcePrefixes.setLabelText(Tr::tr("P&refixes:"));
    sourcePrefixes.setToolTip(Tr::tr("Comma-separated list of source prefixes.\n"
        "\n"
        "These prefixes are used in addition to current file name on Switch Header/Source."));

    sourceSuffix.setSettingsKey("SourceSuffix");
    sourceSuffix.setMimeType(Utils::Constants::CPP_SOURCE_MIMETYPE);
    sourceSuffix.setDefaultValue("cpp");
    sourceSuffix.setLabelText(Tr::tr("S&uffix:"));

    sourceSearchPaths.setSettingsKey("SourceSearchPaths");
    sourceSearchPaths.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
    sourceSearchPaths.setDefaultValue(
            QStringList{QDir::toNativeSeparators("../src"), QDir::toNativeSeparators("../Src"), ".."}.join(','));
    sourceSearchPaths.setLabelText(Tr::tr("Se&arch paths:"));
    sourceSearchPaths.setToolTip(Tr::tr("Comma-separated list of source paths.\n"
        "\n"
        "Paths can be absolute or relative to the directory of the current open document.\n"
        "\n"
        "These paths are used in addition to current directory on Switch Header/Source."));

    licenseTemplatePath.setSettingsKey("LicenseTemplate");
    licenseTemplatePath.setExpectedKind(PathChooser::File);
    licenseTemplatePath.setHistoryCompleter("Cpp.LicenseTemplate.History");
    licenseTemplatePath.setLabelText(Tr::tr("License &template:"));

    headerGuardTemplate.setSettingsKey("HeaderGuardTemplate");
    headerGuardTemplate.setDefaultValue(
        "%{JS: '%{Header:FileName}'.toUpperCase().replace(/^[1-9]/, '_').replace(/[^_a-zA-Z1-9]/g, '_')}");
    headerGuardTemplate.setDisplayStyle(Utils::StringAspect::LineEditDisplay);

    headerPragmaOnce.setSettingsKey("HeaderPragmaOnce");
    headerPragmaOnce.setDefaultValue(false);
    headerPragmaOnce.setLabelText(Tr::tr("Use \"#pragma once\" instead"));
    headerPragmaOnce.setToolTip(
        Tr::tr("Uses \"#pragma once\" instead of \"#ifndef\" include guards."));

    lowerCaseFiles.setSettingsKey(Constants::LOWERCASE_CPPFILES_KEY);
    lowerCaseFiles.setDefaultValue(Constants::LOWERCASE_CPPFILES_DEFAULT);
    lowerCaseFiles.setLabelText(Tr::tr("&Lower case file names"));
}

static bool applySuffixes(const QString &sourceSuffix, const QString &headerSuffix)
{
    Utils::MimeType mt;
    mt = Utils::mimeTypeForName(QLatin1String(Utils::Constants::CPP_SOURCE_MIMETYPE));
    if (!mt.isValid())
        return false;
    mt.setPreferredSuffix(sourceSuffix);
    mt = Utils::mimeTypeForName(QLatin1String(Utils::Constants::CPP_HEADER_MIMETYPE));
    if (!mt.isValid())
        return false;
    mt.setPreferredSuffix(headerSuffix);
    return true;
}

void CppFileSettings::addMimeInitializer() const
{
    Utils::addMimeInitializer([sourceSuffix = sourceSuffix(), headerSuffix = headerSuffix()] {
        if (!applySuffixes(sourceSuffix, headerSuffix))
            qWarning("Unable to apply cpp suffixes to mime database (cpp mime types not found).\n");
    });
}

bool CppFileSettings::applySuffixesToMimeDB()
{
    return applySuffixes(sourceSuffix(), headerSuffix());
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
QString CppFileSettings::licenseTemplate() const
{
    if (licenseTemplatePath().isEmpty())
        return QString();
    QFile file(licenseTemplatePath().toFSPathString());
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        qWarning("Unable to open the license template %s: %s",
                 qPrintable(licenseTemplatePath().toUserOutput()),
                 qPrintable(file.errorString()));
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

QString CppFileSettings::headerGuard(const Utils::FilePath &headerFilePath) const
{
    return HeaderGuardExpander(headerFilePath).expand(headerGuardTemplate());
}

// ------------------ CppFileSettingsWidget

class CppFileSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_OBJECT

public:
    CppFileSettingsWidget() = default;

    void setup(CppFileSettings *settings);

    void apply() final
    {
        m_settings->apply();
        m_settings->writeSettings();
        m_settings->applySuffixesToMimeDB();
        clearHeaderSourceCache();
    }

    void cancel() final { m_settings->cancel(); }

    bool isDirty() const final { return m_settings->isDirty(); }

    void slotEdit();

    CppFileSettings *m_settings = nullptr;
    HeaderGuardExpander m_headerGuardExpander{{}};
};

void CppFileSettingsWidget::setup(CppFileSettings *settings)
{
    m_settings = settings;

    CppFileSettings &s = *settings;

    s.headerGuardTemplate.setMacroExpander(&m_headerGuardExpander);

    using namespace Layouting;
    Column {
        Group {
            title(Tr::tr("Headers")),
            Form {
                s.headerSuffix, st, br,
                s.headerSearchPaths, br,
                s.headerPrefixes, br,
                Tr::tr("Include guard template:"), s.headerPragmaOnce, s.headerGuardTemplate
            },
        },
        Group {
            title(Tr::tr("Sources")),
            Form {
                s.sourceSuffix, st, br,
                s.sourceSearchPaths, br,
                s.sourcePrefixes
            }
        },
        s.lowerCaseFiles,
        Row {
            s.licenseTemplatePath,
            PushButton {
                text(Tr::tr("Edit...")),
                onClicked(this, [this] { slotEdit(); })
            },
        },
        st
    }.attachTo(this);

    s.headerPragmaOnce.addOnVolatileValueChanged(this, [&s] {
        s.headerGuardTemplate.setEnabled(!s.headerPragmaOnce.volatileValue());
    });
    s.headerGuardTemplate.setEnabled(!s.headerPragmaOnce());

    connect(m_settings, &AspectContainer::volatileValueChanged, &checkSettingsDirty);
}

void CppFileSettingsWidget::slotEdit()
{
    FilePath path = FilePath::fromUserInput(m_settings->licenseTemplatePath.volatileValue());
    if (path.isEmpty()) {
        // Pick a file name and write new template, edit with C++
        path = FileUtils::getSaveFilePath(Tr::tr("Choose Location for New License Template File"));
        if (path.isEmpty())
            return;
        FileSaver saver(path, QIODevice::Text);
        saver.write(
            Tr::tr(licenseTemplateTemplate).arg(QGuiApplication::applicationDisplayName()).toUtf8());
        if (const Result<> res = saver.finalize(); !res) {
            FileUtils::showError(res.error());
            return;
        }
        m_settings->licenseTemplatePath.setValue(path.toUserOutput());
    }
    // Edit (now) existing file with C++
    Core::EditorManager::openEditor(path, CppEditor::Constants::CPPEDITOR_ID);
}

// CppFileSettingsPage

class CppFileSettingsPage final : public Core::IOptionsPage
{
public:
    CppFileSettingsPage()
    {
        setId(Constants::CPP_FILE_SETTINGS_ID);
        setDisplayName(Tr::tr("File Naming"));
        setCategory(Constants::CPP_SETTINGS_CATEGORY);
        setWidgetCreator([] {
            auto w = new CppFileSettingsWidget;
            w->setup(&globalCppFileSettings());
            return w;
        });
    }
};

// CppFileSettingsForProject

class CppFileSettingsForProjectWidget : public ProjectSettingsWidget
{
public:
    CppFileSettingsForProjectWidget(Project *project)
        : m_project(project)
    {
        setUseGlobalSettings(true);
        if (m_project) {
            const QVariant entry = m_project->namedSettings(projectSettingsKeyC);
            if (entry.isValid()) {
                const QVariantMap data = mapEntryFromStoreEntry(entry).toMap();
                setUseGlobalSettings(data.value(useGlobalKeyC, true).toBool());
                m_customSettings.fromMap(storeFromMap(data));
            }
        }

        m_customSettings.setAutoApply(true);

        setGlobalSettingsId(Constants::CPP_FILE_SETTINGS_ID);

        updateSubWidget();

        const auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(&m_widget);

        connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged, this, [this] {
            saveSettings();
            clearHeaderSourceCache();
            updateSubWidget();
        });

        connect(&m_customSettings, &AspectContainer::changed, this, [this] {
            saveSettings();
            clearHeaderSourceCache();
        });
    }

private:
    void updateSubWidget()
    {
        if (useGlobalSettings()) {
            m_widget.setup(&globalCppFileSettings());
            m_widget.setEnabled(false);
        } else {
            m_widget.setup(&m_customSettings);
            m_widget.setEnabled(true);
        }
    }

    void saveSettings()
    {
        if (!m_project)
            return;

        // Optimization: Don't save anything if the user never switched away from the default.
        if (useGlobalSettings() && !m_project->namedSettings(projectSettingsKeyC).isValid())
            return;

        Store store;
        m_customSettings.toMap(store);

        QVariantMap data = mapFromStore(store);
        data.insert(useGlobalKeyC, useGlobalSettings());

        m_project->setNamedSettings(projectSettingsKeyC, data);
    }

    Project * const m_project;
    CppFileSettingsWidget m_widget;
    CppFileSettings m_customSettings;
};

class CppFileSettingsProjectPanelFactory final : public ProjectPanelFactory
{
public:
    CppFileSettingsProjectPanelFactory()
    {
        setPriority(99);
        setDisplayName(Tr::tr("C++ File Naming"));
        setCreateWidgetFunction([](Project *project) {
            return new CppFileSettingsForProjectWidget(project);
        });
    }
};

CppFileSettings &globalCppFileSettings()
{
    // This is the global instance. There could be more.
    static CppFileSettings theGlobalCppFileSettings;
    return theGlobalCppFileSettings;
}

static CppFileSettings *findSettings(Project *project, CppFileSettings *customSettings)
{
    if (project) {
        const QVariant entry = project->namedSettings(projectSettingsKeyC);
        if (entry.isValid()) {
            const QVariantMap data = mapEntryFromStoreEntry(entry).toMap();
            const bool useGlobalSettings = data.value(useGlobalKeyC, true).toBool();
            if (!useGlobalSettings) {
                customSettings->fromMap(storeFromMap(data));
                return customSettings;
            }
        }
    }
    return &globalCppFileSettings();
}

CppFileSettingsData cppFileSettingsForProject(Project *project)
{
    CppFileSettings customSettings;
    CppFileSettings *settings = findSettings(project, &customSettings);

    CppFileSettingsData data;

    data.headerSuffix = settings->headerSuffix();
    data.headerPrefixes = settings->headerPrefixes();
    data.headerSearchPaths = settings->headerSearchPaths();

    data.sourceSuffix = settings->sourceSuffix();
    data.sourcePrefixes = settings->sourcePrefixes();
    data.sourceSearchPaths = settings->sourceSearchPaths();

    data.licenseTemplatePath= settings->licenseTemplatePath();
    data.headerGuardTemplate = settings->headerGuardTemplate();

    data.headerPragmaOnce = settings->headerPragmaOnce();
    data.lowerCaseFiles = settings->lowerCaseFiles();

    return data;
}

QString headerGuardForProject(Project *project, const FilePath &headerFilePath)
{
    CppFileSettings customSettings;
    CppFileSettings *settings = findSettings(project, &customSettings);

    return settings->headerGuard(headerFilePath);
}

QString licenseTemplateForProject(Project *project)
{
    CppFileSettings customSettings;
    CppFileSettings *settings = findSettings(project, &customSettings);

    return settings->licenseTemplate();
}

#ifdef WITH_TESTS
namespace {
class CppFileSettingsTest : public QObject
{
    Q_OBJECT

private slots:
    void testHeaderGuard_data()
    {
        QTest::addColumn<QString>("guardTemplate");
        QTest::addColumn<QString>("headerFile");
        QTest::addColumn<QString>("expectedGuard");

        QTest::newRow("default template, .h")
            << QString() << QString("/tmp/header.h") << QString("HEADER_H");
        QTest::newRow("default template, .hpp")
            << QString() << QString("/tmp/header.hpp") << QString("HEADER_HPP");
        QTest::newRow("default template, two extensions")
            << QString() << QString("/tmp/header.in.h") << QString("HEADER_IN_H");
        QTest::newRow("non-default template")
            << QString("%{JS: '%{Header:FilePath}'.toUpperCase().replace(/[.]/, '_').replace(/[/]/g, '_')}")
            << QString("/tmp/header.h") << QString("_TMP_HEADER_H");
    }

    void testHeaderGuard()
    {
        QFETCH(QString, guardTemplate);
        QFETCH(QString, headerFile);
        QFETCH(QString, expectedGuard);

        CppFileSettings settings;
        if (!guardTemplate.isEmpty())
            settings.headerGuardTemplate.setValue(guardTemplate);
        QCOMPARE(settings.headerGuard(FilePath::fromUserInput(headerFile)), expectedGuard);
    }
};
} // namespace
#endif // WITH_TESTS

void setupCppFileSettings(ExtensionSystem::IPlugin &plugin)
{
    static CppFileSettingsProjectPanelFactory theCppFileSettingsProjectPanelFactory;

    static CppFileSettingsPage theCppFileSettingsPage;

    globalCppFileSettings().readSettings();
    globalCppFileSettings().addMimeInitializer();

#ifdef WITH_TESTS
    plugin.addTestCreator([] { return new CppFileSettingsTest; });
#else
    Q_UNUSED(plugin)
#endif
}
} // namespace CppEditor::Internal

#include <cppfilesettingspage.moc>
