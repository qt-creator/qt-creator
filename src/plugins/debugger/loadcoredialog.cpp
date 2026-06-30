// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "loadcoredialog.h"

#include "debuggerkitaspect.h"
#include "debuggerruncontrol.h"
#include "debuggertr.h"
#include "gdb/gdbengine.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/aspects.h>
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
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Debugger::Internal {

class AttachCoreDialogData : public AspectContainer
{
public:
    AttachCoreDialogData()
    {
        setSettingsGroup("DebugMode");

        coreFile.setSettingsKey("LastLocalCoreFile");
        coreFile.setHistoryCompleter("Debugger.CoreFile.History");
        coreFile.setExpectedKind(PathChooser::File);
        coreFile.setPromptDialogTitle(Tr::tr("Select Core File"));
        coreFile.setAllowPathFromDevice(true);
        coreFile.setLabelText(Tr::tr("Core file:"));

        symbolFile.setSettingsKey("LastExternalExecutableFile");
        symbolFile.setHistoryCompleter("Executable");
        symbolFile.setExpectedKind(PathChooser::File);
        symbolFile.setPromptDialogTitle(Tr::tr("Select Executable or Symbol File"));
        symbolFile.setAllowPathFromDevice(true);
        symbolFile.setLabelText(Tr::tr("&Executable or symbol file:"));
        symbolFile.setToolTip(
            Tr::tr("Select a file containing debug information corresponding to the core file. "
                   "Typically, this is the executable or a *.debug file if the debug "
                   "information is stored separately from the executable."));

        overrideStartScript.setSettingsKey("LastExternalStartScript");
        overrideStartScript.setHistoryCompleter("Debugger.StartupScript.History");
        overrideStartScript.setExpectedKind(PathChooser::File);
        overrideStartScript.setPromptDialogTitle(Tr::tr("Select Startup Script"));
        overrideStartScript.setLabelText(Tr::tr("Override &start script:"));

        sysRoot.setSettingsKey("LastSysRoot");
        sysRoot.setHistoryCompleter("Debugger.SysRoot.History");
        sysRoot.setExpectedKind(PathChooser::Directory);
        sysRoot.setPromptDialogTitle(Tr::tr("Select SysRoot Directory"));
        sysRoot.setToolTip(Tr::tr("This option can be used to override the kit's SysRoot setting"));
        sysRoot.setLabelText(Tr::tr("Override S&ysRoot:"));
    }

    FilePathAspect coreFile{this};
    FilePathAspect symbolFile{this};
    FilePathAspect overrideStartScript{this};
    FilePathAspect sysRoot{this};
};

class AttachCoreDialog final : public QDialog
{
public:
    AttachCoreDialog();

    int exec() final;

    FilePath symbolFile() const { return m_data.symbolFile(); }
    FilePath coreFile() const { return m_data.coreFile(); }
    FilePath overrideStartScript() const { return m_data.overrideStartScript(); }
    FilePath sysRoot() const { return m_data.sysRoot(); }

    // For persistance.
    ProjectExplorer::Kit *kit() const { return m_kitChooser->currentKit(); }

    void setKitId(Id id) { m_kitChooser->setCurrentKitId(id); }
    void restoreSettings() { m_data.readSettings(); }
    void saveSettings() const { m_data.writeSettings(); }

    FilePath coreFileCopy() const;
    FilePath symbolFileCopy() const;

private:
    void accepted();
    void changed();
    void coreFileChanged(const FilePath &core);

    KitChooser *m_kitChooser;

    AttachCoreDialogData m_data;

    FilePath m_debuggerPath;

    QDialogButtonBox *m_buttonBox;
    ProgressIndicator *m_progressIndicator;
    QLabel *m_progressLabel;

    QTaskTree m_taskTree;
    Result<FilePath> m_coreFileResult;
    Result<FilePath> m_symbolFileResult;

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
        st.validKit = (m_kitChooser->currentKit() != nullptr);
        st.validSymbolFilename = m_data.symbolFile.pathChooser()->isValid();
        st.validCoreFilename = m_data.coreFile.pathChooser()->isValid();
        return st;
    }
};

AttachCoreDialog::AttachCoreDialog()
    : QDialog(ICore::dialogParent())
{
    setWindowTitle(Tr::tr("Load Core File"));

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    m_buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    m_kitChooser = new KitChooser(this);
    m_kitChooser->setShowIcons(true);
    m_kitChooser->populate();

    m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Small, this);
    m_progressIndicator->setVisible(false);

    m_progressLabel = new QLabel();
    m_progressLabel->setVisible(false);

    // clang-format off
    using namespace Layouting;

    Column {
        Form {
            Tr::tr("Kit:"), m_kitChooser, br,
            m_data.coreFile, br,
            m_data.symbolFile, br,
            m_data.overrideStartScript, br,
            m_data.sysRoot, br,
        },
        st,
        hr,
        Row {
            m_progressIndicator, m_progressLabel, m_buttonBox
        }
    }.attachTo(this);
    // clang-format on
}

