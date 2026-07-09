// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codestyleeditor.h"

#include "codestyleselectorwidget.h"
#include "codestylepool.h"
#include "displaysettings.h"
#include "icodestylepreferences.h"
#include "icodestylepreferencesfactory.h"
#include "indenter.h"
#include "snippets/snippeteditor.h"
#include "snippets/snippetprovider.h"
#include "textdocument.h"
#include "texteditortr.h"

#include <coreplugin/icore.h>
#include <utils/filepath.h>
#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>

#include <QChar>
#include <QFont>
#include <QLabel>
#include <QTextBlock>
#include <QVBoxLayout>

using namespace Utils;

namespace TextEditor {

void CodeStyleEditor::apply() {}

void CodeStyleEditor::cancel() {}

bool CodeStyleEditor::isDirty() const
{
    return false;
}

// The default per-project code style editor: a style selector above a live
// preview. Changes apply immediately to the project's code style, so there is
// no apply/cancel here.
class CodeStyleProjectPreviewEditor final : public QWidget
{
public:
    CodeStyleProjectPreviewEditor(
        const ICodeStylePreferencesFactory *factory,
        const FilePath &projectFile,
        ICodeStylePreferences *codeStyle)
        : m_selector{projectFile, this}
    {
        m_selector.setCodeStyle(codeStyle);

        using namespace Layouting;
        Column {
            &m_selector,
            createTakeEffectImmediatelyLabel(),
            createCodeStylePreview(factory, projectFile, codeStyle),
            createCodeStylePreviewNote(),
            noMargin,
        }.attachTo(this);
    }

private:
    CodeStyleSelectorWidget m_selector;
};

QWidget *ICodeStylePreferencesFactory::createProjectEditor(
    const FilePath &projectFile, ICodeStylePreferences *codeStyle) const
{
    if (m_projectEditorCreator)
        return m_projectEditorCreator(projectFile, codeStyle);
    return new CodeStyleProjectPreviewEditor{this, projectFile, codeStyle};
}

SnippetEditorWidget *createCodeStylePreview(const ICodeStylePreferencesFactory *factory,
                                            const FilePath &projectFile,
                                            ICodeStylePreferences *codeStyle,
                                            QWidget *parent)
{
    auto preview = new SnippetEditorWidget(parent);
    DisplaySettingsData displaySettings = preview->displaySettings();
    displaySettings.m_visualizeWhitespace = true;
    preview->setDisplaySettings(displaySettings);
    SnippetProvider::decorateEditor(preview, factory->snippetGroupId());
    preview->setPlainText(factory->previewText());

    Indenter *indenter = factory->createIndenter(preview->document());
    indenter->setOverriddenPreferences(codeStyle);
    const FilePath fileName = !projectFile.isEmpty()
        ? projectFile.pathAppended("snippet.cpp")
        : Core::ICore::userResourcePath("snippet.cpp");
    indenter->setFileName(fileName);
    preview->textDocument()->setIndenter(indenter);

    const auto updatePreview = [preview, codeStyle] {
        QTextDocument *doc = preview->document();
        preview->textDocument()->indenter()->invalidateCache();
        QTextBlock block = doc->firstBlock();
        QTextCursor tc = preview->textCursor();
        tc.beginEditBlock();
        while (block.isValid()) {
            preview->textDocument()
                ->indenter()
                ->indentBlock(block, QChar::Null, codeStyle->currentTabSettings());
            block = block.next();
        }
        tc.endEditBlock();
    };

    QObject::connect(codeStyle, &ICodeStylePreferences::currentTabSettingsChanged,
                     preview, updatePreview);
    QObject::connect(codeStyle, &ICodeStylePreferences::currentValueChanged,
                     preview, updatePreview);
    QObject::connect(codeStyle, &ICodeStylePreferences::currentPreferencesChanged,
                     preview, updatePreview);
    updatePreview();

    return preview;
}

QLabel *createCodeStylePreviewNote()
{
    auto label = new QLabel(
        Tr::tr("Edit preview contents to see how the current settings "
               "are applied to custom code snippets. Changes in the preview "
               "do not affect the current settings."));
    QFont font = label->font();
    font.setItalic(true);
    label->setFont(font);
    label->setWordWrap(true);
    return label;
}

QWidget *createTakeEffectImmediatelyLabel()
{
    auto infoLabel = new InfoLabel(Tr::tr("All changes below take effect immediately."),
                                   InfoLabelType::Information);
    infoLabel->setFilled(true);
    // Wrap in a plain container: InfoLabel uses its own contentsMargins to place
    // its icon, so callers indent the container instead of the label itself.
    auto container = new QWidget;
    auto layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(infoLabel);
    return container;
}

CodeStyleAspect::CodeStyleAspect(ICodeStylePreferences *codeStyle, Id languageId)
    : m_codeStyle(codeStyle)
    , m_languageId(languageId)
{
    setLayouter([this] {
        ICodeStylePreferencesFactory *factory = codeStyleFactory(m_languageId);
        ensurePageCopy(factory);
        syncFromReal();

        using namespace Layouting;

        QWidget *valueEditor = factory->createValueEditor(m_pageCodeStyle);

        // A self-managed editor lays out its own selector and manages its own
        // deferred apply/cancel (e.g. ClangFormat, whose settings live outside
        // the preferences). Route its contract and show it as it is.
        if (auto selfManaged = qobject_cast<CodeStyleEditor *>(valueEditor)) {
            m_editor = selfManaged;
            connect(m_editor, &CodeStyleEditor::changed,
                    this, [this] { emit volatileValueChanged(); });
            return Column { valueEditor };
        }

        // A plain value editor edits the page-local copy live; build the common
        // selector and preview around it and let this aspect own the deferral.
        auto selector = new CodeStyleSelectorWidget({});
        selector->setCodeStyle(m_pageCodeStyle);
        Utils::installMarkSettingsDirtyTriggerRecursively(selector);

        if (factory->valueEditorHasPreview())
            return Column { selector, valueEditor };
        return Column {
            selector,
            Row {
                Column { valueEditor, st },
                Column {
                    createCodeStylePreview(factory, {}, m_pageCodeStyle),
                    createCodeStylePreviewNote(),
                },
            },
        };
    });
}

void CodeStyleAspect::ensurePageCopy(ICodeStylePreferencesFactory *factory)
{
    if (m_pagePool)
        return;

    // A transient, page-local pool holding editable copies of the real pool's
    // styles, so that selecting, editing, adding and removing styles on the
    // page is all deferred until apply().
    m_pagePool = new CodeStylePool(factory);
    m_pagePool->setTransient(true);

    m_pageCodeStyle = factory->createCodeStyle();
    m_pageCodeStyle->setDelegatingPool(m_pagePool);
    // Share the real style's id so it cannot be picked as its own delegate.
    m_pageCodeStyle->setId(m_codeStyle->id());

    // Any live edit through the value editor or the selector lands in the page
    // copy; reflect it in this aspect's dirtiness. Guarded so the mirroring
    // done by syncFromReal() does not count as a user edit.
    const auto notify = [this] {
        if (!m_syncing)
            emit volatileValueChanged();
    };
    connect(m_pageCodeStyle, &ICodeStylePreferences::currentDelegateChanged, this, notify);
    connect(m_pageCodeStyle, &ICodeStylePreferences::currentValueChanged, this, notify);
    connect(m_pageCodeStyle, &ICodeStylePreferences::currentTabSettingsChanged, this, notify);
    connect(m_pageCodeStyle, &ICodeStylePreferences::currentPreferencesChanged, this, notify);
}

CodeStyleAspect::~CodeStyleAspect()
{
    delete m_editor;
    delete m_pageCodeStyle;
    delete m_pagePool;
}

ICodeStylePreferences *CodeStyleAspect::addPageCopy(ICodeStylePreferences *realStyle)
{
    ICodeStylePreferences *copy = codeStyleFactory(m_languageId)->createCodeStyle();
    copy->setId(realStyle->id());
    // Set read-only before adding so the pool files it as built-in vs. custom.
    copy->setReadOnly(realStyle->isReadOnly());
    copy->setDisplayName(realStyle->displayName());
    copy->setValue(realStyle->value());
    copy->setTabSettings(realStyle->tabSettings());
    m_pagePool->addCodeStyle(copy);
    // addCodeStyle() does not take ownership, so parent the copy to the pool.
    copy->setParent(m_pagePool);
    return copy;
}

void CodeStyleAspect::apply()
{
    // Nothing to commit until the page has been shown at least once.
    if (!m_pageCodeStyle)
        return;

    // Flush the hosted editor's own pending edits.
    if (m_editor)
        m_editor->apply();

    CodeStylePool *realPool = m_codeStyle->delegatingPool();
    const QByteArray selfId = m_codeStyle->id();
    bool changed = false;

    if (realPool) {
        // Remove the real custom styles that were deleted on the page.
        const QList<ICodeStylePreferences *> realCustom = realPool->customCodeStyles();
        for (ICodeStylePreferences *realStyle : realCustom) {
            if (realStyle->id() != selfId && !m_pagePool->codeStyle(realStyle->id())) {
                realPool->removeCodeStyle(realStyle);
                changed = true;
            }
        }
        // Add styles created on the page and write back edited ones. Built-in
        // styles are read-only and never change.
        const QList<ICodeStylePreferences *> pageStyles = m_pagePool->codeStyles();
        for (ICodeStylePreferences *pageStyle : pageStyles) {
            if (pageStyle->isReadOnly())
                continue;
            ICodeStylePreferences *realStyle = realPool->codeStyle(pageStyle->id());
            if (!realStyle) {
                realPool->createCodeStyle(pageStyle->id(), pageStyle->tabSettings(),
                                          pageStyle->value(), pageStyle->displayName());
                changed = true;
                continue;
            }
            if (realStyle->value() != pageStyle->value()) {
                realStyle->setValue(pageStyle->value());
                changed = true;
            }
            if (realStyle->tabSettings() != pageStyle->tabSettings()) {
                realStyle->setTabSettings(pageStyle->tabSettings());
                changed = true;
            }
            if (realStyle->displayName() != pageStyle->displayName()) {
                realStyle->setDisplayName(pageStyle->displayName());
                changed = true;
            }
        }
    }

    // The global style's own settings and selected delegate.
    if (m_codeStyle->value() != m_pageCodeStyle->value()) {
        m_codeStyle->setValue(m_pageCodeStyle->value());
        changed = true;
    }
    if (m_codeStyle->tabSettings() != m_pageCodeStyle->tabSettings()) {
        m_codeStyle->setTabSettings(m_pageCodeStyle->tabSettings());
        changed = true;
    }
    ICodeStylePreferences *pageDelegate = m_pageCodeStyle->currentDelegate();
    ICodeStylePreferences *realDelegate =
        (pageDelegate && realPool) ? realPool->codeStyle(pageDelegate->id()) : nullptr;
    if (m_codeStyle->currentDelegate() != realDelegate) {
        m_codeStyle->setCurrentDelegate(realDelegate);
        changed = true;
    }

    if (changed)
        m_codeStyle->toSettings(m_languageId.toKey());

    // Re-mirror so the page reflects the persisted state (e.g. ids assigned to
    // newly created styles) and dirtiness clears.
    syncFromReal();
    emit volatileValueChanged();
}

void CodeStyleAspect::cancel()
{
    if (!m_pageCodeStyle)
        return;
    if (m_editor)
        m_editor->cancel();
    syncFromReal();
}

bool CodeStyleAspect::isDirty() const
{
    // Guard on the page copy: until the page is shown it is not yet synced.
    if (!m_pageCodeStyle)
        return false;
    if (m_editor && m_editor->isDirty())
        return true;
    if (m_codeStyle->value() != m_pageCodeStyle->value()
        || m_codeStyle->tabSettings() != m_pageCodeStyle->tabSettings()) {
        return true;
    }
    const ICodeStylePreferences *realDelegate = m_codeStyle->currentDelegate();
    const ICodeStylePreferences *pageDelegate = m_pageCodeStyle->currentDelegate();
    const QByteArray realDelegateId = realDelegate ? realDelegate->id() : QByteArray();
    const QByteArray pageDelegateId = pageDelegate ? pageDelegate->id() : QByteArray();
    if (realDelegateId != pageDelegateId)
        return true;
    return poolsDiffer();
}

bool CodeStyleAspect::poolsDiffer() const
{
    CodeStylePool *realPool = m_codeStyle->delegatingPool();
    if (!realPool)
        return false;

    // A page style missing from, or differing from, the real pool?
    const QList<ICodeStylePreferences *> pageStyles = m_pagePool->codeStyles();
    for (ICodeStylePreferences *pageStyle : pageStyles) {
        ICodeStylePreferences *realStyle = realPool->codeStyle(pageStyle->id());
        if (!realStyle)
            return true;
        if (realStyle->value() != pageStyle->value()
            || realStyle->tabSettings() != pageStyle->tabSettings()
            || realStyle->displayName() != pageStyle->displayName()) {
            return true;
        }
    }
    // A real (delegatable) style removed on the page?
    const QByteArray selfId = m_codeStyle->id();
    const QList<ICodeStylePreferences *> realStyles = realPool->codeStyles();
    for (ICodeStylePreferences *realStyle : realStyles) {
        if (realStyle->id() != selfId && !m_pagePool->codeStyle(realStyle->id()))
            return true;
    }
    return false;
}

void CodeStyleAspect::syncFromReal()
{
    m_syncing = true;
    CodeStylePool *realPool = m_codeStyle->delegatingPool();
    const QByteArray selfId = m_codeStyle->id();

    // Drop page styles whose real counterpart is gone.
    const QList<ICodeStylePreferences *> pageStyles = m_pagePool->codeStyles();
    for (ICodeStylePreferences *pageStyle : pageStyles) {
        if (!realPool || !realPool->codeStyle(pageStyle->id()))
            m_pagePool->removeCodeStyle(pageStyle);
    }
    // Add or refresh a page copy for every delegatable real style. The real
    // global is represented by m_pageCodeStyle, so it is skipped.
    if (realPool) {
        const QList<ICodeStylePreferences *> realStyles = realPool->codeStyles();
        for (ICodeStylePreferences *realStyle : realStyles) {
            if (realStyle->id() == selfId)
                continue;
            ICodeStylePreferences *pageStyle = m_pagePool->codeStyle(realStyle->id());
            if (!pageStyle)
                pageStyle = addPageCopy(realStyle);
            pageStyle->setValue(realStyle->value());
            pageStyle->setTabSettings(realStyle->tabSettings());
            pageStyle->setDisplayName(realStyle->displayName());
        }
    }

    // Sync the page global's own settings and its selected delegate.
    m_pageCodeStyle->setValue(m_codeStyle->value());
    m_pageCodeStyle->setTabSettings(m_codeStyle->tabSettings());
    ICodeStylePreferences *realDelegate = m_codeStyle->currentDelegate();
    m_pageCodeStyle->setCurrentDelegate(
        realDelegate ? m_pagePool->codeStyle(realDelegate->id()) : nullptr);

    m_syncing = false;
}

} // TextEditor
