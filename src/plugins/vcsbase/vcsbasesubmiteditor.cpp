// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsbasesubmiteditor.h"

#include "commonvcssettings.h"
#include "nicknamedialog.h"
#include "submiteditorfile.h"
#include "submiteditorwidget.h"
#include "submitfieldwidget.h"
#include "submitfilemodel.h"
#include "vcsbaseplugin.h"
#include "vcsbasetr.h"
#include "vcsoutputwindow.h"
#include "vcsplugin.h"

#include <aggregation/aggregate.h>

#include <coreplugin/find/basetextfind.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <extensionsystem/invoker.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/completingtextedit.h>
#include <utils/fileutils.h>
#include <utils/icon.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>
#include <utils/theme/theme.h>

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <QAction>
#include <QApplication>
#include <QCompleter>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QSet>
#include <QStringListModel>
#include <QStyle>
#include <QToolBar>

#include <cstring>

enum { debug = 0 };

// Return true if word is meaningful and can be added to a completion model
static bool acceptsWordForCompletion(const QString &word)
{
    return word.size() >= 7;
}

/*!
    \class VcsBase::VcsBaseSubmitEditorParameters

    \brief The VcsBaseSubmitEditorParameters class is a utility class
    to parametrize a VcsBaseSubmitEditor.
*/

/*!
    \class  VcsBase::VcsBaseSubmitEditor

    \brief The VcsBaseSubmitEditor class is the base class for a submit editor
    based on the SubmitEditorWidget.

    Presents the commit message in a text editor and an
    checkable list of modified files in a list window. The user can delete
    files from the list by pressing unchecking them or diff the selection
    by doubleclicking.

    The action matching the ids (unless 0) of the parameter struct will be
    registered with the EditorWidget and submit/diff actions will be added to
    a toolbar.

    For the given context, there must be only one instance of the editor
    active.
    To start a submit, set the submit template on the editor and the output
    of the VCS status command listing the modified files as fileList and open
    it.

    The submit process is started by listening on the editor close
    signal and then asking the IDocument interface of the editor to save the file
    within a DocumentManager::blockFileChange() section
    and to launch the submit process. In addition, the action registered
    for submit should be connected to a slot triggering the close of the
    current editor in the editor manager.
*/

namespace VcsBase {

using namespace Internal;
using namespace Utils;

class VcsBaseSubmitEditorPrivate
{
public:
    VcsBaseSubmitEditorPrivate(SubmitEditorWidget *editorWidget,
                               VcsBaseSubmitEditor *q);

    SubmitEditorWidget *m_widget;
    VcsBaseSubmitEditorParameters m_parameters;
    QString m_displayName;
    FilePath m_checkScriptWorkingDirectory;
    SubmitEditorFile m_file;

    QPointer<QAction> m_diffAction;
    QPointer<QAction> m_submitAction;

