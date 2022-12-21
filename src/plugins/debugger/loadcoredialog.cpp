// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "loadcoredialog.h"

#include "debuggerkitinformation.h"
#include "debuggertr.h"
#include "gdb/gdbengine.h"

#include <projectexplorer/devicesupport/devicefilesystemmodel.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTextBrowser>
#include <QTreeView>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger::Internal {

///////////////////////////////////////////////////////////////////////
//
// SelectRemoteFileDialog
//
///////////////////////////////////////////////////////////////////////

class SelectRemoteFileDialog : public QDialog
{
public:
    explicit SelectRemoteFileDialog(QWidget *parent);

    void attachToDevice(Kit *k);
    FilePath localFile() const { return m_localFile; }
    FilePath remoteFile() const { return m_remoteFile; }

private:
    void selectFile();

    QSortFilterProxyModel m_model;
    DeviceFileSystemModel m_fileSystemModel;
    QTreeView *m_fileSystemView;
    QTextBrowser *m_textBrowser;
    QDialogButtonBox *m_buttonBox;
    FilePath m_localFile;
    FilePath m_remoteFile;
    FileTransfer m_fileTransfer;
};

SelectRemoteFileDialog::SelectRemoteFileDialog(QWidget *parent)
    : QDialog(parent)
{
    m_model.setSourceModel(&m_fileSystemModel);

    m_fileSystemView = new QTreeView(this);
    m_fileSystemView->setModel(&m_model);
    m_fileSystemView->setSortingEnabled(true);
    m_fileSystemView->sortByColumn(1, Qt::AscendingOrder);
    m_fileSystemView->setUniformRowHeights(true);
    m_fileSystemView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_fileSystemView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileSystemView->header()->setDefaultSectionSize(100);
    m_fileSystemView->header()->setStretchLastSection(true);

    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setReadOnly(true);

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    m_buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_fileSystemView);
    layout->addWidget(m_textBrowser);
    layout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SelectRemoteFileDialog::selectFile);

    connect(&m_fileTransfer, &FileTransfer::done, this, [this](const ProcessResultData &result) {
        const bool success = result.m_error == QProcess::UnknownError
                          && result.m_exitStatus == QProcess::NormalExit
                          && result.m_exitCode == 0;
        if (success) {
            m_textBrowser->append(Tr::tr("Download of remote file succeeded."));
            accept();
        } else {
            m_textBrowser->append(Tr::tr("Download of remote file failed: %1")
                                  .arg(result.m_errorString));
            m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            m_fileSystemView->setEnabled(true);
        }
    });
}

void SelectRemoteFileDialog::attachToDevice(Kit *k)
{
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    QTC_ASSERT(k, return);
    IDevice::ConstPtr device = DeviceKitAspect::device(k);
    QTC_ASSERT(device, return);
    m_fileSystemModel.setDevice(device);
}

void SelectRemoteFileDialog::selectFile()
{
    QModelIndex idx = m_model.mapToSource(m_fileSystemView->currentIndex());
    if (!idx.isValid())
        return;

    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    m_fileSystemView->setEnabled(false);

    {
        TemporaryFile localFile("remotecore-XXXXXX");
        localFile.open();
        m_localFile = FilePath::fromString(localFile.fileName());
    }

    idx = idx.sibling(idx.row(), 1);
    m_remoteFile = FilePath::fromVariant(m_fileSystemModel.data(idx, DeviceFileSystemModel::PathRole));

    m_fileTransfer.setFilesToTransfer({{m_remoteFile, m_localFile}});
    m_fileTransfer.start();
}

///////////////////////////////////////////////////////////////////////
//
// AttachCoreDialog
//
///////////////////////////////////////////////////////////////////////

class AttachCoreDialogPrivate
{
public:
    KitChooser *kitChooser;

