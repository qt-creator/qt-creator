/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "readonlyfilesdialog.h"
#include "ui_readonlyfilesdialog.h"

#include <coreplugin/editormanager/editormanager_p.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/stringutils.h>

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
    Q_DECLARE_TR_FUNCTIONS(Core::ReadOnlyFilesDialog)

public:
    ReadOnlyFilesDialogPrivate(ReadOnlyFilesDialog *parent, IDocument *document = 0, bool useSaveAs = false);
    ~ReadOnlyFilesDialogPrivate();

    enum ReadOnlyFilesTreeColumn {
        MakeWritable = ReadOnlyFilesDialog::MakeWritable,
        OpenWithVCS = ReadOnlyFilesDialog::OpenWithVCS,
        SaveAs = ReadOnlyFilesDialog::SaveAs,
        FileName = ReadOnlyFilesDialog::FileName,
        Folder = ReadOnlyFilesDialog::Folder,
        NumberOfColumns
    };

    void initDialog(const QStringList &fileNames);
    void promptFailWarning(const QStringList &files, ReadOnlyFilesDialog::ReadOnlyResult type) const;
    QRadioButton *createRadioButtonForItem(QTreeWidgetItem *item, QButtonGroup *group, ReadOnlyFilesTreeColumn type);

    void setAll(int index);
    void updateSelectAll();

    ReadOnlyFilesDialog *q;

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

    Ui::ReadOnlyFilesDialog ui;
};

ReadOnlyFilesDialogPrivate::ReadOnlyFilesDialogPrivate(ReadOnlyFilesDialog *parent, IDocument *document, bool displaySaveAs)
    : q(parent)
    , useSaveAs(displaySaveAs)
    , useVCS(false)
    , showWarnings(false)
    , document(document)
    , mixedText(tr("Mixed"))
    , makeWritableText(tr("Make Writable"))
    , versionControlOpenText(tr("Open with VCS"))
    , saveAsText(tr("Save As"))
{}

ReadOnlyFilesDialogPrivate::~ReadOnlyFilesDialogPrivate()
{
    foreach (const ButtonGroupForFile &groupForFile, buttonGroups)
        delete groupForFile.group;
}

} // namespace Internal

using namespace Internal;

/*!
 * \class ReadOnlyFilesDialog
 * \brief The ReadOnlyFilesDialog class implements a dialog to show a set of
 * files that are classified as not writable.
 *
 * Automatically checks which operations are allowed to make the file writable. These operations
 * are Make Writable which tries to set the file permissions in the file system,
 * Open With Version Control System if the open operation is allowed by the version control system
 * and Save As which is used to save the changes to a document in another file.
 */

ReadOnlyFilesDialog::ReadOnlyFilesDialog(const QList<QString> &fileNames, QWidget *parent)
    : QDialog(parent)
    , d(new ReadOnlyFilesDialogPrivate(this))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    d->initDialog(fileNames);
}

ReadOnlyFilesDialog::ReadOnlyFilesDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent)
    , d(new ReadOnlyFilesDialogPrivate(this))
{
    d->initDialog(QStringList(fileName));
}

ReadOnlyFilesDialog::ReadOnlyFilesDialog(IDocument *document, QWidget *parent,
                                         bool displaySaveAs)
    : QDialog(parent)
    , d(new ReadOnlyFilesDialogPrivate(this, document, displaySaveAs))
{
    d->initDialog(QStringList(document->filePath().toString()));
}

ReadOnlyFilesDialog::ReadOnlyFilesDialog(const QList<IDocument *> &documents, QWidget *parent)
    : QDialog(parent)
    , d(new ReadOnlyFilesDialogPrivate(this))
{
    QStringList files;
    foreach (IDocument *document, documents)
        files << document->filePath().toString();
    d->initDialog(files);
}

ReadOnlyFilesDialog::~ReadOnlyFilesDialog()
{
    delete d;
}

/*!
 * Sets a user defined message in the dialog.
 * \internal
 */
void ReadOnlyFilesDialog::setMessage(const QString &message)
{
    d->ui.msgLabel->setText(message);
}

/*!
 * Enables the error output to the user via a message box. \a warning should
 * show the possible consequences if the file is still read only.
 * \internal
 */
void ReadOnlyFilesDialog::setShowFailWarning(bool show, const QString &warning)
{
    d->showWarnings = show;
    d->failWarning = warning;
}

/*!
 * Opens a message box with an error description according to the type.
 * \internal
 */
