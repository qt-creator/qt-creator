// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "loadcoredialog.h"

#include "debuggerkitaspect.h"
#include "debuggertr.h"
#include "gdb/gdbengine.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/async.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/processinterface.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTextBrowser>
#include <QTreeView>

using namespace Core;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace Debugger::Internal {

///////////////////////////////////////////////////////////////////////
//
// AttachCoreDialog
//
///////////////////////////////////////////////////////////////////////

class AttachCoreDialogPrivate
{
public:
    KitChooser *kitChooser;

    PathChooser *symbolFileName;
    PathChooser *coreFileName;
    PathChooser *overrideStartScriptFileName;
    PathChooser *sysRootDirectory;

    FilePath debuggerPath;

    QDialogButtonBox *buttonBox;
    ProgressIndicator *progressIndicator;
    QLabel *progressLabel;

    TaskTree taskTree;
    expected_str<FilePath> coreFileResult;
    expected_str<FilePath> symbolFileResult;

    struct State
    {
        bool isValid() const
        {
            return validKit && validSymbolFilename && validCoreFilename;
        }

        bool validKit;
        bool validSymbolFilename;
        bool validCoreFilename;
    };

    State getDialogState() const
    {
        State st;
        st.validKit = (kitChooser->currentKit() != nullptr);
        st.validSymbolFilename = symbolFileName->isValid();
        st.validCoreFilename = coreFileName->isValid();
        return st;
    }
};

AttachCoreDialog::AttachCoreDialog(QWidget *parent)
    : QDialog(parent), d(new AttachCoreDialogPrivate)
{
    setWindowTitle(Tr::tr("Load Core File"));

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    d->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    d->kitChooser = new KitChooser(this);
    d->kitChooser->setShowIcons(true);
    d->kitChooser->populate();

    d->coreFileName = new PathChooser(this);
    d->coreFileName->setHistoryCompleter("Debugger.CoreFile.History");
    d->coreFileName->setExpectedKind(PathChooser::File);
    d->coreFileName->setPromptDialogTitle(Tr::tr("Select Core File"));
    d->coreFileName->setAllowPathFromDevice(true);

    d->symbolFileName = new PathChooser(this);
    d->symbolFileName->setHistoryCompleter("Executable");
    d->symbolFileName->setExpectedKind(PathChooser::File);
    d->symbolFileName->setPromptDialogTitle(Tr::tr("Select Executable or Symbol File"));
    d->symbolFileName->setAllowPathFromDevice(true);
    d->symbolFileName->setToolTip(
        Tr::tr("Select a file containing debug information corresponding to the core file. "
           "Typically, this is the executable or a *.debug file if the debug "
           "information is stored separately from the executable."));

    d->overrideStartScriptFileName = new PathChooser(this);
    d->overrideStartScriptFileName->setHistoryCompleter("Debugger.StartupScript.History");
    d->overrideStartScriptFileName->setExpectedKind(PathChooser::File);
    d->overrideStartScriptFileName->setPromptDialogTitle(Tr::tr("Select Startup Script"));

    d->sysRootDirectory = new PathChooser(this);
    d->sysRootDirectory->setHistoryCompleter("Debugger.SysRoot.History");
    d->sysRootDirectory->setExpectedKind(PathChooser::Directory);
    d->sysRootDirectory->setPromptDialogTitle(Tr::tr("Select SysRoot Directory"));
    d->sysRootDirectory->setToolTip(Tr::tr(
        "This option can be used to override the kit's SysRoot setting"));

    d->progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Small, this);
    d->progressIndicator->setVisible(false);

    d->progressLabel = new QLabel();
    d->progressLabel->setVisible(false);

    // clang-format off
    using namespace Layouting;

    Column {
        Form {
            Tr::tr("Kit:"), d->kitChooser, br,
            Tr::tr("Core file:"), d->coreFileName, br,
            Tr::tr("&Executable or symbol file:"), d->symbolFileName, br,
            Tr::tr("Override &start script:"), d->overrideStartScriptFileName, br,
            Tr::tr("Override S&ysRoot:"), d->sysRootDirectory, br,
        },
        st,
        hr,
        Row {
            d->progressIndicator, d->progressLabel, d->buttonBox
        }
    }.attachTo(this);
    // clang-format on
}

AttachCoreDialog::~AttachCoreDialog()
{
    delete d;
}

