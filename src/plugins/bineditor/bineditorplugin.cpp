// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bineditorplugin.h"

#include "bineditorconstants.h"
#include "bineditorservice.h"
#include "bineditortr.h"
#include "bineditorwidget.h"

#include <coreplugin/coreplugintr.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icore.h>

#include <texteditor/codecchooser.h>

#include <QAction>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpressionValidator>
#include <QTextCodec>
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

namespace BinEditor::Internal {

class BinEditorFactory final : public QObject, public IEditorFactory
{
public:
    BinEditorFactory();
};

class BinEditorFind : public IFindSupport
{
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
        m_widget->highlightSearchResults(txt.toLatin1(),
                                         Utils::textDocumentFlagsForFindFlags(findFlags));
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

        int res = m_widget->find(pattern, pos, Utils::textDocumentFlagsForFindFlags(findFlags));
        if (res < 0) {
            pos = (findFlags & FindBackward) ? -1 : 0;
            res = m_widget->find(pattern, pos, Utils::textDocumentFlagsForFindFlags(findFlags));
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
            m_widget->highlightSearchResults(pattern,
                                             Utils::textDocumentFlagsForFindFlags(findFlags));
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
            if (wasReset) {
                m_widget->highlightSearchResults(pattern,
                                                 Utils::textDocumentFlagsForFindFlags(findFlags));
            }
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

    OpenResult open(QString *errorString, const FilePath &filePath,
                    const FilePath &realFilePath) override
    {
        QTC_CHECK(filePath == realFilePath); // The bineditor can do no autosaving
        return openImpl(errorString, filePath);
    }

    OpenResult openImpl(QString *errorString, const FilePath &filePath, quint64 offset = 0)
    {
        const qint64 size = filePath.fileSize();
        if (size < 0) {
            QString msg = Tr::tr("Cannot open %1: %2").arg(filePath.toUserOutput(), Tr::tr("File Error"));
            // FIXME: Was: file.errorString(), but we don't have a file anymore.
            if (errorString)
                *errorString = msg;
            else
                QMessageBox::critical(ICore::dialogParent(), Tr::tr("File Error"), msg);
            return OpenResult::ReadError;
        }

        if (size == 0) {
            QString msg = Tr::tr("The Binary Editor cannot open empty files.");
            if (errorString)
                *errorString = msg;
            else
                QMessageBox::critical(ICore::dialogParent(), Tr::tr("File Error"), msg);
            return OpenResult::CannotHandle;
        }

        if (size / 16 >= qint64(1) << 31) {
            // The limit is 2^31 lines (due to QText* interfaces) * 16 bytes per line.
            QString msg = Tr::tr("The file is too big for the Binary Editor (max. 32GB).");
            if (errorString)
                *errorString = msg;
            else
                QMessageBox::critical(ICore::dialogParent(), Tr::tr("File Error"), msg);
            return OpenResult::CannotHandle;
        }

        if (offset >= quint64(size))
            return OpenResult::CannotHandle;

        setFilePath(filePath);
        m_widget->setSizes(offset, size);
        return OpenResult::Success;
    }

    void provideData(quint64 address)
    {
        const FilePath fn = filePath();
        if (fn.isEmpty())
            return;
        const int blockSize = m_widget->dataBlockSize();
        QByteArray data = fn.fileContents(blockSize, address).value_or(QByteArray());
        const int dataSize = data.size();
        if (dataSize != blockSize)
            data += QByteArray(blockSize - dataSize, 0);
        m_widget->addData(address, data);
//       QMessageBox::critical(ICore::dialogParent(), Tr::tr("File Error"),
//                             Tr::tr("Cannot open %1: %2").arg(
//                                   fn.toUserOutput(), file.errorString()));
    }

    void provideNewRange(quint64 offset)
    {
        if (filePath().exists())
            openImpl(nullptr, filePath(), offset);
    }

public:
    bool isModified() const override
    {
        return isTemporary()/*e.g. memory view*/ ? false
                                                 : m_widget->isModified();
    }

    bool isSaveAsAllowed() const override { return true; }

    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override
    {
        Q_UNUSED(type)
        if (flag == FlagIgnore)
            return true;
        emit aboutToReload();
        int cPos = m_widget->cursorPosition();
        m_widget->clear();
        const bool success = (openImpl(errorString, filePath()) == OpenResult::Success);
        m_widget->setCursorPosition(cPos);
        emit reloadFinished(success);
        return success;
    }

protected:
    bool saveImpl(QString *errorString, const Utils::FilePath &filePath, bool autoSave) override
    {
        QTC_ASSERT(!autoSave, return true); // bineditor does not support autosave - it would be a bit expensive
        if (m_widget->save(errorString, this->filePath(), filePath)) {
            setFilePath(filePath);
            return true;
        }
        return false;
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
        using namespace TextEditor;
        setWidget(widget);
        m_file = new BinEditorDocument(widget);
        m_addressEdit = new QLineEdit;
        auto addressValidator = new QRegularExpressionValidator(QRegularExpression("[0-9a-fA-F]{1,16}"), m_addressEdit);
        m_addressEdit->setValidator(addressValidator);
        m_codecChooser = new CodecChooser(CodecChooser::Filter::SingleByte);
        m_codecChooser->prependNone();

        auto l = new QHBoxLayout;
        auto w = new QWidget;
        l->setContentsMargins(0, 0, 5, 0);
        l->addStretch(1);
        l->addWidget(m_codecChooser);
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
        connect(m_codecChooser, &CodecChooser::codecChanged,
                widget, &BinEditorWidget::setCodec);
        connect(widget, &BinEditorWidget::modificationChanged,
                m_file, &IDocument::changed);
        updateCursorPosition(widget->cursorPosition());
        const QVariant setting = ICore::settings()->value(Constants::C_ENCODING_SETTING);
        if (!setting.isNull())
            m_codecChooser->setAssignedCodec(QTextCodec::codecForName(setting.toByteArray()));
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
    TextEditor::CodecChooser *m_codecChooser;
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

    m_undoAction = new QAction(Tr::tr("&Undo"), this);
    m_redoAction = new QAction(Tr::tr("&Redo"), this);
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
    setDisplayName(::Core::Tr::tr("Binary Editor"));
    addMimeType(Constants::C_BINEDITOR_MIMETYPE);

    setEditorCreator([] {
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

void BinEditorPlugin::initialize()
{
    dd = new BinEditorPluginPrivate;
}

} // BinEditor::Internal

#include "bineditorplugin.moc"