    NickNameDialog *m_nickNameDialog = nullptr;
    bool m_disablePrompt = false;
};

VcsBaseSubmitEditorPrivate::VcsBaseSubmitEditorPrivate(SubmitEditorWidget *editorWidget,
                                                       VcsBaseSubmitEditor *q) :
    m_widget(editorWidget), m_file(q)
{
    auto completer = new QCompleter(q);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    m_widget->descriptionEdit()->setCompleter(completer);
    m_widget->descriptionEdit()->setCompletionLengthThreshold(4);
}

VcsBaseSubmitEditor::VcsBaseSubmitEditor(SubmitEditorWidget *editorWidget)
{
    setWidget(editorWidget);
    d = new VcsBaseSubmitEditorPrivate(editorWidget, this);
}

void VcsBaseSubmitEditor::setParameters(const VcsBaseSubmitEditorParameters &parameters)
{
    d->m_parameters = parameters;
    d->m_file.setId(parameters.id);
    d->m_file.setMimeType(QLatin1String(parameters.mimeType));

    setWidget(d->m_widget);
    document()->setPreferredDisplayName(Tr::tr(d->m_parameters.displayName));

    // Message font according to settings
    CompletingTextEdit *descriptionEdit = d->m_widget->descriptionEdit();
    const TextEditor::FontSettings fs = TextEditor::TextEditorSettings::fontSettings();
    const QTextCharFormat tf = fs.toTextCharFormat(TextEditor::C_TEXT);
    descriptionEdit->setFont(tf.font());
    const QTextCharFormat selectionFormat = fs.toTextCharFormat(TextEditor::C_SELECTION);
    QPalette pal;
    pal.setColor(QPalette::Base, tf.background().color());
    pal.setColor(QPalette::Text, tf.foreground().color());
    pal.setColor(QPalette::WindowText, tf.foreground().color());
    if (selectionFormat.background().style() != Qt::NoBrush)
        pal.setColor(QPalette::Highlight, selectionFormat.background().color());
    pal.setBrush(QPalette::HighlightedText, selectionFormat.foreground());
    descriptionEdit->setPalette(pal);

    d->m_file.setModified(false);
    // We are always clean to prevent the editor manager from asking to save.

    connect(d->m_widget, &SubmitEditorWidget::diffSelected,
            this, &VcsBaseSubmitEditor::slotDiffSelectedVcsFiles);
    connect(descriptionEdit, &QTextEdit::textChanged,
            this, &VcsBaseSubmitEditor::fileContentsChanged);

    const CommonVcsSettings &settings = commonSettings();
    // Add additional context menu settings
    if (!settings.submitMessageCheckScript().isEmpty()
            || !settings.nickNameMailMap().isEmpty()) {
        auto sep = new QAction(this);
        sep->setSeparator(true);
        d->m_widget->addDescriptionEditContextMenuAction(sep);
        // Run check action
        if (!settings.submitMessageCheckScript().isEmpty()) {
            auto checkAction = new QAction(Tr::tr("Check Message"), this);
            connect(checkAction, &QAction::triggered,
                    this, &VcsBaseSubmitEditor::slotCheckSubmitMessage);
            d->m_widget->addDescriptionEditContextMenuAction(checkAction);
        }
        // Insert nick
        if (!settings.nickNameMailMap().isEmpty()) {
            auto insertAction = new QAction(Tr::tr("Insert Name..."), this);
            connect(insertAction, &QAction::triggered, this, &VcsBaseSubmitEditor::slotInsertNickName);
            d->m_widget->addDescriptionEditContextMenuAction(insertAction);
        }
    }
    // Do we have user fields?
    if (!settings.nickNameFieldListFile().isEmpty())
        createUserFields(settings.nickNameFieldListFile());

    // wrapping. etc
    slotUpdateEditorSettings();
    connect(&settings, &CommonVcsSettings::changed,
            this, &VcsBaseSubmitEditor::slotUpdateEditorSettings);
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, [this] {
                if (Core::EditorManager::currentEditor() == this)
                    updateFileModel();
            });
    connect(qApp, &QApplication::applicationStateChanged,
            this, [this](Qt::ApplicationState state) {
                if (state == Qt::ApplicationActive)
                    updateFileModel();
            });

    auto aggregate = new Aggregation::Aggregate;
    aggregate->add(new Core::BaseTextFind(descriptionEdit));
    aggregate->add(this);
}

VcsBaseSubmitEditor::~VcsBaseSubmitEditor()
{
    delete d->m_widget;
    delete d;
}

void VcsBaseSubmitEditor::slotUpdateEditorSettings()
{
    setLineWrapWidth(commonSettings().lineWrapWidth());
    setLineWrap(commonSettings().lineWrap());
}

// Return a trimmed list of non-empty field texts
static inline QStringList fieldTexts(const QString &fileContents)
{
    QStringList rc;
    const QStringList rawFields = fileContents.trimmed().split(QLatin1Char('\n'));
    for (const QString &field : rawFields) {
        const QString trimmedField = field.trimmed();
        if (!trimmedField.isEmpty())
            rc.push_back(trimmedField);
    }
    return rc;
}

