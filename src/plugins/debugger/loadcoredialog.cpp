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
using namespace Tasking;
using namespace Utils;

namespace Debugger::Internal {

class AttachCoreDialog final : public QDialog
{
public:
    AttachCoreDialog();

    int exec() final;

    FilePath symbolFile() const { return m_symbolFileName->filePath(); }
    FilePath coreFile() const { return m_coreFileName->filePath(); }
    FilePath overrideStartScript() const { return m_overrideStartScript->filePath(); }
    FilePath sysRoot() const { return m_sysRootDirectory->filePath(); }

    // For persistance.
    ProjectExplorer::Kit *kit() const { return m_kitChooser->currentKit(); }

    void setSymbolFile(const FilePath &filePath) { m_symbolFileName->setFilePath(filePath); }
    void setCoreFile(const FilePath &filePath) { m_coreFileName->setFilePath(filePath); }
    void setOverrideStartScript(const FilePath &filePath) { m_overrideStartScript->setFilePath(filePath); }
    void setSysRoot(const FilePath &sysRoot) { m_sysRootDirectory->setFilePath(sysRoot); }
    void setKitId(Id id) { m_kitChooser->setCurrentKitId(id); }

    FilePath coreFileCopy() const;
    FilePath symbolFileCopy() const;

private:
    void accepted();
    void changed();
    void coreFileChanged(const FilePath &core);

    KitChooser *m_kitChooser;

    PathChooser *m_symbolFileName;
    PathChooser *m_coreFileName;
    PathChooser *m_overrideStartScript;
    PathChooser *m_sysRootDirectory;

    FilePath m_debuggerPath;

    QDialogButtonBox *m_buttonBox;
    ProgressIndicator *m_progressIndicator;
    QLabel *m_progressLabel;

    TaskTree m_taskTree;
    expected_str<FilePath> m_coreFileResult;
    expected_str<FilePath> m_symbolFileResult;

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
        st.validSymbolFilename = m_symbolFileName->isValid();
        st.validCoreFilename = m_coreFileName->isValid();
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

    m_coreFileName = new PathChooser(this);
    m_coreFileName->setHistoryCompleter("Debugger.CoreFile.History");
    m_coreFileName->setExpectedKind(PathChooser::File);
    m_coreFileName->setPromptDialogTitle(Tr::tr("Select Core File"));
    m_coreFileName->setAllowPathFromDevice(true);

    m_symbolFileName = new PathChooser(this);
    m_symbolFileName->setHistoryCompleter("Executable");
    m_symbolFileName->setExpectedKind(PathChooser::File);
    m_symbolFileName->setPromptDialogTitle(Tr::tr("Select Executable or Symbol File"));
    m_symbolFileName->setAllowPathFromDevice(true);
    m_symbolFileName->setToolTip(
        Tr::tr("Select a file containing debug information corresponding to the core file. "
           "Typically, this is the executable or a *.debug file if the debug "
           "information is stored separately from the executable."));

    m_overrideStartScript = new PathChooser(this);
    m_overrideStartScript->setHistoryCompleter("Debugger.StartupScript.History");
    m_overrideStartScript->setExpectedKind(PathChooser::File);
    m_overrideStartScript->setPromptDialogTitle(Tr::tr("Select Startup Script"));

    m_sysRootDirectory = new PathChooser(this);
    m_sysRootDirectory->setHistoryCompleter("Debugger.SysRoot.History");
    m_sysRootDirectory->setExpectedKind(PathChooser::Directory);
    m_sysRootDirectory->setPromptDialogTitle(Tr::tr("Select SysRoot Directory"));
    m_sysRootDirectory->setToolTip(Tr::tr(
     "This option can be used to override the kit's SysRoot setting"));

    m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Small, this);
    m_progressIndicator->setVisible(false);

    m_progressLabel = new QLabel();
    m_progressLabel->setVisible(false);

    // clang-format off
    using namespace Layouting;