void ReadOnlyFilesDialogPrivate::promptFailWarning(const QStringList &files, ReadOnlyFilesDialog::ReadOnlyResult type) const
{
    if (files.isEmpty())
        return;
    QString title;
    QString message;
    QString details;
    if (files.count() == 1) {
        const QString file = files.first();
        switch (type) {
        case ReadOnlyFilesDialog::RO_OpenVCS: {
            if (IVersionControl *vc = versionControls[file]) {
                const QString openText = Utils::stripAccelerator(vc->vcsOpenText());
                title = tr("Failed to %1 File").arg(openText);
                message = tr("%1 file %2 from version control system %3 failed.")
                        .arg(openText)
                        .arg(QDir::toNativeSeparators(file))
                        .arg(vc->displayName())
                    + QLatin1Char('\n')
                    + failWarning;
            } else {
                title = tr("No Version Control System Found");
                message = tr("Cannot open file %1 from version control system.\n"
                             "No version control system found.")
                        .arg(QDir::toNativeSeparators(file))
                    + QLatin1Char('\n')
                    + failWarning;;
            }
            break;
        }
        case ReadOnlyFilesDialog::RO_MakeWritable:
            title = tr("Cannot Set Permissions");
            message = tr("Cannot set permissions for %1 to writable.")
                    .arg(QDir::toNativeSeparators(file))
                + QLatin1Char('\n')
                + failWarning;
            break;
        case ReadOnlyFilesDialog::RO_SaveAs:
            title = tr("Cannot Save File");
            message = tr("Cannot save file %1").arg(QDir::toNativeSeparators(file))
                + QLatin1Char('\n')
                + failWarning;
            break;
        default:
            title = tr("Canceled Changing Permissions");
            message = failWarning;
            break;
        }
    } else {
        title = tr("Could Not Change Permissions on Some Files");
        message = failWarning + QLatin1Char('\n')
                + tr("See details for a complete list of files.");
        details = files.join(QLatin1Char('\n'));
    }
    QMessageBox msgBox(QMessageBox::Warning, title, message,
                       QMessageBox::Ok, ICore::dialogParent());
    msgBox.setDetailedText(details);
    msgBox.exec();
}

/*!
 * Executes the ReadOnlyFilesDialog dialog.
 * Returns ReadOnlyResult to provide information about the operation that was
 * used to make the files writable.
 *
 * \internal
 *
 * Also displays an error dialog when some operations cannot be executed and the
 * function \c setShowFailWarning() was called.
 */
int ReadOnlyFilesDialog::exec()
{
    if (QDialog::exec() != QDialog::Accepted)
        return RO_Cancel;

    ReadOnlyResult result = RO_Cancel;
    QStringList failedToMakeWritable;
    foreach (ReadOnlyFilesDialogPrivate::ButtonGroupForFile buttongroup, d->buttonGroups) {
        result = static_cast<ReadOnlyResult>(buttongroup.group->checkedId());
        switch (result) {
        case RO_MakeWritable:
            if (!Utils::FileUtils::makeWritable(Utils::FileName(QFileInfo(buttongroup.fileName)))) {
                failedToMakeWritable << buttongroup.fileName;
                continue;
            }
            break;
        case RO_OpenVCS:
            if (!d->versionControls[buttongroup.fileName]->vcsOpen(buttongroup.fileName)) {
                failedToMakeWritable << buttongroup.fileName;
                continue;
            }
            break;
        case RO_SaveAs:
            if (!EditorManagerPrivate::saveDocumentAs(d->document)) {
                failedToMakeWritable << buttongroup.fileName;
                continue;
            }
            break;
        default:
            failedToMakeWritable << buttongroup.fileName;
            continue;
        }
        if (!QFileInfo(buttongroup.fileName).isWritable())
            failedToMakeWritable << buttongroup.fileName;
    }
    if (!failedToMakeWritable.isEmpty()) {
        if (d->showWarnings)
            d->promptFailWarning(failedToMakeWritable, result);
    }
    return failedToMakeWritable.isEmpty() ? result : RO_Cancel;
}

/*!
 * Creates a radio button in the group \a group and in the column specified by
 * \a type.
 * Returns the created button.
 * \internal
 */
QRadioButton *ReadOnlyFilesDialogPrivate::createRadioButtonForItem(QTreeWidgetItem *item, QButtonGroup *group,
                                                                   ReadOnlyFilesTreeColumn type)