void VcsBaseSubmitEditor::createUserFields(const FilePath &fieldConfigFile)
{
    FileReader reader;
    if (!reader.fetch(fieldConfigFile, QIODevice::Text, Core::ICore::dialogParent()))
        return;

    // Parse into fields
    const QStringList fields = fieldTexts(QString::fromUtf8(reader.data()));
    if (fields.empty())
        return;
    // Create a completer on user names
    const QStandardItemModel *nickNameModel = VcsPlugin::instance()->nickNameModel();
    auto completer = new QCompleter(NickNameDialog::nickNameList(nickNameModel), this);

    auto fieldWidget = new SubmitFieldWidget;
    connect(fieldWidget, &SubmitFieldWidget::browseButtonClicked,
            this, &VcsBaseSubmitEditor::slotSetFieldNickName);
    fieldWidget->setCompleter(completer);
    fieldWidget->setAllowDuplicateFields(true);
    fieldWidget->setHasBrowseButton(true);
    fieldWidget->setFields(fields);
    d->m_widget->addSubmitFieldWidget(fieldWidget);
}

void VcsBaseSubmitEditor::registerActions(QAction *editorUndoAction, QAction *editorRedoAction,
                                          QAction *submitAction, QAction *diffAction)
{
    d->m_widget->registerActions(editorUndoAction, editorRedoAction, submitAction, diffAction);
    d->m_diffAction = diffAction;
    d->m_submitAction = submitAction;
}

QAbstractItemView::SelectionMode VcsBaseSubmitEditor::fileListSelectionMode() const
{
    return d->m_widget->fileListSelectionMode();
}

void VcsBaseSubmitEditor::setFileListSelectionMode(QAbstractItemView::SelectionMode sm)
{
    d->m_widget->setFileListSelectionMode(sm);
}

bool VcsBaseSubmitEditor::isEmptyFileListEnabled() const
{
    return d->m_widget->isEmptyFileListEnabled();
}

void VcsBaseSubmitEditor::setEmptyFileListEnabled(bool e)
{
    d->m_widget->setEmptyFileListEnabled(e);
}

bool VcsBaseSubmitEditor::lineWrap() const
{
    return d->m_widget->lineWrap();
}

void VcsBaseSubmitEditor::setLineWrap(bool w)
{
    d->m_widget->setLineWrap(w);
}

int VcsBaseSubmitEditor::lineWrapWidth() const
{
    return d->m_widget->lineWrapWidth();
}

void VcsBaseSubmitEditor::setLineWrapWidth(int w)
{
    d->m_widget->setLineWrapWidth(w);
}

Core::IDocument *VcsBaseSubmitEditor::document() const
{
    return &d->m_file;
}

FilePath VcsBaseSubmitEditor::checkScriptWorkingDirectory() const
{
    return d->m_checkScriptWorkingDirectory;
}

void VcsBaseSubmitEditor::setCheckScriptWorkingDirectory(const FilePath &s)
{
    d->m_checkScriptWorkingDirectory = s;
}

QStringList VcsBaseSubmitEditor::checkedFiles() const
{
    return d->m_widget->checkedFiles();
}

static QSet<FilePath> filesFromModel(SubmitFileModel *model)
{
    QSet<FilePath> result;
    result.reserve(model->rowCount());
    for (int row = 0; row < model->rowCount(); ++row) {
        result.insert(model->repositoryRoot().resolvePath(model->file(row)).absoluteFilePath());
    }
    return result;
}

void VcsBaseSubmitEditor::setFileModel(SubmitFileModel *model)
{
    QTC_ASSERT(model, return);
    SubmitFileModel *oldModel = d->m_widget->fileModel();
    QList<int> selected;
    if (oldModel) {
        model->updateSelections(oldModel);
        selected = d->m_widget->selectedRows();
    }
    d->m_widget->setFileModel(model);
    delete oldModel;
    if (!selected.isEmpty())
        d->m_widget->setSelectedRows(selected);

    const QSet<FilePath> files = filesFromModel(model);
    // add file names to completion
    QSet<QString> completionItems = Utils::transform(files, &FilePath::fileName);
    QObject *cppModelManager = ExtensionSystem::PluginManager::getObjectByName("CppModelManager");
    if (cppModelManager) {
        const auto symbols = ExtensionSystem::invoke<QSet<QString>>(cppModelManager,
                                                                    "symbolsInFiles",
                                                                    files);
        completionItems += Utils::filtered(symbols, acceptsWordForCompletion);
    }

    // Populate completer with symbols
    if (!completionItems.isEmpty()) {
        QCompleter *completer = d->m_widget->descriptionEdit()->completer();
        QStringList symbolsList = Utils::toList(completionItems);
        symbolsList.sort();
        completer->setModel(new QStringListModel(symbolsList, completer));
    }
}

