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

#include "bineditorplugin.h"
#include "bineditorwidget.h"
#include "bineditorconstants.h"
#include "bineditorservice.h"

#include <coreplugin/icore.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QVariant>

#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QToolBar>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/find/ifindsupport.h>
#include <coreplugin/idocument.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/reloadpromptutils.h>
#include <utils/qtcassert.h>

using namespace Utils;
using namespace Core;

namespace BinEditor {
namespace Internal {

class BinEditorFind : public IFindSupport
{
    Q_OBJECT

public:
    BinEditorFind(BinEditorWidget *widget)
    {
        m_widget = widget;
    }

    bool supportsReplace() const override { return false; }
    QString currentFindString() const override { return QString(); }
    QString completedFindString() const override { return QString(); }

    FindFlags supportedFindFlags() const override
    {
        return FindBackward | FindCaseSensitively;
    }

    void resetIncrementalSearch() override
    {
        m_incrementalStartPos = m_contPos = -1;
        m_incrementalWrappedState = false;
    }

    void highlightAll(const QString &txt, FindFlags findFlags) override
    {
        m_widget->highlightSearchResults(txt.toLatin1(), textDocumentFlagsForFindFlags(findFlags));
    }

    void clearHighlights() override
    {
        m_widget->highlightSearchResults(QByteArray());
    }

    int find(const QByteArray &pattern, int pos, FindFlags findFlags, bool *wrapped)
    {
        if (wrapped)
            *wrapped = false;
        if (pattern.isEmpty()) {
            m_widget->setCursorPosition(pos);
            return pos;
        }

        int res = m_widget->find(pattern, pos, textDocumentFlagsForFindFlags(findFlags));
        if (res < 0) {
            pos = (findFlags & FindBackward) ? -1 : 0;
            res = m_widget->find(pattern, pos, textDocumentFlagsForFindFlags(findFlags));
            if (res < 0)
                return res;
            if (wrapped)
                *wrapped = true;
        }
        return res;
    }

    Result findIncremental(const QString &txt, FindFlags findFlags) override
    {
        QByteArray pattern = txt.toLatin1();
        if (pattern != m_lastPattern)
            resetIncrementalSearch(); // Because we don't search for nibbles.
        m_lastPattern = pattern;
        if (m_incrementalStartPos < 0)
            m_incrementalStartPos = m_widget->selectionStart();
        if (m_contPos == -1)
            m_contPos = m_incrementalStartPos;
        bool wrapped;
        int found = find(pattern, m_contPos, findFlags, &wrapped);
        if (wrapped != m_incrementalWrappedState && (found >= 0)) {
            m_incrementalWrappedState = wrapped;
            showWrapIndicator(m_widget);
        }
        Result result;
        if (found >= 0) {
            result = Found;
            m_widget->highlightSearchResults(pattern, textDocumentFlagsForFindFlags(findFlags));
            m_contPos = -1;
        } else {
            if (found == -2) {
                result = NotYetFound;
                m_contPos +=
                        findFlags & FindBackward
                        ? -BinEditorWidget::SearchStride : BinEditorWidget::SearchStride;
            } else {
                result = NotFound;
                m_contPos = -1;
                m_widget->highlightSearchResults(QByteArray(), {});
            }
        }
        return result;
    }