{
    QRadioButton *radioButton = new QRadioButton(q);
    group->addButton(radioButton, type);
    item->setTextAlignment(type, Qt::AlignHCenter);
    ui.treeWidget->setItemWidget(item, type, radioButton);
    return radioButton;
}

/*!
 * Checks the type of the select all combo box and changes the user selection
 * per file accordingly.
 * \internal
 */
void ReadOnlyFilesDialogPrivate::setAll(int index)
{
    // If mixed is the current index, no need to change the user selection.
    if (index == setAllIndexForOperation[-1/*mixed*/])
        return;

    // Get the selected type from the select all combo box.
    ReadOnlyFilesTreeColumn type = NumberOfColumns;
    if (index == setAllIndexForOperation[MakeWritable])
        type = MakeWritable;
    else if (index == setAllIndexForOperation[OpenWithVCS])
        type = OpenWithVCS;
    else if (index == setAllIndexForOperation[SaveAs])
        type = SaveAs;

    // Check for every file if the selected operation is available and change it to the operation.
    foreach (ReadOnlyFilesDialogPrivate::ButtonGroupForFile groupForFile, buttonGroups) {
        QRadioButton *radioButton = qobject_cast<QRadioButton*> (groupForFile.group->button(type));
        if (radioButton)
            radioButton->setChecked(true);
    }
}

/*!
 * Updates the select all combo box depending on the selection the user made in
 * the tree widget.
 * \internal
 */
void ReadOnlyFilesDialogPrivate::updateSelectAll()
{
    int selectedOperation = -1;
    foreach (ReadOnlyFilesDialogPrivate::ButtonGroupForFile groupForFile, buttonGroups) {
        if (selectedOperation == -1) {
            selectedOperation = groupForFile.group->checkedId();
        } else if (selectedOperation != groupForFile.group->checkedId()) {
            ui.setAll->setCurrentIndex(0);
            return;
        }
    }
    ui.setAll->setCurrentIndex(setAllIndexForOperation[selectedOperation]);
}

/*!
 * Adds files to the dialog and checks for a possible operation to make the file
 * writable.
 * \a fileNames contains the list of the files that should be added to the
 * dialog.
 * \internal
 */