SubmitFileModel *VcsBaseSubmitEditor::fileModel() const
{
    return d->m_widget->fileModel();
}

QStringList VcsBaseSubmitEditor::rowsToFiles(const QList<int> &rows) const
{
    if (rows.empty())
        return {};

    QStringList rc;
    const SubmitFileModel *model = fileModel();
    const int count = rows.size();
    for (int i = 0; i < count; i++)
        rc.push_back(model->file(rows.at(i)));
    return rc;
}

void VcsBaseSubmitEditor::slotDiffSelectedVcsFiles(const QList<int> &rawList)
{
    if (d->m_parameters.diffType == VcsBaseSubmitEditorParameters::DiffRows)
        emit diffSelectedRows(rawList);
    else
        emit diffSelectedFiles(rowsToFiles(rawList));
}

QByteArray VcsBaseSubmitEditor::fileContents() const
{
    return description().toLocal8Bit();
}

bool VcsBaseSubmitEditor::setFileContents(const QByteArray &contents)
{
    setDescription(QString::fromUtf8(contents));
    return true;
}

QString VcsBaseSubmitEditor::description() const
{
    return d->m_widget->descriptionText();
}

void VcsBaseSubmitEditor::setDescription(const QString &text)
{
    d->m_widget->setDescriptionText(text);
}

bool VcsBaseSubmitEditor::isDescriptionMandatory() const
{
    return d->m_widget->isDescriptionMandatory();
}

void VcsBaseSubmitEditor::setDescriptionMandatory(bool v)
{
    d->m_widget->setDescriptionMandatory(v);
}

enum { checkDialogMinimumWidth = 500 };

void VcsBaseSubmitEditor::accept(VcsBasePluginPrivate *plugin)
{
    auto submitWidget = static_cast<SubmitEditorWidget *>(this->widget());

    Core::EditorManager::activateEditor(this, Core::EditorManager::IgnoreNavigationHistory);

    QString errorMessage;
    const bool canCommit = checkSubmitMessage(&errorMessage) && submitWidget->canSubmit(&errorMessage);
    if (!canCommit) {
        VcsOutputWindow::appendError(plugin->commitErrorMessage(errorMessage));
    } else if (plugin->activateCommit()) {
        close();
    }
}

void VcsBaseSubmitEditor::close()
{
    d->m_disablePrompt = true;
    Core::EditorManager::closeDocuments({document()});
}

bool VcsBaseSubmitEditor::promptSubmit(VcsBasePluginPrivate *plugin)
{
    if (d->m_disablePrompt)
        return true;

    Core::EditorManager::activateEditor(this, Core::EditorManager::IgnoreNavigationHistory);

    auto submitWidget = static_cast<SubmitEditorWidget *>(this->widget());
    if (!submitWidget->isEnabled() || !submitWidget->isEdited())
        return true;

    QMessageBox mb(Core::ICore::dialogParent());
    mb.setWindowTitle(plugin->commitAbortTitle());
    mb.setIcon(QMessageBox::Warning);
    mb.setText(plugin->commitAbortMessage());
    QPushButton *closeButton = mb.addButton(Tr::tr("&Close"), QMessageBox::AcceptRole);
    QPushButton *keepButton = mb.addButton(Tr::tr("&Keep Editing"), QMessageBox::RejectRole);
    mb.setDefaultButton(keepButton);
    mb.setEscapeButton(keepButton);
    mb.exec();
    return mb.clickedButton() == closeButton;
}

QString VcsBaseSubmitEditor::promptForNickName()
{
    if (!d->m_nickNameDialog)
        d->m_nickNameDialog = new NickNameDialog(VcsPlugin::instance()->nickNameModel(), d->m_widget);
    if (d->m_nickNameDialog->exec() == QDialog::Accepted)
       return d->m_nickNameDialog->nickName();
    return {};
}

void VcsBaseSubmitEditor::slotInsertNickName()
{
    const QString nick = promptForNickName();
    if (!nick.isEmpty())
        d->m_widget->descriptionEdit()->textCursor().insertText(nick);
}

