// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingspage.h"

#include "clearcaseconstants.h"
#include "clearcaseplugin.h"
#include "clearcasesettings.h"
#include "clearcasetr.h"

#include <vcsbase/vcsbaseconstants.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QCoreApplication>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QSpinBox>

using namespace Utils;

namespace ClearCase::Internal {

class SettingsPageWidget final : public Core::IOptionsPageWidget
{
public:
    SettingsPageWidget();

private:
    void apply() final;

    Utils::PathChooser *commandPathChooser;
    QRadioButton *graphicalDiffRadioButton;
    QRadioButton *externalDiffRadioButton;
    QLineEdit *diffArgsEdit;
    QSpinBox *historyCountSpinBox;
    QSpinBox *timeOutSpinBox;
    QCheckBox *autoCheckOutCheckBox;
    QCheckBox *promptCheckBox;
    QCheckBox *disableIndexerCheckBox;
    QLineEdit *indexOnlyVOBsEdit;
    QCheckBox *autoAssignActivityCheckBox;
    QCheckBox *noCommentCheckBox;
};

SettingsPageWidget::SettingsPageWidget()
{
    commandPathChooser = new PathChooser;
    commandPathChooser->setPromptDialogTitle(Tr::tr("ClearCase Command"));
    commandPathChooser->setExpectedKind(PathChooser::ExistingCommand);
    commandPathChooser->setHistoryCompleter("ClearCase.Command.History");

    graphicalDiffRadioButton = new QRadioButton(Tr::tr("&Graphical (single file only)"));
    graphicalDiffRadioButton->setChecked(true);

    auto diffWidget = new QWidget;
    diffWidget->setEnabled(false);

    externalDiffRadioButton = new QRadioButton(Tr::tr("&External"));
    QObject::connect(externalDiffRadioButton, &QRadioButton::toggled, diffWidget, &QWidget::setEnabled);

    diffArgsEdit = new QLineEdit(diffWidget);

    QPalette palette;
    QBrush brush(QColor(255, 0, 0, 255));
    brush.setStyle(Qt::SolidPattern);
    palette.setBrush(QPalette::Active, QPalette::WindowText, brush);
    palette.setBrush(QPalette::Inactive, QPalette::WindowText, brush);
    QBrush brush1(QColor(68, 96, 92, 255));
    brush1.setStyle(Qt::SolidPattern);
    palette.setBrush(QPalette::Disabled, QPalette::WindowText, brush1);

    auto diffWarningLabel = new QLabel;
    diffWarningLabel->setPalette(palette);
    diffWarningLabel->setWordWrap(true);

    historyCountSpinBox = new QSpinBox;
    historyCountSpinBox->setMaximum(10000);

    timeOutSpinBox = new QSpinBox;
    timeOutSpinBox->setSuffix(Tr::tr("s", nullptr));
    timeOutSpinBox->setRange(1, 360);
    timeOutSpinBox->setValue(30);

    autoCheckOutCheckBox = new QCheckBox(Tr::tr("&Automatically check out files on edit"));

    promptCheckBox = new QCheckBox(Tr::tr("&Prompt on check-in"));

    disableIndexerCheckBox = new QCheckBox(Tr::tr("Di&sable indexer"));

    indexOnlyVOBsEdit = new QLineEdit;
    indexOnlyVOBsEdit->setToolTip(Tr::tr("VOBs list, separated by comma. Indexer will only traverse "
        "the specified VOBs. If left blank, all active VOBs will be indexed."));

    autoAssignActivityCheckBox = new QCheckBox(Tr::tr("Aut&o assign activity names"));
    autoAssignActivityCheckBox->setToolTip(Tr::tr("Check this if you have a trigger that renames "
        "the activity automatically. You will not be prompted for activity name."));

    noCommentCheckBox = new QCheckBox(Tr::tr("Do &not prompt for comment during checkout or check-in"));
    noCommentCheckBox->setToolTip(Tr::tr("Check out or check in files with no comment (-nc/omment)."));

    using namespace Layouting;

    Form {
        Tr::tr("Arg&uments:"), diffArgsEdit, noMargin
    }.attachTo(diffWidget);

    Column {
        Group {
            title(Tr::tr("Configuration")),
            Form {
                Tr::tr("&Command:"), commandPathChooser
            }
        },

        Group {
            title(Tr::tr("Diff")),
            Form {
                graphicalDiffRadioButton, br,
                externalDiffRadioButton, diffWidget, br,
                Span(2, diffWarningLabel)
            }
        },

        Group {
            title(Tr::tr("Miscellaneous")),
            Form {
                Tr::tr("&History count:"), historyCountSpinBox, br,
                Tr::tr("&Timeout:"), timeOutSpinBox, br,
                autoCheckOutCheckBox, br,
                autoAssignActivityCheckBox, br,
                noCommentCheckBox, br,
                promptCheckBox, br,
                disableIndexerCheckBox, br,
                Tr::tr("&Index only VOBs:"), indexOnlyVOBsEdit,
             }
        },
        st
    }.attachTo(this);


    const ClearCaseSettings &s = ClearCasePlugin::settings();

    commandPathChooser->setFilePath(FilePath::fromString(s.ccCommand));
    timeOutSpinBox->setValue(s.timeOutS);
    autoCheckOutCheckBox->setChecked(s.autoCheckOut);
    noCommentCheckBox->setChecked(s.noComment);
    bool extDiffAvailable = !Environment::systemEnvironment().searchInPath(QLatin1String("diff")).isEmpty();
    if (extDiffAvailable) {
        diffWarningLabel->setVisible(false);
    } else {
        QString diffWarning = Tr::tr("In order to use External diff, \"diff\" command needs to be accessible.");
        if (HostOsInfo::isWindowsHost()) {
            diffWarning += QLatin1Char(' ');
            diffWarning.append(Tr::tr("DiffUtils is available for free download at "
                                      "http://gnuwin32.sourceforge.net/packages/diffutils.htm. "
                                      "Extract it to a directory in your PATH."));
        }
        diffWarningLabel->setText(diffWarning);
        externalDiffRadioButton->setEnabled(false);
    }
    if (extDiffAvailable && s.diffType == ExternalDiff)
        externalDiffRadioButton->setChecked(true);
    else
        graphicalDiffRadioButton->setChecked(true);
    autoAssignActivityCheckBox->setChecked(s.autoAssignActivityName);
    historyCountSpinBox->setValue(s.historyCount);
    disableIndexerCheckBox->setChecked(s.disableIndexer);
    diffArgsEdit->setText(s.diffArgs);
    indexOnlyVOBsEdit->setText(s.indexOnlyVOBs);
}

void SettingsPageWidget::apply()
{
    ClearCaseSettings rc;
    rc.ccCommand = commandPathChooser->rawFilePath().toString();
    rc.ccBinaryPath = commandPathChooser->filePath();
    rc.timeOutS = timeOutSpinBox->value();
    rc.autoCheckOut = autoCheckOutCheckBox->isChecked();
    rc.noComment = noCommentCheckBox->isChecked();
    if (graphicalDiffRadioButton->isChecked())
        rc.diffType = GraphicalDiff;
    else if (externalDiffRadioButton->isChecked())
        rc.diffType = ExternalDiff;
    rc.autoAssignActivityName = autoAssignActivityCheckBox->isChecked();
    rc.historyCount = historyCountSpinBox->value();
    rc.disableIndexer = disableIndexerCheckBox->isChecked();
    rc.diffArgs = diffArgsEdit->text();
    rc.indexOnlyVOBs = indexOnlyVOBsEdit->text();
    rc.extDiffAvailable = externalDiffRadioButton->isEnabled();

    ClearCasePlugin::setSettings(rc);
}

ClearCaseSettingsPage::ClearCaseSettingsPage()
{
    setId(ClearCase::Constants::VCS_ID_CLEARCASE);
    setDisplayName(Tr::tr("ClearCase"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new SettingsPageWidget; });
}

} // ClearCase::Internal