void ReadOnlyFilesDialogPrivate::initDialog(const QStringList &fileNames)
{
    ui.setupUi(q);
    ui.buttonBox->addButton(tr("Change &Permission"), QDialogButtonBox::AcceptRole);
    ui.buttonBox->addButton(QDialogButtonBox::Cancel);

    QString vcsOpenTextForAll;
    QString vcsMakeWritableTextForAll;
    bool useMakeWritable = false;
    foreach (const QString &fileName, fileNames) {
        const QFileInfo info = QFileInfo(fileName);
        const QString visibleName = info.fileName();
        const QString directory = info.absolutePath();

        // Setup a default entry with filename folder and make writable radio button.
        QTreeWidgetItem *item = new QTreeWidgetItem(ui.treeWidget);
        item->setText(FileName, visibleName);
        item->setIcon(FileName, FileIconProvider::icon(fileName));
        item->setText(Folder, Utils::FileUtils::shortNativePath(Utils::FileName(QFileInfo(directory))));
        QButtonGroup *radioButtonGroup = new QButtonGroup;

        // Add a button for opening the file with a version control system
        // if the file is managed by an version control system which allows opening files.
        IVersionControl *versionControlForFile =
                VcsManager::findVersionControlForDirectory(directory);
        const bool fileManagedByVCS = versionControlForFile
                && versionControlForFile->openSupportMode(fileName) != IVersionControl::NoOpen;
        if (fileManagedByVCS) {
            const QString vcsOpenTextForFile =
                    Utils::stripAccelerator(versionControlForFile->vcsOpenText());
            const QString vcsMakeWritableTextforFile =
                    Utils::stripAccelerator(versionControlForFile->vcsMakeWritableText());
            if (!useVCS) {
                vcsOpenTextForAll = vcsOpenTextForFile;
                vcsMakeWritableTextForAll = vcsMakeWritableTextforFile;
                useVCS = true;
            } else {
                // If there are different open or make writable texts choose the default one.
                if (vcsOpenTextForFile != vcsOpenTextForAll)
                    vcsOpenTextForAll.clear();
                if (vcsMakeWritableTextforFile != vcsMakeWritableTextForAll)
                    vcsMakeWritableTextForAll.clear();
            }
            // Add make writable if it is supported by the reposetory.
            if (versionControlForFile->openSupportMode(fileName) == IVersionControl::OpenOptional) {
                useMakeWritable = true;
                createRadioButtonForItem(item, radioButtonGroup, MakeWritable);
            }
            createRadioButtonForItem(item, radioButtonGroup, OpenWithVCS)->setChecked(true);
        } else {
            useMakeWritable = true;
            createRadioButtonForItem(item, radioButtonGroup, MakeWritable)->setChecked(true);
        }
        // Add a Save As radio button if requested.
        if (useSaveAs)
            createRadioButtonForItem(item, radioButtonGroup, SaveAs);
        // If the file is managed by a version control system save the vcs for this file.
        versionControls[fileName] = fileManagedByVCS ? versionControlForFile : 0;

        // Also save the buttongroup for every file to get the result for each entry.
        ReadOnlyFilesDialogPrivate::ButtonGroupForFile groupForFile;
        groupForFile.fileName = fileName;
        groupForFile.group = radioButtonGroup;
        buttonGroups.append(groupForFile);
        QObject::connect(radioButtonGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
                [this](int) { updateSelectAll(); });
    }

    // Apply the Mac file dialog style.
    if (Utils::HostOsInfo::isMacHost())
        ui.treeWidget->setAlternatingRowColors(true);

    // Do not show any options to the user if he has no choice.
    if (!useSaveAs && (!useVCS || !useMakeWritable)) {
        ui.treeWidget->setColumnHidden(MakeWritable, true);
        ui.treeWidget->setColumnHidden(OpenWithVCS, true);
        ui.treeWidget->setColumnHidden(SaveAs, true);
        ui.treeWidget->resizeColumnToContents(FileName);
        ui.treeWidget->resizeColumnToContents(Folder);
        ui.setAll->setVisible(false);
        ui.setAllLabel->setVisible(false);
        ui.verticalLayout->removeItem(ui.setAllLayout);
        if (useVCS)
            ui.msgLabel->setText(tr("The following files are not checked out yet.\n"
                                     "Do you want to check them out now?"));
        return;
    }

    // If there is just one file entry, there is no need to show the select all combo box
    if (fileNames.count() < 2) {
        ui.setAll->setVisible(false);
        ui.setAllLabel->setVisible(false);
        ui.verticalLayout->removeItem(ui.setAllLayout);
    }

    // Add items to the Set all combo box.
    ui.setAll->addItem(mixedText);
    setAllIndexForOperation[-1/*mixed*/] = ui.setAll->count() - 1;
    if (useVCS) {
        // If the files are managed by just one version control system, the Open and Make Writable
        // text for the specific system is used.
        if (!vcsOpenTextForAll.isEmpty() && vcsOpenTextForAll != versionControlOpenText) {
            versionControlOpenText = vcsOpenTextForAll;
            ui.treeWidget->headerItem()->setText(OpenWithVCS, versionControlOpenText);
        }
        if (!vcsMakeWritableTextForAll.isEmpty() && vcsMakeWritableTextForAll != makeWritableText) {
            makeWritableText = vcsMakeWritableTextForAll;
            ui.treeWidget->headerItem()->setText(MakeWritable, makeWritableText);
        }
        ui.setAll->addItem(versionControlOpenText);
        ui.setAll->setCurrentIndex(ui.setAll->count() - 1);
        setAllIndexForOperation[OpenWithVCS] = ui.setAll->count() - 1;
    }
    if (useMakeWritable) {
        ui.setAll->addItem(makeWritableText);
        setAllIndexForOperation[MakeWritable] = ui.setAll->count() - 1;
        if (ui.setAll->currentIndex() == -1)
            ui.setAll->setCurrentIndex(ui.setAll->count() - 1);
    }
    if (useSaveAs) {
        ui.setAll->addItem(saveAsText);
        setAllIndexForOperation[SaveAs] = ui.setAll->count() - 1;
    }
    QObject::connect(ui.setAll, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
                     [this](int index) { setAll(index); });

    // Filter which columns should be visible and resize them to content.
    for (int i = 0; i < NumberOfColumns; ++i) {
        if ((i == SaveAs && !useSaveAs) || (i == OpenWithVCS && !useVCS)
                || (i == MakeWritable && !useMakeWritable)) {
            ui.treeWidget->setColumnHidden(i, true);
            continue;
        }
        ui.treeWidget->resizeColumnToContents(i);
    }
}

}// namespace Core