    Result findStep(const QString &txt, FindFlags findFlags) override
    {
        QByteArray pattern = txt.toLatin1();
        bool wasReset = (m_incrementalStartPos < 0);
        if (m_contPos == -1) {
            m_contPos = m_widget->cursorPosition() + 1;
            if (findFlags & FindBackward)
                m_contPos = m_widget->selectionStart()-1;
        }
        bool wrapped;
        int found = find(pattern, m_contPos, findFlags, &wrapped);
        if (wrapped)
            showWrapIndicator(m_widget);
        Result result;
        if (found >= 0) {
            result = Found;
            m_incrementalStartPos = found;
            m_contPos = -1;
            if (wasReset)
                m_widget->highlightSearchResults(pattern, textDocumentFlagsForFindFlags(findFlags));
        } else if (found == -2) {
            result = NotYetFound;
            m_contPos += findFlags & FindBackward
                         ? -BinEditorWidget::SearchStride : BinEditorWidget::SearchStride;
        } else {
            result = NotFound;
            m_contPos = -1;
        }

        return result;
    }

private:
    BinEditorWidget *m_widget;
    qint64 m_incrementalStartPos = -1;
    qint64 m_contPos = -1; // Only valid if last result was NotYetFound.
    bool m_incrementalWrappedState = false;
    QByteArray m_lastPattern;
};


class BinEditorDocument : public IDocument
{
    Q_OBJECT
public:
    BinEditorDocument(BinEditorWidget *parent) :
        IDocument(parent)
    {
        setId(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID);
        setMimeType(QLatin1String(BinEditor::Constants::C_BINEDITOR_MIMETYPE));
        m_widget = parent;
        EditorService *es = m_widget->editorService();
        es->setFetchDataHandler([this](quint64 address) { provideData(address); });
        es->setNewRangeRequestHandler([this](quint64 offset) { provideNewRange(offset); });
        es->setDataChangedHandler([this](quint64, const QByteArray &) { contentsChanged(); });
    }

    QByteArray contents() const override
    {
        return m_widget->contents();
    }

    bool setContents(const QByteArray &contents) override
    {
        m_widget->clear();
        if (!contents.isEmpty()) {
            m_widget->setSizes(0, contents.length(), contents.length());
            m_widget->addData(0, contents);
        }
        return true;
    }

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override
    {
        return type == TypeRemoved ? BehaviorSilent : IDocument::reloadBehavior(state, type);
    }

    bool save(QString *errorString, const QString &fn, bool autoSave) override
    {
        QTC_ASSERT(!autoSave, return true); // bineditor does not support autosave - it would be a bit expensive
        const FilePath fileNameToUse = fn.isEmpty() ? filePath() : FilePath::fromString(fn);
        if (m_widget->save(errorString, filePath().toString(), fileNameToUse.toString())) {
            setFilePath(fileNameToUse);
            return true;
        } else {
            return false;
        }
    }

    OpenResult open(QString *errorString, const QString &fileName,
                    const QString &realFileName) override
    {
        QTC_CHECK(fileName == realFileName); // The bineditor can do no autosaving
        return openImpl(errorString, fileName);
    }

    OpenResult openImpl(QString *errorString, const QString &fileName, quint64 offset = 0)
    {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            file.close();
            quint64 size = static_cast<quint64>(file.size());
            if (size == 0) {
                QString msg = tr("The Binary Editor cannot open empty files.");
                if (errorString)
                    *errorString = msg;
                else
                    QMessageBox::critical(ICore::dialogParent(), tr("File Error"), msg);
                return OpenResult::CannotHandle;
            }
            if (size / 16 >= qint64(1) << 31) {
                // The limit is 2^31 lines (due to QText* interfaces) * 16 bytes per line.
                QString msg = tr("The file is too big for the Binary Editor (max. 32GB).");
                if (errorString)
                    *errorString = msg;
                else
                    QMessageBox::critical(ICore::dialogParent(), tr("File Error"), msg);
                return OpenResult::CannotHandle;
            }
            if (offset >= size)
                return OpenResult::CannotHandle;
            setFilePath(FilePath::fromString(fileName));
            m_widget->setSizes(offset, file.size());
            return OpenResult::Success;
        }
        QString errStr = tr("Cannot open %1: %2").arg(
                QDir::toNativeSeparators(fileName), file.errorString());
        if (errorString)
            *errorString = errStr;
        else
            QMessageBox::critical(ICore::dialogParent(), tr("File Error"), errStr);
        return OpenResult::ReadError;
    }

