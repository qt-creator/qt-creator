// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "readonlyfilesdialog.h"

#include "../coreplugintr.h"
#include "../editormanager/editormanager_p.h"
#include "../icore.h"
#include "../idocument.h"
#include "../iversioncontrol.h"
#include "../vcsmanager.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/stringutils.h>

#include <QButtonGroup>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QTreeWidget>

using namespace Utils;

namespace Core {
namespace Internal {

class ReadOnlyFilesDialogPrivate
{
public:
    ReadOnlyFilesDialogPrivate(ReadOnlyFilesDialog *parent, IDocument *document = nullptr, bool useSaveAs = false);
    ~ReadOnlyFilesDialogPrivate();

    enum ReadOnlyFilesTreeColumn {
        MakeWritable = ReadOnlyFilesDialog::MakeWritable,
        OpenWithVCS = ReadOnlyFilesDialog::OpenWithVCS,
        SaveAs = ReadOnlyFilesDialog::SaveAs,
        FileName = ReadOnlyFilesDialog::FileName,
        Folder = ReadOnlyFilesDialog::Folder,
        NumberOfColumns
    };

    void initDialog(const FilePaths &filePaths);
    void promptFailWarning(const FilePaths &files, ReadOnlyFilesDialog::ReadOnlyResult type) const;
    QRadioButton *createRadioButtonForItem(QTreeWidgetItem *item, QButtonGroup *group, ReadOnlyFilesTreeColumn type);

    void setAll(int index);
    void updateSelectAll();

    ReadOnlyFilesDialog *q;

    // Buttongroups containing the operation for one file.
    struct ButtonGroupForFile
    {
        FilePath filePath;
        QButtonGroup *group;
    };
    QList <ButtonGroupForFile> buttonGroups;

    QMap <int, int> setAllIndexForOperation;
    // The version control systems for every file, if the file isn't in VCS the value is 0.
    QHash<FilePath, IVersionControl*> versionControls;

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

