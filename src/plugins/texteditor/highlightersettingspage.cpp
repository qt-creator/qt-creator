// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "highlightersettingspage.h"

#include "highlightersettings.h"
#include "highlighter.h"
#include "texteditortr.h"

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>

using namespace Utils;

namespace TextEditor {

class HighlighterSettingsPageWidget;

class HighlighterSettingsPagePrivate
{
public:
    void ensureInitialized()
    {
        if (m_initialized)
            return;
        m_initialized = true;
        m_settings.fromSettings(m_settingsPrefix, Core::ICore::settings());
        migrateGenericHighlighterFiles();
    }

    void migrateGenericHighlighterFiles()
    {
        QDir userDefinitionPath(m_settings.definitionFilesPath().toString());
        if (userDefinitionPath.mkdir("syntax")) {
            const auto link = Utils::HostOsInfo::isAnyUnixHost()
                                  ? static_cast<bool(*)(const QString &, const QString &)>(&QFile::link)
                                  : static_cast<bool(*)(const QString &, const QString &)>(&QFile::copy);

            for (const QFileInfo &file : userDefinitionPath.entryInfoList({"*.xml"}, QDir::Files))
                link(file.filePath(), file.absolutePath() + "/syntax/" + file.fileName());
        }
    }

    bool m_initialized = false;
    const Key m_settingsPrefix{"Text"};

    HighlighterSettings m_settings;

    QPointer<HighlighterSettingsPageWidget> m_widget;
};

class HighlighterSettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    HighlighterSettingsPageWidget(HighlighterSettingsPagePrivate *d) : d(d)
    {
        d->ensureInitialized();

        auto definitionsInfolabel = new QLabel(this);
        definitionsInfolabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        definitionsInfolabel->setTextFormat(Qt::RichText);
        definitionsInfolabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        definitionsInfolabel->setWordWrap(true);
        definitionsInfolabel->setOpenExternalLinks(true);
        definitionsInfolabel->setText(Tr::tr("<html><head/><body><p>Highlight definitions are provided by the "
                                             "<a href=\"https://api.kde.org/frameworks/syntax-highlighting/html/index.html\">"
                                             "KSyntaxHighlighting</a> engine.</p></body></html>"));

        auto downloadDefinitions = new QPushButton(Tr::tr("Download Definitions"));
        downloadDefinitions->setToolTip(Tr::tr("Download missing and update existing syntax definition files."));

        auto updateStatus = new QLabel;
        updateStatus->setObjectName("updateStatus");

        m_definitionFilesPath = new PathChooser;
        m_definitionFilesPath->setFilePath(d->m_settings.definitionFilesPath());
        m_definitionFilesPath->setExpectedKind(PathChooser::ExistingDirectory);
        m_definitionFilesPath->setHistoryCompleter("TextEditor.Highlighter.History");

        auto reloadDefinitions = new QPushButton(Tr::tr("Reload Definitions"));
        reloadDefinitions->setToolTip(Tr::tr("Reload externally modified definition files."));

        auto resetCache = new QPushButton(Tr::tr("Reset Remembered Definitions"));
        resetCache->setToolTip(Tr::tr("Reset definitions remembered for files that can be "
                                      "associated with more than one highlighter definition."));

        m_ignoreEdit = new QLineEdit(d->m_settings.ignoredFilesPatterns());

        using namespace Layouting;
        Column {
            definitionsInfolabel,
            Space(3),
            Group {
                title(Tr::tr("Syntax Highlight Definition Files")),
                Column {
                    Row { downloadDefinitions, updateStatus, st },
                    Row { Tr::tr("User Highlight Definition Files"),
                                m_definitionFilesPath, reloadDefinitions },
                    Row { st, resetCache }
                }
            },
            Row { Tr::tr("Ignored file patterns:"), m_ignoreEdit },
            st
        }.attachTo(this);

        connect(downloadDefinitions, &QPushButton::pressed,
                [label = QPointer<QLabel>(updateStatus)]() {
                    Highlighter::downloadDefinitions([label] {
                        if (label)
                            label->setText(Tr::tr("Download finished"));
                    });
                });

        connect(reloadDefinitions, &QPushButton::pressed, this, [] {
            Highlighter::reload();
        });
        connect(resetCache, &QPushButton::clicked, this, [] {
            Highlighter::clearDefinitionForDocumentCache();
        });
    }

    void apply() final
    {
        bool changed = d->m_settings.definitionFilesPath() != m_definitionFilesPath->filePath()
                    || d->m_settings.ignoredFilesPatterns() != m_ignoreEdit->text();

        if (changed) {
            d->m_settings.setDefinitionFilesPath(m_definitionFilesPath->filePath());
            d->m_settings.setIgnoredFilesPatterns(m_ignoreEdit->text());
            d->m_settings.toSettings(d->m_settingsPrefix, Core::ICore::settings());
        }
    }

    PathChooser *m_definitionFilesPath;
    QLineEdit *m_ignoreEdit;
    HighlighterSettingsPagePrivate *d;
};

// HighlighterSettingsPage

HighlighterSettingsPage::HighlighterSettingsPage()
    : d(new HighlighterSettingsPagePrivate)
{
    setId(Constants::TEXT_EDITOR_HIGHLIGHTER_SETTINGS);
    setDisplayName(Tr::tr("Generic Highlighter"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr("Text Editor"));
    setCategoryIconPath(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY_ICON_PATH);
    setWidgetCreator([this] { return new HighlighterSettingsPageWidget(d); });
}

HighlighterSettingsPage::~HighlighterSettingsPage()
{
    delete d;
}

const HighlighterSettings &HighlighterSettingsPage::highlighterSettings() const
{
    d->ensureInitialized();
    return d->m_settings;
}

} // TextEditor