    void provideData(quint64 address)
    {
        const FilePath fn = filePath();
        if (fn.isEmpty())
            return;
        QFile file(fn.toString());
        if (file.open(QIODevice::ReadOnly)) {
            int blockSize = m_widget->dataBlockSize();
            file.seek(address);
            QByteArray data = file.read(blockSize);
            file.close();
            const int dataSize = data.size();
            if (dataSize != blockSize)
                data += QByteArray(blockSize - dataSize, 0);
            m_widget->addData(address, data);
        } else {
            QMessageBox::critical(ICore::dialogParent(), tr("File Error"),
                                  tr("Cannot open %1: %2").arg(
                                        fn.toUserOutput(), file.errorString()));
        }
    }

    void provideNewRange(quint64 offset)
    {
        if (filePath().exists())
            openImpl(nullptr, filePath().toString(), offset);
    }

public:
    bool isModified() const override
    {
        return isTemporary()/*e.g. memory view*/ ? false
                                                 : m_widget->isModified();
    }

    bool isFileReadOnly() const override {
        const FilePath fn = filePath();
        if (fn.isEmpty())
            return false;
        return !fn.toFileInfo().isWritable();
    }

    bool isSaveAsAllowed() const override { return true; }

    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override
    {
        if (flag == FlagIgnore)
            return true;
        if (type == TypePermissions) {
            emit changed();
        } else {
            emit aboutToReload();
            int cPos = m_widget->cursorPosition();
            m_widget->clear();
            const bool success = (openImpl(errorString, filePath().toString()) == OpenResult::Success);
            m_widget->setCursorPosition(cPos);
            emit reloadFinished(success);
            return success;
        }
        return true;
    }

private:
    BinEditorWidget *m_widget;
};

class BinEditor : public IEditor
{
    Q_OBJECT
public:
    BinEditor(BinEditorWidget *widget)
    {
        setWidget(widget);
        m_file = new BinEditorDocument(widget);
        m_addressEdit = new QLineEdit;
        auto addressValidator = new QRegularExpressionValidator(QRegularExpression("[0-9a-fA-F]{1,16}"), m_addressEdit);
        m_addressEdit->setValidator(addressValidator);

        auto l = new QHBoxLayout;
        auto w = new QWidget;
        l->setContentsMargins(0, 0, 5, 0);
        l->addStretch(1);
        l->addWidget(m_addressEdit);
        w->setLayout(l);

        m_toolBar = new QToolBar;
        m_toolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        m_toolBar->addWidget(w);

        widget->setEditor(this);

        connect(widget, &BinEditorWidget::cursorPositionChanged,
                this, &BinEditor::updateCursorPosition);
        connect(m_addressEdit, &QLineEdit::editingFinished,
                this, &BinEditor::jumpToAddress);
        connect(widget, &BinEditorWidget::modificationChanged,
                m_file, &IDocument::changed);
        updateCursorPosition(widget->cursorPosition());
    }

    ~BinEditor() override
    {
        delete m_widget;
    }

    IDocument *document() const override { return m_file; }

    QWidget *toolBar() override { return m_toolBar; }

private:
    void updateCursorPosition(qint64 position) {
        m_addressEdit->setText(QString::number(editorWidget()->baseAddress() + position, 16));
    }

    void jumpToAddress() {
        editorWidget()->jumpToAddress(m_addressEdit->text().toULongLong(nullptr, 16));
        updateCursorPosition(editorWidget()->cursorPosition());
    }

    BinEditorWidget *editorWidget() const
    {
        QTC_ASSERT(qobject_cast<BinEditorWidget *>(m_widget.data()), return nullptr);
        return static_cast<BinEditorWidget *>(m_widget.data());
    }

private:
    BinEditorDocument *m_file;
    QToolBar *m_toolBar;
    QLineEdit *m_addressEdit;
};

///////////////////////////////// BinEditorPluginPrivate //////////////////////////////////

class BinEditorPluginPrivate : public QObject
{
public:
    BinEditorPluginPrivate();
    ~BinEditorPluginPrivate() override;

    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_selectAllAction = nullptr;

