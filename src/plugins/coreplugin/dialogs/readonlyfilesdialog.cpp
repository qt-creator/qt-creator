/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "fileiconprovider.h"
#include "readonlyfilesdialog.h"
#include "ui_readonlyfilesdialog.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>

namespace Core {
namespace Internal {

class ReadOnlyFilesDialogPrivate
{
public:
    ReadOnlyFilesDialogPrivate(IDocument *document = 0, bool useSaveAs = false);
    ~ReadOnlyFilesDialogPrivate();

    // Buttongroups containing the operation for one file.
    struct ButtonGroupForFile
    {
        QString fileName;
        QButtonGroup *group;
    };
    QList <ButtonGroupForFile> buttonGroups;

    QMap <int, int> setAllIndexForOperation;
    // The version control systems for every file, if the file isn't in VCS the value is 0.
    QHash <QString, IVersionControl*> versionControls;

    // Define if some specific operations should be allowed to make the files writable.
    const bool useSaveAs;
    bool useVCS;

    // Define if an error should be displayed when an operation fails.
    bool showWarnings;
    QString failWarning;

    // The document is necessary for the Save As operation.
    IDocument *document;

    // Operation text for the tree widget header and combo box entries for
    // modifying operations for all files.
    const QString mixedText;
    QString makeWritableText;
    QString versionControlOpenText;
    const QString saveAsText;
};

ReadOnlyFilesDialogPrivate::ReadOnlyFilesDialogPrivate(IDocument *document, bool displaySaveAs)
    : useSaveAs(displaySaveAs)
    , useVCS(false)
    , showWarnings(false)
    , document(document)
    , mixedText(ReadOnlyFilesDialog::tr("Mixed"))
    , makeWritableText(ReadOnlyFilesDialog::tr("Make Writable"))
    , versionControlOpenText(ReadOnlyFilesDialog::tr("Open With VCS"))
    , saveAsText(ReadOnlyFilesDialog::tr("Save As"))
{}

ReadOnlyFilesDialogPrivate::~ReadOnlyFilesDialogPrivate()
{
    foreach (const ButtonGroupForFile &groupForFile, buttonGroups)
        delete groupForFile.group;
}

/*!
 * \class ReadOnlyFilesDialog
 * \brief Dialog to show a set of files which are classified as not writable.
 *
 * Automatically checks which operations are allowed to make the file writable. These operations
 * are Make Writable which tries to set the file permissions in the file system,
 * Open With Version Control System if the open operation is allowed by the version control system
 * and Save As which is used to save the changes to a document in another file.
 */

ReadOnlyFilesDialog::ReadOnlyFilesDialog(const QList<QString> &fileNames, QWidget *parent)
    : QDialog(parent)
    , d(new ReadOnlyFilesDialogPrivate)
    , ui(new Ui::ReadOnlyFilesDialog)
{
    initDialog(fileNames);
}

ReadOnlyFilesDialog::ReadOnlyFilesDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent)
    , d(new ReadOnlyFilesDialogPrivate)
    , ui(new Ui::ReadOnlyFilesDialog)
{
    initDialog(QStringList() << fileName);
}

ReadOnlyFilesDialog::ReadOnlyFilesDialog(IDocument *document, QWidget *parent,
                                         bool displaySaveAs)
    : QDialog(parent)
    , d(new ReadOnlyFilesDialogPrivate(document, displaySaveAs))
    , ui(new Ui::ReadOnlyFilesDialog)
{
    initDialog(QStringList() << document->fileName());
}

ReadOnlyFilesDialog::ReadOnlyFilesDialog(const QList<IDocument *> documents, QWidget *parent)
    : QDialog(parent)
    , d(new ReadOnlyFilesDialogPrivate)
    , ui(new Ui::ReadOnlyFilesDialog)
{
    QStringList files;
    foreach (IDocument *document, documents)
        files << document->fileName();
    initDialog(files);
}

ReadOnlyFilesDialog::~ReadOnlyFilesDialog()
{
    delete ui;
    delete d;
}

/*!
 * \brief Set a user defined message in the dialog.
 * \internal
 */
void ReadOnlyFilesDialog::setMessage(const QString &message)
{
    ui->msgLabel->setText(message);
}

/*!
 * \brief Enable the error output to the user via a messageBox.
 * \param warning Added to the dialog, should show possible consequences if the file is still read only.
 * \internal
 */
void ReadOnlyFilesDialog::setShowFailWarning(bool show, const QString &warning)
{
    d->showWarnings = show;
    d->failWarning = warning;
}

/*!
 * \brief Opens a message box with an error description according to the type.
 * \internal
 */
void ReadOnlyFilesDialog::promptFailWarning(const QStringList &files, ReadOnlyResult type) const
{
    if (files.isEmpty())
        return;
    QString title;
    QString message;
    QString details;
    if (files.count() == 1) {
        const QString file = files.first();
        switch (type) {
        case RO_OpenVCS: {
            if (IVersionControl *vc = d->versionControls[file]) {
                const QString openText = vc->vcsOpenText().remove(QLatin1Char('&'));
                title = tr("Failed To: %1 File").arg(openText);
                message = tr("%1 file %2 from version control system %3 failed.\n")
                        .arg(openText)
                        .arg(QDir::toNativeSeparators(file))
                        .arg(vc->displayName());
                message += d->failWarning;
            } else {
                title = tr("No Version Control System Found");
                message = tr("Cannot open file %1 from version control system.\n"
                             "No version control system found.\n")
                        .arg(QDir::toNativeSeparators(file));
                message += d->failWarning;
            }
            break;
        }
        case RO_MakeWritable:
            title = tr("Cannot Set Permissions");
            message = tr("Cannot set permissions for %1 to writable.\n")
                    .arg(QDir::toNativeSeparators(file));
            message += d->failWarning;
            break;
        case RO_SaveAs:
            title = tr("Cannot Save File");
            message = tr("Cannot save file %1\n").arg(QDir::toNativeSeparators(file));
            message += d->failWarning;
            break;
        default:
            title = tr("Canceled Changing Permissions");
            message = d->failWarning;
            break;
        }
    } else {
        title = tr("Could Not Change Permissions On Some Files");
        message = d->failWarning;
        message += tr("\nSee details for a complete list of files.");
        details = files.join(QLatin1String("\n"));
    }
    QMessageBox msgBox(QMessageBox::Warning, title, message);
    msgBox.setDetailedText(details);
    msgBox.exec();
}

/*!
 * \brief Executes the dialog.
 * \return ReadOnlyResult which gives information about the operation which was used to make the files writable.
 * \internal
 *
 * Also displays an error dialog when some operations can't be executed and the function
 * setShowFailWarning was called.
 */
int ReadOnlyFilesDialog::exec()
{
    if (QDialog::exec() != QDialog::Accepted)
        return RO_Cancel;

    ReadOnlyResult result;
    QStringList failedToMakeWritable;
    foreach (ReadOnlyFilesDialogPrivate::ButtonGroupForFile buttengroup, d->buttonGroups) {
        result = static_cast<ReadOnlyResult>(buttengroup.group->checkedId());
        switch (result) {
        case RO_MakeWritable:
            if (!Utils::FileUtils::makeWritable(Utils::FileName(QFileInfo(buttengroup.fileName)))) {
                failedToMakeWritable << buttengroup.fileName;
                continue;
            }
            break;
        case RO_OpenVCS:
            if (!d->versionControls[buttengroup.fileName]->vcsOpen(buttengroup.fileName)) {
                failedToMakeWritable << buttengroup.fileName;
                continue;
            }
            break;
        case RO_SaveAs:
            if (!EditorManager::instance()->saveDocumentAs(d->document)) {
                failedToMakeWritable << buttengroup.fileName;
                continue;
            }
            break;
        default:
            failedToMakeWritable << buttengroup.fileName;
            continue;
        }
        if (!QFileInfo(buttengroup.fileName).isWritable())
            failedToMakeWritable << buttengroup.fileName;
    }
    if (!failedToMakeWritable.isEmpty()) {
        if (d->showWarnings)
            promptFailWarning(failedToMakeWritable, result);
    }
    return failedToMakeWritable.isEmpty() ? result : RO_Cancel;
}

/*!
 * \brief Creates a radio button in the column specified with type.
 * \param group the created button will be added to this group.
 * \return the created button.
 * \internal
 */
QRadioButton* ReadOnlyFilesDialog::createRadioButtonForItem(QTreeWidgetItem *item, QButtonGroup *group,
                                                            ReadOnlyFilesDialog::ReadOnlyFilesTreeColumn type)

{
    QRadioButton *radioButton = new QRadioButton(this);
    group->addButton(radioButton, type);
    item->setTextAlignment(type, Qt::AlignHCenter);
    ui->treeWidget->setItemWidget(item, type, radioButton);
    return radioButton;
}

/*!
 * \brief Checks the type of the select all combo box and change the user selection per file accordingly.
 * \internal
 */
void ReadOnlyFilesDialog::setAll(int index)
{
    // If mixed is the current index, no need to change the user selection.
    if (index == d->setAllIndexForOperation[-1/*mixed*/])
        return;

    // Get the selected type from the select all combo box.
    ReadOnlyFilesTreeColumn type;
    if (index == d->setAllIndexForOperation[MakeWritable])
        type = MakeWritable;
    else if (index == d->setAllIndexForOperation[OpenWithVCS])
        type = OpenWithVCS;
    else if (index == d->setAllIndexForOperation[SaveAs])
        type = SaveAs;

    // Check for every file if the selected operation is available and change it to the operation.
    foreach (ReadOnlyFilesDialogPrivate::ButtonGroupForFile groupForFile, d->buttonGroups) {
        QRadioButton *radioButton = qobject_cast<QRadioButton*> (groupForFile.group->button(type));
        if (radioButton)
            radioButton->setChecked(true);
    }
}

/*!
 * \brief Updates the select all combo box depending on the selection in the tree widget the user made.
 * \internal
 */
void ReadOnlyFilesDialog::updateSelectAll()
{
    int selectedOperation = -1;
    foreach (ReadOnlyFilesDialogPrivate::ButtonGroupForFile groupForFile, d->buttonGroups) {
        if (selectedOperation == -1) {
            selectedOperation = groupForFile.group->checkedId();
        } else if (selectedOperation != groupForFile.group->checkedId()) {
            ui->setAll->setCurrentIndex(0);
            return;
        }
    }
    ui->setAll->setCurrentIndex(d->setAllIndexForOperation[selectedOperation]);
}

/*!
 * \brief Adds files to the dialog and check for possible operation to make the file writable.
 * \param fileNames A List of files which should be added to the dialog.
 * \internal
 */
void ReadOnlyFilesDialog::initDialog(const QStringList &fileNames)
{
    ui->setupUi(this);
    ui->buttonBox->addButton(tr("&Change Permission"), QDialogButtonBox::AcceptRole);
    ui->buttonBox->addButton(QDialogButtonBox::Cancel);

    QString vcsOpenTextForAll;
    QString vcsMakeWritableTextForAll;
    bool useMakeWritable = false;
    foreach (const QString &fileName, fileNames) {
        const QFileInfo info = QFileInfo(fileName);
        const QString visibleName = info.fileName();
        const QString directory = info.absolutePath();

        // Setup a default entry with filename folder and make writable radio button.
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(FileName, visibleName);
        item->setIcon(FileName, FileIconProvider::instance()->icon(QFileInfo(fileName)));
        item->setText(Folder, Utils::FileUtils::shortNativePath(Utils::FileName(QFileInfo(directory))));
        QButtonGroup *radioButtonGroup = new QButtonGroup;

        // Add a button for opening the file with a version control system
        // if the file is managed by an version control system which allows opening files.
        IVersionControl *versionControlForFile =
                ICore::vcsManager()->findVersionControlForDirectory(directory);
        const bool fileManagedByVCS = versionControlForFile
                && versionControlForFile->openSupportMode() != IVersionControl::NoOpen;
        if (fileManagedByVCS) {
            const QString vcsOpenTextForFile =
                    versionControlForFile->vcsOpenText().remove(QLatin1Char('&'));
            const QString vcsMakeWritableTextforFile =
                    versionControlForFile->vcsMakeWritableText().remove(QLatin1Char('&'));
            if (!d->useVCS) {
                vcsOpenTextForAll = vcsOpenTextForFile;
                vcsMakeWritableTextForAll = vcsMakeWritableTextforFile;
                d->useVCS = true;
            } else {
                // If there are different open or make writable texts choose the default one.
                if (vcsOpenTextForFile != vcsOpenTextForAll)
                    vcsOpenTextForAll.clear();
                if (vcsMakeWritableTextforFile != vcsMakeWritableTextForAll)
                    vcsMakeWritableTextForAll.clear();
            }
            // Add make writable if it is supported by the reposetory.
            if (versionControlForFile->openSupportMode() == IVersionControl::OpenOptional) {
                useMakeWritable = true;
                createRadioButtonForItem(item, radioButtonGroup, MakeWritable);
            }
            createRadioButtonForItem(item, radioButtonGroup, OpenWithVCS)->setChecked(true);
        } else {
            useMakeWritable = true;
            createRadioButtonForItem(item, radioButtonGroup, MakeWritable)->setChecked(true);
        }
        // Add a Save As radio button if requested.
        if (d->useSaveAs)
            createRadioButtonForItem(item, radioButtonGroup, SaveAs);
        // If the file is managed by a version control system save the vcs for this file.
        d->versionControls[fileName] = fileManagedByVCS ? versionControlForFile : 0;

        // Also save the buttongroup for every file to get the result for each entry.
        ReadOnlyFilesDialogPrivate::ButtonGroupForFile groupForFile;
        groupForFile.fileName = fileName;
        groupForFile.group = radioButtonGroup;
        d->buttonGroups.append(groupForFile);
        connect(radioButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(updateSelectAll()));
    }

    // Apply the Mac file dialog style.
    if (Utils::HostOsInfo::isMacHost())
        ui->treeWidget->setAlternatingRowColors(true);

    // Do not show any options to the user if he has no choice.
    if (!d->useSaveAs && (!d->useVCS || !useMakeWritable)) {
        ui->treeWidget->setColumnHidden(MakeWritable, true);
        ui->treeWidget->setColumnHidden(OpenWithVCS, true);
        ui->treeWidget->setColumnHidden(SaveAs, true);
        ui->treeWidget->resizeColumnToContents(FileName);
        ui->treeWidget->resizeColumnToContents(Folder);
        ui->setAll->setVisible(false);
        ui->setAllLabel->setVisible(false);
        ui->verticalLayout->removeItem(ui->setAllLayout);
        if (d->useVCS)
            ui->msgLabel->setText(tr("The following files are not checked out by now.\n"
                                     "Do you want to check out these files now?"));
        return;
    }

    // If there is just one file entry, there is no need to show the select all combo box
    if (fileNames.count() < 2) {
        ui->setAll->setVisible(false);
        ui->setAllLabel->setVisible(false);
        ui->verticalLayout->removeItem(ui->setAllLayout);
    }

    // Add items to the Set all combo box.
    ui->setAll->addItem(d->mixedText);
    d->setAllIndexForOperation[-1/*mixed*/] = ui->setAll->count() - 1;
    if (d->useVCS) {
        // If the files are managed by just one version control system, the Open and Make Writable
        // text for the specific system is used.
        if (!vcsOpenTextForAll.isEmpty() && vcsOpenTextForAll != d->versionControlOpenText) {
            d->versionControlOpenText = vcsOpenTextForAll;
            ui->treeWidget->headerItem()->setText(OpenWithVCS, d->versionControlOpenText);
        }
        if (!vcsMakeWritableTextForAll.isEmpty() && vcsMakeWritableTextForAll != d->makeWritableText) {
            d->makeWritableText = vcsMakeWritableTextForAll;
            ui->treeWidget->headerItem()->setText(MakeWritable, d->makeWritableText);
        }
        ui->setAll->addItem(d->versionControlOpenText);
        ui->setAll->setCurrentIndex(ui->setAll->count() - 1);
        d->setAllIndexForOperation[OpenWithVCS] = ui->setAll->count() - 1;
    }
    if (useMakeWritable) {
        ui->setAll->addItem(d->makeWritableText);
        d->setAllIndexForOperation[MakeWritable] = ui->setAll->count() - 1;
        if (ui->setAll->currentIndex() == -1)
            ui->setAll->setCurrentIndex(ui->setAll->count() - 1);
    }
    if (d->useSaveAs) {
        ui->setAll->addItem(d->saveAsText);
        d->setAllIndexForOperation[SaveAs] = ui->setAll->count() - 1;
    }
    connect(ui->setAll, SIGNAL(activated(int)), this, SLOT(setAll(int)));

    // Filter which columns should be visible and resize them to content.
    for (int i = 0; i < NumberOfColumns; ++i) {
        if ((i == SaveAs && !d->useSaveAs) || (i == OpenWithVCS && !d->useVCS)
                || (i == MakeWritable && !useMakeWritable)) {
            ui->treeWidget->setColumnHidden(i, true);
            continue;
        }
        ui->treeWidget->resizeColumnToContents(i);
    }
}

}// namespace Internal
}// namespace Core