int AttachCoreDialog::exec()
{
    connect(&m_data.symbolFile, &FilePathAspect::validChanged, this, &AttachCoreDialog::changed);
    connect(&m_data.coreFile, &FilePathAspect::validChanged, this, [this] {
        coreFileChanged(m_data.coreFile());
    });
    connect(m_kitChooser, &KitChooser::currentIndexChanged, this, &AttachCoreDialog::changed);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &AttachCoreDialog::accepted);
    changed();

    connect(&m_taskTree, &QTaskTree::done, this, [this] {
        setEnabled(true);
        m_progressIndicator->setVisible(false);
        m_progressLabel->setVisible(false);

        if (!m_coreFileResult) {
            QMessageBox::critical(this,
                                  Tr::tr("Error"),
                                  Tr::tr("Failed to copy core file to device: %1")
                                      .arg(m_coreFileResult.error()));
            return;
        }

        if (!m_symbolFileResult) {
            QMessageBox::critical(this,
                                  Tr::tr("Error"),
                                  Tr::tr("Failed to copy symbol file to device: %1")
                                      .arg(m_symbolFileResult.error()));
            return;
        }

        accept();
    });
    connect(&m_taskTree, &QTaskTree::progressValueChanged, this, [this](int value) {
        const QString text = Tr::tr("Copying files to device... %1/%2")
                                 .arg(value)
                                 .arg(m_taskTree.progressMaximum());
        m_progressLabel->setText(text);
    });

    State st = getDialogState();
    if (!st.validKit) {
        m_kitChooser->setFocus();
    } else if (!st.validCoreFilename) {
        m_data.coreFile.pathChooser()->setFocus();
    } else if (!st.validSymbolFilename) {
        m_data.symbolFile.pathChooser()->setFocus();
    }

    return QDialog::exec();
}

void AttachCoreDialog::accepted()
{
    const DebuggerItem debuggerItem = Debugger::DebuggerKitAspect::debugger(kit());
    if (!debuggerItem)
        return;
    const FilePath debuggerCommand = debuggerItem.command();

    const auto copyFile = [debuggerCommand](const FilePath &srcPath) -> Result<FilePath> {
        if (!srcPath.isSameDevice(debuggerCommand)) {
            const Result<FilePath> tmpPath = debuggerCommand.tmpDir();
            if (!tmpPath)
                return make_unexpected(tmpPath.error());

            const FilePath pattern = (*tmpPath
                                      / (srcPath.fileName() + ".XXXXXXXXXXX"));

            const Result<FilePath> resultPath = pattern.createTempFile();
            if (!resultPath)
                return make_unexpected(resultPath.error());
            const Result<> result = srcPath.copyFile(*resultPath);
            if (!result)
                return make_unexpected(result.error());

            return resultPath;
        }

        return srcPath;
    };

    using ResultType = Result<FilePath>;

    const auto copyFileAsync = [=](QPromise<ResultType> &promise, const FilePath &srcPath) {
        promise.addResult(copyFile(srcPath));
    };

    const Group root = {
        parallel,
        AsyncTask<ResultType>{[this, copyFileAsync](auto &task) {
                                  task.setConcurrentCallData(copyFileAsync, coreFile());
                              },
                              [this](const Async<ResultType> &task) { m_coreFileResult = task.result(); },
                              CallDoneFlag::OnSuccess},
        AsyncTask<ResultType>{[this, copyFileAsync](auto &task) {
                                  task.setConcurrentCallData(copyFileAsync, symbolFile());
                              },
                              [this](const Async<ResultType> &task) { m_symbolFileResult = task.result(); },
                              CallDoneFlag::OnSuccess}
    };

    m_taskTree.setRecipe(root);
    m_taskTree.start();

    m_progressLabel->setText(Tr::tr("Copying files to device..."));

    setEnabled(false);
    m_progressIndicator->setVisible(true);
    m_progressLabel->setVisible(true);
}

void AttachCoreDialog::coreFileChanged(const FilePath &coreFile)
{
    if (coreFile.osType() != OsType::OsTypeWindows && coreFile.exists()) {
        Kit *k = m_kitChooser->currentKit();
        QTC_ASSERT(k, return);
        ProcessRunData debugger = DebuggerKitAspect::runnable(k);
        CoreInfo cinfo = CoreInfo::readExecutableNameFromCore(debugger, coreFile);
        if (!cinfo.foundExecutableName.isEmpty())
            m_data.symbolFile.setValue(cinfo.foundExecutableName);
        else if (!m_data.symbolFile.pathChooser()->isValid() && !cinfo.rawStringFromCore.isEmpty())
            m_data.symbolFile.setValue(FilePath::fromString(cinfo.rawStringFromCore));
    }
    changed();
}

void AttachCoreDialog::changed()
{
    State st = getDialogState();
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(st.isValid());
}

FilePath AttachCoreDialog::coreFileCopy() const
{
    return m_coreFileResult.value_or(m_data.symbolFile());
}

FilePath AttachCoreDialog::symbolFileCopy() const
{
    return m_symbolFileResult.value_or(m_data.symbolFile());
}

void runAttachToCoreDialog()
{
    AttachCoreDialog dlg;

    QtcSettings *settings = ICore::settings();
    const Key kitKey("DebugMode/LastExternalKit");

    const QString lastExternalKit = settings->value(kitKey).toString();
    if (!lastExternalKit.isEmpty())
        dlg.setKitId(Id::fromString(lastExternalKit));
    dlg.restoreSettings();

    if (dlg.exec() != QDialog::Accepted)
        return;

    dlg.saveSettings();
    settings->setValue(kitKey, dlg.kit()->id().toSetting());

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setKit(dlg.kit());
    runControl->setDisplayName(Tr::tr("Core file \"%1\"").arg(dlg.coreFile().toUserOutput()));

    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    rp.setInferiorExecutable(dlg.symbolFileCopy());
    rp.setCoreFilePath(dlg.coreFileCopy());
    rp.setStartMode(AttachToCore);
    rp.setCloseMode(DetachAtClose);
    rp.setOverrideStartScript(dlg.overrideStartScript());
    const FilePath sysRoot = dlg.sysRoot();
    if (!sysRoot.isEmpty())
        rp.setSysRoot(sysRoot);

    runControl->setRunRecipe(debuggerRecipe(runControl, rp));
    runControl->start();
}

} // Debugger::Internal