    Column {
        Form {
            Tr::tr("Kit:"), m_kitChooser, br,
            Tr::tr("Core file:"), m_coreFileName, br,
            Tr::tr("&Executable or symbol file:"), m_symbolFileName, br,
            Tr::tr("Override &start script:"), m_overrideStartScript, br,
            Tr::tr("Override S&ysRoot:"), m_sysRootDirectory, br,
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
    connect(m_symbolFileName, &PathChooser::validChanged, this, &AttachCoreDialog::changed);
    connect(m_coreFileName, &PathChooser::validChanged, this, [this] {
        coreFileChanged(m_coreFileName->unexpandedFilePath());
    });
    connect(m_coreFileName, &PathChooser::textChanged, this, [this] {
        coreFileChanged(m_coreFileName->unexpandedFilePath());
    });
    connect(m_kitChooser, &KitChooser::currentIndexChanged, this, &AttachCoreDialog::changed);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &AttachCoreDialog::accepted);
    changed();

    connect(&m_taskTree, &TaskTree::done, this, [this] {
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
                                      .arg(m_coreFileResult.error()));
            return;
        }

        accept();
    });
    connect(&m_taskTree, &TaskTree::progressValueChanged, this, [this](int value) {
        const QString text = Tr::tr("Copying files to device... %1/%2")
                                 .arg(value)
                                 .arg(m_taskTree.progressMaximum());
        m_progressLabel->setText(text);
    });

    State st = getDialogState();
    if (!st.validKit) {
        m_kitChooser->setFocus();
    } else if (!st.validCoreFilename) {
        m_coreFileName->setFocus();
    } else if (!st.validSymbolFilename) {
        m_symbolFileName->setFocus();
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
            const Result result = srcPath.copyFile(resultPath.value());
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
        AsyncTask<ResultType>{[this, copyFileAsync](auto &task) {
                                  task.setConcurrentCallData(copyFileAsync, coreFile());
                              },
                              [this](const Async<ResultType> &task) { m_coreFileResult = task.result(); },
                              CallDoneIf::Success},
        AsyncTask<ResultType>{[this, copyFileAsync](auto &task) {
                                  task.setConcurrentCallData(copyFileAsync, symbolFile());
                              },
                              [this](const Async<ResultType> &task) { m_symbolFileResult = task.result(); },
                              CallDoneIf::Success}
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
            m_symbolFileName->setFilePath(cinfo.foundExecutableName);
        else if (!m_symbolFileName->isValid() && !cinfo.rawStringFromCore.isEmpty())
            m_symbolFileName->setFilePath(FilePath::fromString(cinfo.rawStringFromCore));
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
    return m_coreFileResult.value_or(m_symbolFileName->filePath());
}

FilePath AttachCoreDialog::symbolFileCopy() const
{
    return m_symbolFileResult.value_or(m_symbolFileName->filePath());
}

void runAttachToCoreDialog()
{
    AttachCoreDialog dlg;

    QtcSettings *settings  = ICore::settings();

    const Key executableKey("DebugMode/LastExternalExecutableFile");
    const Key localCoreKey("DebugMode/LastLocalCoreFile");
    const Key kitKey("DebugMode/LastExternalKit");
    const Key startScriptKey("DebugMode/LastExternalStartScript");
    const Key sysrootKey("DebugMode/LastSysRoot");

    const QString lastExternalKit = settings->value(kitKey).toString();
    if (!lastExternalKit.isEmpty())
        dlg.setKitId(Id::fromString(lastExternalKit));
    dlg.setSymbolFile(FilePath::fromSettings(settings->value(executableKey)));
    dlg.setCoreFile(FilePath::fromSettings(settings->value(localCoreKey)));
    dlg.setOverrideStartScript(FilePath::fromSettings(settings->value(startScriptKey)));
    dlg.setSysRoot(FilePath::fromSettings(settings->value(sysrootKey)));

    if (dlg.exec() != QDialog::Accepted)
        return;

    settings->setValue(executableKey, dlg.symbolFile().toSettings());
    settings->setValue(localCoreKey, dlg.coreFile().toSettings());
    settings->setValue(kitKey, dlg.kit()->id().toSetting());
    settings->setValue(startScriptKey, dlg.overrideStartScript().toSettings());
    settings->setValue(sysrootKey, dlg.sysRoot().toSettings());

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setKit(dlg.kit());
    runControl->setDisplayName(Tr::tr("Core file \"%1\"").arg(dlg.coreFile().toUserOutput()));

    auto debugger = new DebuggerRunTool(runControl);
    DebuggerRunParameters &rp = debugger->runParameters();
    debugger->setInferiorExecutable(dlg.symbolFileCopy());
    debugger->setCoreFilePath(dlg.coreFileCopy());
    rp.setStartMode(AttachToCore);
    debugger->setCloseMode(DetachAtClose);
    debugger->setOverrideStartScript(dlg.overrideStartScript());
    const FilePath sysRoot = dlg.sysRoot();
    if (!sysRoot.isEmpty())
        debugger->setSysRoot(sysRoot);

    runControl->start();
}

} // Debugger::Internal
