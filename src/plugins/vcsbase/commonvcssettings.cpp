// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commonvcssettings.h"

#include "vcsbaseconstants.h"
#include "vcsbasetr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/vcsmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/spellchecker.h>

#include <QComboBox>
#include <QLocale>
#include <QStandardItem>

using namespace Core;
using namespace Utils;

namespace VcsBase::Internal {

// Return default for the ssh-askpass command (default to environment)
static QString sshPasswordPromptDefault()
{
    const QString envSetting = qtcEnvironmentVariable("SSH_ASKPASS");
    if (!envSetting.isEmpty())
        return envSetting;
    if (HostOsInfo::isWindowsHost())
        return QLatin1String("win-ssh-askpass");
    return QLatin1String("ssh-askpass");
}

CommonVcsSettings &commonSettings()
{
    static CommonVcsSettings settings;
    return settings;
}

void SpellCheckLanguageAspect::fixupComboBox(QComboBox *comboBox)
{
    comboBox->setMinimumContentsLength(20);
}

QString submitMessageSpellCheckLanguage()
{
    if (!commonSettings().spellCheck())
        return {};
    const QString language = commonSettings().spellCheckLanguage();
    if (!language.isEmpty())
        return language;

    // Submit messages are written in English far more often than in the language the
    // machine is set to, which is what the platform answers with.
    SpellChecker *checker = SpellChecker::instance();
    const QString platformLanguage = checker->defaultLanguage();
    if (platformLanguage.startsWith("en"))
        return platformLanguage;
    const QString english = Utils::findOrDefault(checker->availableLanguages(),
                                                 [](const QString &candidate) {
                                                     return candidate.startsWith("en");
                                                 });
    return english.isEmpty() ? platformLanguage : english;
}

static QString languageDisplayName(const QString &language)
{
    const QLocale locale(QString(language).replace('-', '_'));
    if (locale.language() == QLocale::C)
        return language;
    const QString name = QLocale::languageToString(locale.language());
    if (!language.contains('_') && !language.contains('-'))
        return name;
    return QString("%1 (%2)").arg(name, QLocale::territoryToString(locale.territory()));
}

static void fillSpellCheckLanguageItems(const StringSelectionAspect::ResultCallback &callback)
{
    auto defaultItem = new QStandardItem(Tr::tr("<Default>"));
    defaultItem->setToolTip(Tr::tr("English, if a dictionary for it is installed, "
                                   "otherwise the system language."));
    defaultItem->setData(QString());
    QList<QStandardItem *> items{defaultItem};

    QList<QStandardItem *> languageItems;
    for (const QString &language : SpellChecker::instance()->availableLanguages()) {
        auto item = new QStandardItem(languageDisplayName(language));
        item->setData(language);
        languageItems.append(item);
    }
    Utils::sort(languageItems, [](QStandardItem *a, QStandardItem *b) {
        return a->text().localeAwareCompare(b->text()) < 0;
    });
    callback(items + languageItems);
}

CommonVcsSettings::CommonVcsSettings()
{
    setAutoApply(false);
    setSettingsGroup("VCS");

    nickNameMailMap.setSettingsKey("NickNameMailMap");
    nickNameMailMap.setExpectedKind(PathChooserKind::File);
    nickNameMailMap.setHistoryCompleter("Vcs.NickMap.History");
    nickNameMailMap.setLabelText(Tr::tr("User/&alias configuration file:"));
    nickNameMailMap.setToolTip(Tr::tr("A file listing nicknames in a 4-column mailmap format:\n"
        "'name <email> alias <email>'."));

    nickNameFieldListFile.setSettingsKey("NickNameFieldListFile");
    nickNameFieldListFile.setExpectedKind(PathChooserKind::File);
    nickNameFieldListFile.setHistoryCompleter("Vcs.NickFields.History");
    nickNameFieldListFile.setLabelText(Tr::tr("User &fields configuration file:"));
    nickNameFieldListFile.setToolTip(Tr::tr("A simple file containing lines with field names like "
        "\"Reviewed-By:\" which will be added below the submit editor."));

    submitMessageCheckScript.setSettingsKey("SubmitMessageCheckScript");
    submitMessageCheckScript.setExpectedKind(PathChooserKind::ExistingCommand);
    submitMessageCheckScript.setHistoryCompleter("Vcs.MessageCheckScript.History");
    submitMessageCheckScript.setLabelText(Tr::tr("Submit message &check script:"));
    submitMessageCheckScript.setToolTip(Tr::tr("An executable which is called with the submit message "
        "in a temporary file as first argument. It should return with an exit != 0 and a message "
        "on standard error to indicate failure."));

    sshPasswordPrompt.setSettingsKey("SshPasswordPrompt");
    sshPasswordPrompt.setExpectedKind(PathChooserKind::ExistingCommand);
    sshPasswordPrompt.setHistoryCompleter("Vcs.SshPrompt.History");
    sshPasswordPrompt.setDefaultValue(sshPasswordPromptDefault());
    sshPasswordPrompt.setLabelText(Tr::tr("&SSH prompt command:"));
    sshPasswordPrompt.setToolTip(Tr::tr("Specifies a command that is executed to graphically prompt "
        "for a password,\nshould a repository require SSH-authentication "
        "(see documentation on SSH and the environment variable SSH_ASKPASS)."));

    lineWrap.setSettingsKey("LineWrap");
    lineWrap.setDefaultValue(true);
    lineWrap.setLabelText(Tr::tr("Wrap submit message at"));

    lineWrapWidth.setSettingsKey("LineWrapWidth");
    lineWrapWidth.setSuffix(Tr::tr(" characters"));
    lineWrapWidth.setDefaultValue(72);

    vcsShowStatus.setSettingsKey("ShowVcsStatus");
    vcsShowStatus.setDefaultValue(true);
    vcsShowStatus.setLabelText(Tr::tr("Show file status with refresh interval"));
    vcsShowStatus.setToolTip(Tr::tr("Request file status updates from files and reflect them "
                                    "on the project tree."));
    vcsShowStatusInterval.setSettingsKey("ShowVcsStatusInterval");
    vcsShowStatusInterval.setSuffix(Tr::tr(" seconds"));
    vcsShowStatusInterval.setDefaultValue(10);
    vcsShowStatusInterval.setRange(1, 20);
    vcsShowStatusInterval.setToolTip(Tr::tr("Specifies the file status update refresh interval."));

    spellCheck.setSettingsKey("SpellCheck");
    spellCheck.setDefaultValue(true);
    spellCheck.setLabelText(Tr::tr("Check spelling of submit messages in"));
    spellCheck.setVisible(SpellChecker::instance()->isAvailable());
    spellCheckLanguage.setSettingsKey("SpellCheckLanguage");
    spellCheckLanguage.setFillCallback(fillSpellCheckLanguageItems);
    spellCheckLanguage.setComboBoxEditable(false);
    spellCheckLanguage.setEnabler(&spellCheck);
    spellCheckLanguage.setVisible(SpellChecker::instance()->isAvailable());

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Row { vcsShowStatus, vcsShowStatusInterval, st },
            Row { lineWrap, lineWrapWidth, st },
            Row { spellCheck, spellCheckLanguage, st },
            Form {
                submitMessageCheckScript, br,
                nickNameMailMap, br,
                nickNameFieldListFile, br,
                sshPasswordPrompt, br,
                empty,
                PushButton {
                    text(Tr::tr("Reset Version Control Cache")),
                    Layouting::toolTip(Tr::tr("Reset information about which "
                                              "version control system handles which directory.")),
                    onClicked(this, &VcsManager::clearVersionControlCache)
                }
            }
        };
    });

    auto updatePath = [this] {
        Environment env;
        env.appendToPath(VcsManager::additionalToolsPath());
        sshPasswordPrompt.setEnvironment(env);
    };

    updatePath();
    connect(VcsManager::instance(), &VcsManager::configurationChanged, this, updatePath);

    readSettings();
}

// CommonVcsSettingsPage

class CommonVcsSettingsPage final : public IOptionsPage
{
public:
    CommonVcsSettingsPage()
    {
        setId(Constants::VCS_COMMON_SETTINGS_ID);
        setDisplayName(Tr::tr("General"));
        setCategory(Constants::VCS_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &commonSettings(); });
    }
};

const CommonVcsSettingsPage settingsPage;

} // VcsBase::Internal