int AttachCoreDialog::exec()
{
    connect(d->symbolFileName, &PathChooser::validChanged, this, &AttachCoreDialog::changed);
    connect(d->coreFileName, &PathChooser::validChanged, this, [this] {
        coreFileChanged(d->coreFileName->rawFilePath());
    });
    connect(d->coreFileName, &PathChooser::textChanged, this, [this] {
        coreFileChanged(d->coreFileName->rawFilePath());
    });
    connect(d->kitChooser, &KitChooser::currentIndexChanged, this, &AttachCoreDialog::changed);
    connect(d->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(d->buttonBox, &QDialogButtonBox::accepted, this, &AttachCoreDialog::accepted);
    changed();

    connect(&d->taskTree, &TaskTree::done, this, [&]() {
        setEnabled(true);
        d->progressIndicator->setVisible(false);
        d->progressLabel->setVisible(false);

        if (!d->coreFileResult) {
            QMessageBox::critical(this,
                                  Tr::tr("Error"),
                                  Tr::tr("Failed to copy core file to device: %1")
                                      .arg(d->coreFileResult.error()));
            return;
        }

        if (!d->symbolFileResult) {
            QMessageBox::critical(this,
                                  Tr::tr("Error"),
                                  Tr::tr("Failed to copy symbol file to device: %1")
                                      .arg(d->coreFileResult.error()));
            return;
        }

        accept();
    });
    connect(&d->taskTree, &TaskTree::progressValueChanged, this, [this](int value) {
        const QString text = Tr::tr("Copying files to device... %1/%2")
                                 .arg(value)
                                 .arg(d->taskTree.progressMaximum());
        d->progressLabel->setText(text);
    });

    AttachCoreDialogPrivate::State st = d->getDialogState();
    if (!st.validKit) {
        d->kitChooser->setFocus();
    } else if (!st.validCoreFilename) {
        d->coreFileName->setFocus();
    } else if (!st.validSymbolFilename) {
        d->symbolFileName->setFocus();
    }

    return QDialog::exec();
}

void AttachCoreDialog::accepted()
{
    const DebuggerItem *debuggerItem = Debugger::DebuggerKitAspect::debugger(kit());
    const FilePath debuggerCommand = debuggerItem->command();

    const auto copyFile = [debuggerCommand](const FilePath &srcPath) -> expected_str<FilePath> {
        if (!srcPath.isSameDevice(debuggerCommand)) {
            const expected_str<FilePath> tmpPath = debuggerCommand.tmpDir();
            if (!tmpPath)
                return make_unexpected(tmpPath.error());

            const FilePath pattern = (tmpPath.value()
                                      / (srcPath.fileName() + ".XXXXXXXXXXX"));

            const expected_str<FilePath> resultPath = pattern.createTempFile();
            if (!resultPath)
                return make_unexpected(resultPath.error());
            const expected_str<void> result = srcPath.copyFile(resultPath.value());
            if (!result)
                return make_unexpected(result.error());

            return resultPath;
        }

        return srcPath;
    };

    using ResultType = expected_str<FilePath>;

    const auto copyFileAsync = [=](QPromise<ResultType> &promise, const FilePath &srcPath) {
        promise.addResult(copyFile(srcPath));
    };

    const Group root = {
        parallel,
        AsyncTask<ResultType>{[=](auto &task) {
                              task.setConcurrentCallData(copyFileAsync, this->coreFile());
                          },
                          [=](const auto &task) { d->coreFileResult = task.result(); }},
        AsyncTask<ResultType>{[=](auto &task) {
                              task.setConcurrentCallData(copyFileAsync, this->symbolFile());
                          },
                          [=](const auto &task) { d->symbolFileResult = task.result(); }},
    };

    d->taskTree.setRecipe(root);
    d->taskTree.start();

    d->progressLabel->setText(Tr::tr("Copying files to device..."));

    setEnabled(false);
    d->progressIndicator->setVisible(true);
    d->progressLabel->setVisible(true);
}

void AttachCoreDialog::coreFileChanged(const FilePath &coreFile)
{
    if (coreFile.osType() != OsType::OsTypeWindows && coreFile.exists()) {
        Kit *k = d->kitChooser->currentKit();
        QTC_ASSERT(k, return);
        ProcessRunData debugger = DebuggerKitAspect::runnable(k);
        CoreInfo cinfo = CoreInfo::readExecutableNameFromCore(debugger, coreFile);
        if (!cinfo.foundExecutableName.isEmpty())
            d->symbolFileName->setFilePath(cinfo.foundExecutableName);
        else if (!d->symbolFileName->isValid() && !cinfo.rawStringFromCore.isEmpty())
            d->symbolFileName->setFilePath(FilePath::fromString(cinfo.rawStringFromCore));
    }
    changed();
}

void AttachCoreDialog::changed()
{
    AttachCoreDialogPrivate::State st = d->getDialogState();
    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(st.isValid());
}

FilePath AttachCoreDialog::coreFile() const
{
    return d->coreFileName->filePath();
}

FilePath AttachCoreDialog::symbolFile() const
{
    return d->symbolFileName->filePath();
}

FilePath AttachCoreDialog::coreFileCopy() const
{
    return d->coreFileResult.value_or(d->symbolFileName->filePath());
}

FilePath AttachCoreDialog::symbolFileCopy() const
{
    return d->symbolFileResult.value_or(d->symbolFileName->filePath());
}

void AttachCoreDialog::setSymbolFile(const FilePath &symbolFilePath)
{
    d->symbolFileName->setFilePath(symbolFilePath);
}

void AttachCoreDialog::setCoreFile(const FilePath &coreFilePath)
{
    d->coreFileName->setFilePath(coreFilePath);
}

void AttachCoreDialog::setKitId(Id id)
{
    d->kitChooser->setCurrentKitId(id);
}

Kit *AttachCoreDialog::kit() const
{
    return d->kitChooser->currentKit();
}

FilePath AttachCoreDialog::overrideStartScript() const
{
    return d->overrideStartScriptFileName->filePath();
}

void AttachCoreDialog::setOverrideStartScript(const FilePath &scriptName)
{
    d->overrideStartScriptFileName->setFilePath(scriptName);
}

FilePath AttachCoreDialog::sysRoot() const
{
    return d->sysRootDirectory->filePath();
}

void AttachCoreDialog::setSysRoot(const FilePath &sysRoot)
{
    d->sysRootDirectory->setFilePath(sysRoot);
}

} // Debugger::Internal