void VcsBaseSubmitEditor::slotSetFieldNickName(int i)
{
    if (SubmitFieldWidget *sfw = d->m_widget->submitFieldWidgets().constFirst()) {
        const QString nick = promptForNickName();
        if (!nick.isEmpty())
            sfw->setFieldValue(i, nick);
    }
}

void VcsBaseSubmitEditor::slotCheckSubmitMessage()
{
    QString errorMessage;
    if (!checkSubmitMessage(&errorMessage)) {
        QMessageBox msgBox(QMessageBox::Warning, Tr::tr("Submit Message Check Failed"),
                           errorMessage, QMessageBox::Ok, d->m_widget);
        msgBox.setMinimumWidth(checkDialogMinimumWidth);
        msgBox.exec();
    }
}

bool VcsBaseSubmitEditor::checkSubmitMessage(QString *errorMessage) const
{
    const FilePath checkScript = commonSettings().submitMessageCheckScript();
    if (checkScript.isEmpty())
        return true;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool rc = runSubmitMessageCheckScript(checkScript, errorMessage);
    QApplication::restoreOverrideCursor();
    return rc;
}

static QString msgCheckScript(const FilePath &workingDir, const FilePath &cmd)
{
    const QString nativeCmd = cmd.toUserOutput();
    return workingDir.isEmpty() ?
           Tr::tr("Executing %1").arg(nativeCmd) :
           Tr::tr("Executing [%1] %2").arg(workingDir.toUserOutput(), nativeCmd);
}

bool VcsBaseSubmitEditor::runSubmitMessageCheckScript(const FilePath &checkScript, QString *errorMessage) const
{
    QTC_ASSERT(!checkScript.needsDevice(), return false); // Not supported below.
    // Write out message
    TempFileSaver saver(TemporaryDirectory::masterDirectoryPath() + "/msgXXXXXX.txt");
    saver.write(fileContents());
    if (!saver.finalize(errorMessage))
        return false;
    // Run check process
    VcsOutputWindow::appendShellCommandLine(msgCheckScript(d->m_checkScriptWorkingDirectory,
                                                           checkScript));
    Process checkProcess;
    if (!d->m_checkScriptWorkingDirectory.isEmpty())
        checkProcess.setWorkingDirectory(d->m_checkScriptWorkingDirectory);
    checkProcess.setCommand({checkScript, {saver.filePath().path()}});
    checkProcess.start();
    const bool succeeded = checkProcess.waitForFinished();

    const QString stdOut = checkProcess.stdOut();
    if (!stdOut.isEmpty())
        VcsOutputWindow::appendSilently(stdOut);
    const QString stdErr = checkProcess.stdErr();
    if (!stdErr.isEmpty())
        VcsOutputWindow::appendSilently(stdErr);

    if (!succeeded)
        *errorMessage = checkProcess.exitMessage();

    return succeeded;
}

QIcon VcsBaseSubmitEditor::diffIcon()
{
    using namespace Utils;
    return Icon({
        {":/vcsbase/images/diff_documents.png", Theme::PanelTextColorDark},
        {":/vcsbase/images/diff_arrows.png", Theme::IconsStopColor}
    }, Icon::Tint).icon();
}

QIcon VcsBaseSubmitEditor::submitIcon()
{
    using namespace Utils;
    return Icon({
        {":/vcsbase/images/submit_db.png", Theme::PanelTextColorDark},
        {":/vcsbase/images/submit_arrow.png", Theme::IconsRunColor}
    }, Icon::Tint | Icon::PunchEdges).icon();
}

// Reduce a list of untracked files reported by a VCS down to the files
// that are actually part of the current project(s).
void VcsBaseSubmitEditor::filterUntrackedFilesOfProject(const FilePath &repositoryDirectory,
                                                        QStringList *untrackedFiles)
{
    for (QStringList::iterator it = untrackedFiles->begin(); it != untrackedFiles->end(); ) {
        const FilePath path = repositoryDirectory.resolvePath(*it).absoluteFilePath();
        if (ProjectExplorer::ProjectManager::projectForFile(path))
            ++it;
        else
            it = untrackedFiles->erase(it);
    }
}

} // namespace VcsBase
