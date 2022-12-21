// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "highlightersettingspage.h"

#include "highlightersettings.h"
#include "highlighter.h"

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QApplication>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>

using namespace Utils;

namespace TextEditor {
namespace Internal {

class HighlighterSettingsPageWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(TextEditor::Internal::HighlighterSettingsPage)
public:
    QLabel *definitionsInfolabel;
    QPushButton *downloadDefinitions;
    QLabel *updateStatus;
    PathChooser *definitionFilesPath;
    QPushButton *reloadDefinitions;
    QPushButton *resetCache;
    QLineEdit *ignoreEdit;

    HighlighterSettingsPageWidget()
    {
        resize(521, 332);

        definitionsInfolabel = new QLabel(this);
        definitionsInfolabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        definitionsInfolabel->setTextFormat(Qt::RichText);
        definitionsInfolabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        definitionsInfolabel->setWordWrap(true);
        definitionsInfolabel->setOpenExternalLinks(true);
        definitionsInfolabel->setText(tr("<html><head/><body><p>Highlight definitions are provided by the "
                                         "<a href=\"https://api.kde.org/frameworks/syntax-highlighting/html/index.html\">"
                                         "KSyntaxHighlighting</a> engine.</p></body></html>"));

        downloadDefinitions = new QPushButton(tr("Download Definitions"));
        downloadDefinitions->setToolTip(tr("Download missing and update existing syntax definition files."));

        updateStatus = new QLabel;
        updateStatus->setObjectName("updateStatus");

        definitionFilesPath = new PathChooser;
        definitionFilesPath->setExpectedKind(PathChooser::ExistingDirectory);
        definitionFilesPath->setHistoryCompleter("TextEditor.Highlighter.History");

        reloadDefinitions = new QPushButton(tr("Reload Definitions"));
        reloadDefinitions->setToolTip(tr("Reload externally modified definition files."));

        resetCache = new QPushButton(tr("Reset Remembered Definitions"));
        resetCache->setToolTip(tr("Reset definitions remembered for files that can be "
                                  "associated with more than one highlighter definition."));

        ignoreEdit = new QLineEdit;

        using namespace Layouting;
        Column {
            definitionsInfolabel,
            Space(3),
            Group {
                title(tr("Syntax Highlight Definition Files")),
                Column {
                    Row { downloadDefinitions, updateStatus, st },
                    Row { tr("User Highlight Definition Files"),
                                definitionFilesPath, reloadDefinitions },
                    Row { st, resetCache }
                }
            },
            Row { tr("Ignored file patterns:"), ignoreEdit },
            st
        }.attachTo(this);

        connect(downloadDefinitions, &QPushButton::pressed,
                [label = QPointer<QLabel>(updateStatus)]() {
                    Highlighter::downloadDefinitions([label] {
                        if (label)
                            label->setText(tr("Download finished"));
                    });
                });

        connect(reloadDefinitions, &QPushButton::pressed, this, [] {
            Highlighter::reload();
        });
        connect(resetCache, &QPushButton::clicked, this, [] {
            Highlighter::clearDefinitionForDocumentCache();
        });
    }
};

} // Internal

using namespace Internal;

class HighlighterSettingsPagePrivate
{
    Q_DECLARE_TR_FUNCTIONS(TextEditor::Internal::HighlighterSettingsPage)

public:
    HighlighterSettingsPagePrivate() = default;

    void ensureInitialized();
    void migrateGenericHighlighterFiles();

    void settingsFromUI();
    void settingsToUI();
    bool settingsChanged();

    bool m_initialized = false;
    const QString m_settingsPrefix{"Text"};

    HighlighterSettings m_settings;

    QPointer<HighlighterSettingsPageWidget> m_widget;
};

void HighlighterSettingsPagePrivate::migrateGenericHighlighterFiles()
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

void HighlighterSettingsPagePrivate::ensureInitialized()
{
    if (m_initialized)
        return;
    m_initialized = true;
    m_settings.fromSettings(m_settingsPrefix, Core::ICore::settings());
    migrateGenericHighlighterFiles();
}

HighlighterSettingsPage::HighlighterSettingsPage()
    : d(new HighlighterSettingsPagePrivate)
{
    setId(Constants::TEXT_EDITOR_HIGHLIGHTER_SETTINGS);
    setDisplayName(HighlighterSettingsPagePrivate::tr("Generic Highlighter"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("TextEditor", "Text Editor"));
    setCategoryIconPath(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY_ICON_PATH);
}

HighlighterSettingsPage::~HighlighterSettingsPage()
{
    delete d;
}

QWidget *HighlighterSettingsPage::widget()
{
    if (!d->m_widget) {
        d->m_widget = new HighlighterSettingsPageWidget;
        d->settingsToUI();
    }
    return d->m_widget;
}

void HighlighterSettingsPage::apply()
{
    if (!d->m_widget) // page was not shown
        return;
    if (d->settingsChanged())
        d->settingsFromUI();
}

void HighlighterSettingsPage::finish()
{
    delete d->m_widget;
    d->m_widget = nullptr;
}

const HighlighterSettings &HighlighterSettingsPage::highlighterSettings() const
{
    d->ensureInitialized();
    return d->m_settings;
}

void HighlighterSettingsPagePrivate::settingsFromUI()
{
    ensureInitialized();
    m_settings.setDefinitionFilesPath(m_widget->definitionFilesPath->filePath());
    m_settings.setIgnoredFilesPatterns(m_widget->ignoreEdit->text());
    m_settings.toSettings(m_settingsPrefix, Core::ICore::settings());
}

void HighlighterSettingsPagePrivate::settingsToUI()
{
    ensureInitialized();
    m_widget->definitionFilesPath->setFilePath(m_settings.definitionFilesPath());
    m_widget->ignoreEdit->setText(m_settings.ignoredFilesPatterns());
}

bool HighlighterSettingsPagePrivate::settingsChanged()
{
    ensureInitialized();
    return m_settings.definitionFilesPath() != m_widget->definitionFilesPath->filePath()
        || m_settings.ignoredFilesPatterns() != m_widget->ignoreEdit->text();
}

} // TextEditor