    QLabel *m_msgLabel;
    QTreeWidget *m_treeWidget;
    QComboBox *m_setAll;
};

ReadOnlyFilesDialogPrivate::ReadOnlyFilesDialogPrivate(ReadOnlyFilesDialog *parent, IDocument *document, bool displaySaveAs)
    : q(parent)
    , useSaveAs(displaySaveAs)
    , useVCS(false)
    , showWarnings(false)
    , document(document)
    , mixedText(Tr::tr("Mixed"))
    , makeWritableText(Tr::tr("Make Writable"))
    , versionControlOpenText(Tr::tr("Open with VCS"))
    , saveAsText(Tr::tr("Save As"))
{}

ReadOnlyFilesDialogPrivate::~ReadOnlyFilesDialogPrivate()
{
    for (const ButtonGroupForFile &groupForFile : std::as_const(buttonGroups))
        delete groupForFile.group;
}

} // namespace Internal

using namespace Internal;

/*!
 * \class Core::ReadOnlyFilesDialog
 * \inmodule QtCreator
 * \internal
 * \brief The ReadOnlyFilesDialog class implements a dialog to show a set of
 * files that are classified as not writable.
 *
 * Automatically checks which operations are allowed to make the file writable. These operations
 * are \c MakeWritable (RO_MakeWritable), which tries to set the file permissions in the file system,
 * \c OpenWithVCS (RO_OpenVCS) if the open operation is allowed by the version control system,
 * and \c SaveAs (RO_SaveAs), which is used to save the changes to a document under another file
 * name.
 */

/*! \enum ReadOnlyFilesDialog::ReadOnlyResult
    This enum holds the operations that are allowed to make the file writable.

     \value RO_Cancel
            Cancels the operation.
     \value RO_OpenVCS
            Opens the file under control of the version control system.
     \value RO_MakeWritable
            Sets the file permissions in the file system.
     \value RO_SaveAs
            Saves changes to a document under another file name.
*/

ReadOnlyFilesDialog::ReadOnlyFilesDialog(const Utils::FilePaths &filePaths, QWidget *parent)
    : QDialog(parent)
    , d(new ReadOnlyFilesDialogPrivate(this))
{
    d->initDialog(filePaths);
}

ReadOnlyFilesDialog::ReadOnlyFilesDialog(const Utils::FilePath &filePath, QWidget *parent)
    : QDialog(parent)
    , d(new ReadOnlyFilesDialogPrivate(this))
{
    d->initDialog({filePath});
}

ReadOnlyFilesDialog::ReadOnlyFilesDialog(IDocument *document, QWidget *parent,
                                         bool displaySaveAs)
    : QDialog(parent)
    , d(new ReadOnlyFilesDialogPrivate(this, document, displaySaveAs))
{
    d->initDialog({document->filePath()});
}

ReadOnlyFilesDialog::ReadOnlyFilesDialog(const QList<IDocument *> &documents, QWidget *parent)
    : QDialog(parent)
    , d(new ReadOnlyFilesDialogPrivate(this))
{
    FilePaths files;
    for (IDocument *document : documents)
        files << document->filePath();
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
    d->m_msgLabel->setText(message);
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
void ReadOnlyFilesDialogPrivate::promptFailWarning(const FilePaths &files, ReadOnlyFilesDialog::ReadOnlyResult type) const
{
    if (files.isEmpty())
        return;
    QString title;
    QString message;
    QString details;
    if (files.count() == 1) {
        const FilePath file = files.first();
        switch (type) {
        case ReadOnlyFilesDialog::RO_OpenVCS: {
            if (IVersionControl *vc = versionControls[file]) {
                const QString openText = Utils::stripAccelerator(vc->vcsOpenText());
                title = Tr::tr("Failed to %1 File").arg(openText);
                message = Tr::tr("%1 file %2 from version control system %3 failed.")
                        .arg(openText)
                        .arg(file.toUserOutput())
                        .arg(vc->displayName())
                    + '\n'
                    + failWarning;
            } else {
                title = Tr::tr("No Version Control System Found");
                message = Tr::tr("Cannot open file %1 from version control system.\n"
                             "No version control system found.")
                        .arg(file.toUserOutput())
                    + '\n'
                    + failWarning;
            }
            break;
        }
        case ReadOnlyFilesDialog::RO_MakeWritable:
            title = Tr::tr("Cannot Set Permissions");
            message = Tr::tr("Cannot set permissions for %1 to writable.")
                    .arg(file.toUserOutput())
                + '\n'
                + failWarning;
            break;
        case ReadOnlyFilesDialog::RO_SaveAs:
            title = Tr::tr("Cannot Save File");
            message = Tr::tr("Cannot save file %1").arg(file.toUserOutput())
                + '\n'
                + failWarning;
            break;
        default:
            title = Tr::tr("Canceled Changing Permissions");
            message = failWarning;
            break;
        }
    } else {
        title = Tr::tr("Could Not Change Permissions on Some Files");
        message = failWarning + QLatin1Char('\n')
                + Tr::tr("See details for a complete list of files.");
        details = Utils::transform(files, &FilePath::toString).join('\n');
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
    FilePaths failedToMakeWritable;
    for (const ReadOnlyFilesDialogPrivate::ButtonGroupForFile &buttongroup
         : std::as_const(d->buttonGroups)) {
        result = static_cast<ReadOnlyResult>(buttongroup.group->checkedId());
        switch (result) {
        case RO_MakeWritable:
            if (!Utils::FileUtils::makeWritable(buttongroup.filePath)) {
                failedToMakeWritable << buttongroup.filePath;
                continue;
            }
            break;
        case RO_OpenVCS:
            if (!d->versionControls[buttongroup.filePath]->vcsOpen(buttongroup.filePath)) {
                failedToMakeWritable << buttongroup.filePath;
                continue;
            }
            break;
        case RO_SaveAs:
            if (!EditorManagerPrivate::saveDocumentAs(d->document)) {
                failedToMakeWritable << buttongroup.filePath;
                continue;
            }
            break;
        default:
            failedToMakeWritable << buttongroup.filePath;
            continue;
        }
        if (!buttongroup.filePath.toFileInfo().isWritable())
            failedToMakeWritable << buttongroup.filePath;
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
    auto radioButton = new QRadioButton(q);
    group->addButton(radioButton, type);
    item->setTextAlignment(type, Qt::AlignHCenter);
    m_treeWidget->setItemWidget(item, type, radioButton);
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
    for (const ReadOnlyFilesDialogPrivate::ButtonGroupForFile &groupForFile :
         std::as_const(buttonGroups)) {
        auto radioButton = qobject_cast<QRadioButton *>(groupForFile.group->button(type));
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
    for (const ReadOnlyFilesDialogPrivate::ButtonGroupForFile &groupForFile :
         std::as_const(buttonGroups)) {
        if (selectedOperation == -1) {
            selectedOperation = groupForFile.group->checkedId();
        } else if (selectedOperation != groupForFile.group->checkedId()) {
            m_setAll->setCurrentIndex(0);
            return;
        }
    }
    m_setAll->setCurrentIndex(setAllIndexForOperation[selectedOperation]);
}

/*!
 * Adds files to the dialog and checks for a possible operation to make the file
 * writable.
 * \a filePaths contains the list of the files that should be added to the
 * dialog.
 * \internal
 */
void ReadOnlyFilesDialogPrivate::initDialog(const FilePaths &filePaths)
{
    q->resize(639, 217);
    q->setWindowTitle(Tr::tr("Files Without Write Permissions"));

    m_msgLabel = new QLabel(Tr::tr(
        "The following files have no write permissions. Do you want to change the permissions?"));

    m_treeWidget = new QTreeWidget;
    auto headerItem = new QTreeWidgetItem;
    headerItem->setText(0, Tr::tr("Make Writable"));
    headerItem->setText(1, Tr::tr("Open with VCS"));
    headerItem->setText(2, Tr::tr("Save As"));
    headerItem->setText(3, Tr::tr("Filename"));
    headerItem->setText(4, Tr::tr("Path"));
    m_treeWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_treeWidget->setTextElideMode(Qt::ElideLeft);
    m_treeWidget->setIndentation(0);
    m_treeWidget->setUniformRowHeights(true);
    m_treeWidget->setItemsExpandable(false);
    m_treeWidget->setColumnCount(5);
    m_treeWidget->setHeaderItem(headerItem);

    m_setAll = new QComboBox;

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::NoButton);
    buttonBox->addButton(Tr::tr("Change &Permission"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, q, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, q, &QDialog::reject);

    using namespace Layouting;

    QWidget *setAllWidget = Row {
        Tr::tr("Select all, if possible: "),
        m_setAll,
        st,
        noMargin
    }.emerge();

    // clang-format off
    Column {
        m_msgLabel,
        m_treeWidget,
        setAllWidget,
        buttonBox
    }.attachTo(q);
    // clang-format on

    QString vcsOpenTextForAll;
    QString vcsMakeWritableTextForAll;
    bool useMakeWritable = false;
    for (const FilePath &filePath : filePaths) {
        const QString visibleName = filePath.fileName();
        const FilePath directory = filePath.absolutePath();

        // Setup a default entry with filename folder and make writable radio button.
        auto item = new QTreeWidgetItem(m_treeWidget);
        item->setText(FileName, visibleName);
        item->setIcon(FileName, FileIconProvider::icon(filePath));
        item->setText(Folder, directory.shortNativePath());
        auto radioButtonGroup = new QButtonGroup;

        // Add a button for opening the file with a version control system
        // if the file is managed by an version control system which allows opening files.
        IVersionControl *versionControlForFile =
                VcsManager::findVersionControlForDirectory(directory);
        const bool fileManagedByVCS = versionControlForFile
                && versionControlForFile->openSupportMode(filePath) != IVersionControl::NoOpen;
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
            if (versionControlForFile->openSupportMode(filePath) == IVersionControl::OpenOptional) {
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
        versionControls[filePath] = fileManagedByVCS ? versionControlForFile : nullptr;

        // Also save the buttongroup for every file to get the result for each entry.
        buttonGroups.append({filePath, radioButtonGroup});
        QObject::connect(radioButtonGroup, &QButtonGroup::buttonClicked,
                         q, [this] { updateSelectAll(); });
    }

    // Apply the Mac file dialog style.
    if (Utils::HostOsInfo::isMacHost())
        m_treeWidget->setAlternatingRowColors(true);

    // Do not show any options to the user if he has no choice.
    if (!useSaveAs && (!useVCS || !useMakeWritable)) {
        m_treeWidget->setColumnHidden(MakeWritable, true);
        m_treeWidget->setColumnHidden(OpenWithVCS, true);
        m_treeWidget->setColumnHidden(SaveAs, true);
        m_treeWidget->resizeColumnToContents(FileName);
        m_treeWidget->resizeColumnToContents(Folder);
        setAllWidget->setVisible(false);
        if (useVCS) {
            m_msgLabel->setText(Tr::tr("The following files are not checked out yet.\n"
                                       "Do you want to check them out now?"));
        }
        return;
    }

    // If there is just one file entry, there is no need to show the select all combo box
    if (filePaths.count() < 2)
        setAllWidget->setVisible(false);

    // Add items to the Set all combo box.
    m_setAll->addItem(mixedText);
    setAllIndexForOperation[-1 /*mixed*/] = m_setAll->count() - 1;
    if (useVCS) {
        // If the files are managed by just one version control system, the Open and Make Writable
        // text for the specific system is used.
        if (!vcsOpenTextForAll.isEmpty() && vcsOpenTextForAll != versionControlOpenText) {
            versionControlOpenText = vcsOpenTextForAll;
            m_treeWidget->headerItem()->setText(OpenWithVCS, versionControlOpenText);
        }
        if (!vcsMakeWritableTextForAll.isEmpty() && vcsMakeWritableTextForAll != makeWritableText) {
            makeWritableText = vcsMakeWritableTextForAll;
            m_treeWidget->headerItem()->setText(MakeWritable, makeWritableText);
        }
        m_setAll->addItem(versionControlOpenText);
        m_setAll->setCurrentIndex(m_setAll->count() - 1);
        setAllIndexForOperation[OpenWithVCS] = m_setAll->count() - 1;
    }
    if (useMakeWritable) {
        m_setAll->addItem(makeWritableText);
        setAllIndexForOperation[MakeWritable] = m_setAll->count() - 1;
        if (m_setAll->currentIndex() == -1)
            m_setAll->setCurrentIndex(m_setAll->count() - 1);
    }
    if (useSaveAs) {
        m_setAll->addItem(saveAsText);
        setAllIndexForOperation[SaveAs] = m_setAll->count() - 1;
    }
    QObject::connect(m_setAll, &QComboBox::activated, q, [this](int index) { setAll(index); });

    // Filter which columns should be visible and resize them to content.
    for (int i = 0; i < NumberOfColumns; ++i) {
        if ((i == SaveAs && !useSaveAs) || (i == OpenWithVCS && !useVCS)
                || (i == MakeWritable && !useMakeWritable)) {
            m_treeWidget->setColumnHidden(i, true);
            continue;
        }
        m_treeWidget->resizeColumnToContents(i);
    }
}

}// namespace Core