    QCheckBox *forceLocalCheckBox;
    QLabel *forceLocalLabel;
    PathChooser *symbolFileName;
    PathChooser *localCoreFileName;
    PathChooser *remoteCoreFileName;
    QPushButton *selectRemoteCoreButton;

    PathChooser *overrideStartScriptFileName;
    PathChooser *sysRootDirectory;

    QDialogButtonBox *buttonBox;

    struct State
    {
        bool isValid() const
        {
            return validKit && validSymbolFilename && validCoreFilename;
        }

        bool validKit;
        bool validSymbolFilename;
        bool validCoreFilename;
        bool localCoreFile;
        bool localKit;
    };

    State getDialogState(const AttachCoreDialog &p) const
    {
        State st;
        st.localCoreFile = p.useLocalCoreFile();
        st.validKit = (kitChooser->currentKit() != nullptr);
        st.validSymbolFilename = symbolFileName->isValid();

        if (st.localCoreFile)
            st.validCoreFilename = localCoreFileName->isValid();
        else
            st.validCoreFilename = !p.remoteCoreFile().isEmpty();

        st.localKit = p.isLocalKit();
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

    d->forceLocalCheckBox = new QCheckBox(this);
    d->forceLocalLabel = new QLabel(this);
    d->forceLocalLabel->setText(Tr::tr("Use local core file:"));
    d->forceLocalLabel->setBuddy(d->forceLocalCheckBox);

    d->remoteCoreFileName = new PathChooser(this);
    d->selectRemoteCoreButton = new QPushButton(PathChooser::browseButtonLabel(), this);

    d->localCoreFileName = new PathChooser(this);
    d->localCoreFileName->setHistoryCompleter("Debugger.CoreFile.History");
    d->localCoreFileName->setExpectedKind(PathChooser::File);
    d->localCoreFileName->setPromptDialogTitle(Tr::tr("Select Core File"));

    d->symbolFileName = new PathChooser(this);
    d->symbolFileName->setHistoryCompleter("LocalExecutable");
    d->symbolFileName->setExpectedKind(PathChooser::File);
    d->symbolFileName->setPromptDialogTitle(Tr::tr("Select Executable or Symbol File"));
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

    auto coreLayout = new QHBoxLayout;
    coreLayout->addWidget(d->localCoreFileName);
    coreLayout->addWidget(d->remoteCoreFileName);
    coreLayout->addWidget(d->selectRemoteCoreButton);

    auto formLayout = new QFormLayout;
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setHorizontalSpacing(6);
    formLayout->setVerticalSpacing(6);
    formLayout->addRow(Tr::tr("Kit:"), d->kitChooser);
    formLayout->addRow(d->forceLocalLabel, d->forceLocalCheckBox);
    formLayout->addRow(Tr::tr("Core file:"), coreLayout);
    formLayout->addRow(Tr::tr("&Executable or symbol file:"), d->symbolFileName);
    formLayout->addRow(Tr::tr("Override &start script:"), d->overrideStartScriptFileName);
    formLayout->addRow(Tr::tr("Override S&ysRoot:"), d->sysRootDirectory);
    formLayout->addRow(d->buttonBox);

    auto vboxLayout = new QVBoxLayout(this);
    vboxLayout->addLayout(formLayout);
    vboxLayout->addStretch();
    vboxLayout->addWidget(Layouting::createHr());
    vboxLayout->addWidget(d->buttonBox);
}

AttachCoreDialog::~AttachCoreDialog()
{
    delete d;
}

int AttachCoreDialog::exec()
{
    connect(d->selectRemoteCoreButton, &QAbstractButton::clicked, this, &AttachCoreDialog::selectRemoteCoreFile);
    connect(d->remoteCoreFileName, &PathChooser::textChanged, this, [this] {
        coreFileChanged(d->remoteCoreFileName->rawFilePath());
    });
    connect(d->symbolFileName, &PathChooser::textChanged, this, &AttachCoreDialog::changed);
    connect(d->localCoreFileName, &PathChooser::textChanged, this, [this] {
        coreFileChanged(d->localCoreFileName->rawFilePath());
    });
    connect(d->forceLocalCheckBox, &QCheckBox::stateChanged, this, &AttachCoreDialog::changed);
    connect(d->kitChooser, &KitChooser::currentIndexChanged, this, &AttachCoreDialog::changed);
    connect(d->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(d->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    changed();

    AttachCoreDialogPrivate::State st = d->getDialogState(*this);
    if (!st.validKit) {
        d->kitChooser->setFocus();
    } else if (!st.validCoreFilename) {
        if (st.localCoreFile)
            d->localCoreFileName->setFocus();
        else
            d->remoteCoreFileName->setFocus();
    } else if (!st.validSymbolFilename) {
        d->symbolFileName->setFocus();
    }

    return QDialog::exec();
}

bool AttachCoreDialog::isLocalKit() const
{
    Kit *k = d->kitChooser->currentKit();
    QTC_ASSERT(k, return false);
    IDevice::ConstPtr device = DeviceKitAspect::device(k);
    QTC_ASSERT(device, return false);
    return device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

bool AttachCoreDialog::useLocalCoreFile() const
{
    return isLocalKit() || d->forceLocalCheckBox->isChecked();
}

void AttachCoreDialog::coreFileChanged(const FilePath &coreFile)
{
    if (coreFile.osType() != OsType::OsTypeWindows && coreFile.exists()) {
        Kit *k = d->kitChooser->currentKit();
        QTC_ASSERT(k, return);
        Runnable debugger = DebuggerKitAspect::runnable(k);
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
    AttachCoreDialogPrivate::State st = d->getDialogState(*this);

    d->forceLocalLabel->setVisible(!st.localKit);
    d->forceLocalCheckBox->setVisible(!st.localKit);
    if (st.localCoreFile) {
        d->localCoreFileName->setVisible(true);
        d->remoteCoreFileName->setVisible(false);
        d->selectRemoteCoreButton->setVisible(false);
    } else {
        d->localCoreFileName->setVisible(false);
        d->remoteCoreFileName->setVisible(true);
        d->selectRemoteCoreButton->setVisible(true);
    }

    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(st.isValid());
}

void AttachCoreDialog::selectRemoteCoreFile()
{
    changed();
    QTC_ASSERT(!isLocalKit(), return);
    SelectRemoteFileDialog dlg(this);
    dlg.setWindowTitle(Tr::tr("Select Remote Core File"));
    dlg.attachToDevice(d->kitChooser->currentKit());
    if (dlg.exec() == QDialog::Rejected)
        return;
    d->localCoreFileName->setFilePath(dlg.localFile());
    d->remoteCoreFileName->setFilePath(dlg.remoteFile());
    changed();
}

FilePath AttachCoreDialog::localCoreFile() const
{
    return d->localCoreFileName->filePath();
}

FilePath AttachCoreDialog::symbolFile() const
{
    return d->symbolFileName->filePath();
}

void AttachCoreDialog::setSymbolFile(const FilePath &symbolFilePath)
{
    d->symbolFileName->setFilePath(symbolFilePath);
}

void AttachCoreDialog::setLocalCoreFile(const FilePath &coreFilePath)
{
    d->localCoreFileName->setFilePath(coreFilePath);
}

void AttachCoreDialog::setRemoteCoreFile(const FilePath &coreFilePath)
{
    d->remoteCoreFileName->setFilePath(coreFilePath);
}

FilePath AttachCoreDialog::remoteCoreFile() const
{
    return d->remoteCoreFileName->filePath();
}

void AttachCoreDialog::setKitId(Id id)
{
    d->kitChooser->setCurrentKitId(id);
}

void AttachCoreDialog::setForceLocalCoreFile(bool on)
{
    d->forceLocalCheckBox->setChecked(on);
}

bool AttachCoreDialog::forcesLocalCoreFile() const
{
    return d->forceLocalCheckBox->isChecked();
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