    FactoryServiceImpl m_factoryService;
    BinEditorFactory m_editorFactory;
};

BinEditorPluginPrivate::BinEditorPluginPrivate()
{
    ExtensionSystem::PluginManager::addObject(&m_factoryService);
    ExtensionSystem::PluginManager::addObject(&m_editorFactory);

    m_undoAction = new QAction(BinEditorPlugin::tr("&Undo"), this);
    m_redoAction = new QAction(BinEditorPlugin::tr("&Redo"), this);
    m_copyAction = new QAction(this);
    m_selectAllAction = new QAction(this);

    Context context;
    context.add(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID);
    context.add(Constants::C_BINEDITOR);

    ActionManager::registerAction(m_undoAction, Core::Constants::UNDO, context);
    ActionManager::registerAction(m_redoAction, Core::Constants::REDO, context);
    ActionManager::registerAction(m_copyAction, Core::Constants::COPY, context);
    ActionManager::registerAction(m_selectAllAction, Core::Constants::SELECTALL, context);
}

BinEditorPluginPrivate::~BinEditorPluginPrivate()
{
    ExtensionSystem::PluginManager::removeObject(&m_editorFactory);
    ExtensionSystem::PluginManager::removeObject(&m_factoryService);
}

static BinEditorPluginPrivate *dd = nullptr;

///////////////////////////////// BinEditorFactory //////////////////////////////////

BinEditorFactory::BinEditorFactory()
{
    setId(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID);
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", Constants::C_BINEDITOR_DISPLAY_NAME));
    addMimeType(Constants::C_BINEDITOR_MIMETYPE);

    setEditorCreator([this] {
        auto widget = new BinEditorWidget();
        auto editor = new BinEditor(widget);

        connect(dd->m_undoAction, &QAction::triggered, widget, &BinEditorWidget::undo);
        connect(dd->m_redoAction, &QAction::triggered, widget, &BinEditorWidget::redo);
        connect(dd->m_copyAction, &QAction::triggered, widget, &BinEditorWidget::copy);
        connect(dd->m_selectAllAction, &QAction::triggered, widget, &BinEditorWidget::selectAll);

        auto updateActions = [widget] {
            dd->m_selectAllAction->setEnabled(true);
            dd->m_undoAction->setEnabled(widget->isUndoAvailable());
            dd->m_redoAction->setEnabled(widget->isRedoAvailable());
        };

        connect(widget, &BinEditorWidget::undoAvailable, widget, updateActions);
        connect(widget, &BinEditorWidget::redoAvailable, widget, updateActions);

        auto aggregate = new Aggregation::Aggregate;
        auto binEditorFind = new BinEditorFind(widget);
        aggregate->add(binEditorFind);
        aggregate->add(widget);

        return editor;
    });
}

///////////////////////////////// BinEditor Services //////////////////////////////////

EditorService *FactoryServiceImpl::createEditorService(const QString &title0, bool wantsEditor)
{
    BinEditorWidget *widget = nullptr;
    if (wantsEditor) {
        QString title = title0;
        IEditor *editor = EditorManager::openEditorWithContents(
                    Core::Constants::K_DEFAULT_BINARY_EDITOR_ID, &title);
        if (!editor)
            return nullptr;
        widget = qobject_cast<BinEditorWidget *>(editor->widget());
        widget->setEditor(editor);
    } else {
        widget = new BinEditorWidget;
        widget->setWindowTitle(title0);
    }
    return widget->editorService();
}

///////////////////////////////// BinEditorPlugin //////////////////////////////////

BinEditorPlugin::~BinEditorPlugin()
{
    delete dd;
    dd = nullptr;
}

bool BinEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    dd = new BinEditorPluginPrivate;

    return true;
}

} // namespace Internal
} // namespace BinEditor

#include "bineditorplugin.moc"
