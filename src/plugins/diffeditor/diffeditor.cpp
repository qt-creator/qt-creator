// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diffeditor.h"
#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffeditoricons.h"
#include "diffeditortr.h"
#include "diffview.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>

#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/marginsettings.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyle>
#include <QTextBlock>
#include <QTextCodec>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

static const char settingsGroupC[] = "DiffEditor";
static const char descriptionVisibleKeyC[] = "DescriptionVisible";
static const char descriptionHeightKeyC[] = "DescriptionHeight";
static const char horizontalScrollBarSynchronizationKeyC[] = "HorizontalScrollBarSynchronization";
static const char contextLineCountKeyC[] = "ContextLineNumbers";
static const char ignoreWhitespaceKeyC[] = "IgnoreWhitespace";

static const char diffViewKeyC[] = "DiffEditorType";

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace DiffEditor::Internal {

class DescriptionEditorWidget : public TextEditorWidget
{
    Q_OBJECT
public:
    DescriptionEditorWidget(QWidget *parent = nullptr);

    QSize sizeHint() const override;

signals:
    void requestResize();

protected:
    void setDisplaySettings(const DisplaySettings &ds) override;
    void setMarginSettings(const MarginSettings &ms) override;
    void applyFontSettings() override;
};

DescriptionEditorWidget::DescriptionEditorWidget(QWidget *parent)
    : TextEditorWidget(parent)
{
    setupFallBackEditor("DiffEditor.DescriptionEditor");

    DisplaySettings settings = displaySettings();
    settings.m_textWrapping = false;
    settings.m_displayLineNumbers = false;
    settings.m_displayFoldingMarkers = false;
    settings.m_markTextChanges = false;
    settings.m_highlightBlocks = false;
    TextEditorWidget::setDisplaySettings(settings);

    setCodeFoldingSupported(true);
    setFrameStyle(QFrame::NoFrame);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto context = new IContext(this);
    context->setWidget(this);
    context->setContext(Context(Constants::C_DIFF_EDITOR_DESCRIPTION));
    ICore::addContextObject(context);

    textDocument()->setSyntaxHighlighter(new SyntaxHighlighter);
}

QSize DescriptionEditorWidget::sizeHint() const
{
    QSize size = TextEditorWidget::sizeHint();
    size.setHeight(size.height() / 5);
    return size;
}

void DescriptionEditorWidget::setDisplaySettings(const DisplaySettings &ds)
{
    DisplaySettings settings = displaySettings();
    settings.m_visualizeWhitespace = ds.m_visualizeWhitespace;
    settings.m_scrollBarHighlights = ds.m_scrollBarHighlights;
    settings.m_highlightCurrentLine = ds.m_highlightCurrentLine;
    TextEditorWidget::setDisplaySettings(settings);
}

void DescriptionEditorWidget::setMarginSettings(const MarginSettings &ms)
{
    Q_UNUSED(ms)
    TextEditorWidget::setMarginSettings({});
}

void DescriptionEditorWidget::applyFontSettings()
{
    TextEditorWidget::applyFontSettings();
    emit requestResize();
}

///////////////////////////////// DiffEditor //////////////////////////////////

DiffEditor::DiffEditor()
{
    // Editor:
    setDuplicateSupported(true);

    // Widget:
    QSplitter *splitter = new MiniSplitter(Qt::Vertical);

    connect(splitter, &QSplitter::splitterMoved, this, [this, splitter](int pos) {
        if (!m_showDescription)
            return;
        const int lineSpacing = splitter->widget(0)->fontMetrics().lineSpacing();
        const int descHeight = (pos + lineSpacing - 1) / lineSpacing; // round up
        if (m_descriptionHeight == descHeight)
            return;
        m_descriptionHeight = descHeight;
        saveSetting(descriptionHeightKeyC, descHeight);
    });
    m_descriptionWidget = new DescriptionEditorWidget(splitter);
    m_descriptionWidget->setReadOnly(true);
    connect(m_descriptionWidget, &DescriptionEditorWidget::requestResize, this, [this, splitter] {
        if (splitter->count() == 0)
            return;
        QList<int> sizes = splitter->sizes();
        const int descHeight = splitter->widget(0)->fontMetrics().lineSpacing() * m_descriptionHeight;
        const int diff = descHeight - sizes[0];
        if (diff > 0) {
            sizes[0] += diff;
            sizes[1] -= diff;
            splitter->setSizes(sizes);
        }
    });
    splitter->addWidget(m_descriptionWidget);

    m_stackedWidget = new QStackedWidget(splitter);
    splitter->addWidget(m_stackedWidget);

    m_unifiedView = new UnifiedView;
    m_sideBySideView = new SideBySideView;

    addView(m_sideBySideView);
    addView(m_unifiedView);

    setWidget(splitter);

    // Toolbar:
    m_toolBar = new QToolBar;
    m_toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    const int size = m_views.at(0)->widget()->style()->pixelMetric(QStyle::PM_SmallIconSize);
    m_toolBar->setIconSize(QSize(size, size));

    m_entriesComboBox = new QComboBox;
    m_entriesComboBox->setMinimumContentsLength(20);
    // Make the combo box prefer to expand
    QSizePolicy policy = m_entriesComboBox->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_entriesComboBox->setSizePolicy(policy);
    connect(m_entriesComboBox, &QComboBox::currentIndexChanged,
            this, &DiffEditor::currentIndexChanged);
    m_toolBar->addWidget(m_entriesComboBox);

    QLabel *contextLabel = new QLabel(m_toolBar);
    contextLabel->setText(Tr::tr("Context lines:"));
    contextLabel->setContentsMargins(6, 0, 6, 0);
    m_contextLabelAction = m_toolBar->addWidget(contextLabel);

    m_contextSpinBox = new QSpinBox(m_toolBar);
    m_contextSpinBox->setRange(1, 100);
    m_contextSpinBox->setFrame(false);
    m_contextSpinBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding); // Mac Qt5
    m_contextSpinBoxAction = m_toolBar->addWidget(m_contextSpinBox);

    m_whitespaceButtonAction = m_toolBar->addAction(Tr::tr("Ignore Whitespace"));
    m_whitespaceButtonAction->setCheckable(true);

    m_toggleDescriptionAction = m_toolBar->addAction(Icons::TOP_BAR.icon(), {});
    m_toggleDescriptionAction->setCheckable(true);

    m_reloadAction = m_toolBar->addAction(Utils::Icons::RELOAD_TOOLBAR.icon(), Tr::tr("Reload Diff"));
    m_reloadAction->setToolTip(Tr::tr("Reload Diff"));

    m_toggleSyncAction = m_toolBar->addAction(Utils::Icons::LINK_TOOLBAR.icon(), {});
    m_toggleSyncAction->setCheckable(true);

    m_viewSwitcherAction = m_toolBar->addAction(QIcon(), {});

    connect(m_whitespaceButtonAction, &QAction::toggled,
            this, &DiffEditor::ignoreWhitespaceHasChanged);
    connect(m_contextSpinBox, &QSpinBox::valueChanged,
            this, &DiffEditor::contextLineCountHasChanged);
    connect(m_toggleSyncAction, &QAction::toggled, this, &DiffEditor::toggleSync);
    connect(m_toggleDescriptionAction, &QAction::toggled, this, &DiffEditor::toggleDescription);
    connect(m_viewSwitcherAction, &QAction::triggered, this, [this] { showDiffView(nextView()); });
}

void DiffEditor::setDocument(QSharedPointer<DiffEditorDocument> doc)
{
    QTC_ASSERT(m_document.isNull(), return);
    QTC_ASSERT(doc, return);

    m_document = doc;

    connect(m_document.data(), &DiffEditorDocument::documentChanged,
            this, &DiffEditor::documentHasChanged);
    connect(m_document.data(), &DiffEditorDocument::descriptionChanged,
            this, &DiffEditor::updateDescription);
    connect(m_document.data(), &DiffEditorDocument::aboutToReload,
            this, &DiffEditor::prepareForReload);
    connect(m_document.data(), &DiffEditorDocument::reloadFinished,
            this, &DiffEditor::reloadHasFinished);

    connect(m_reloadAction, &QAction::triggered, this, [this] { m_document->reload(); });
    connect(m_document.data(), &DiffEditorDocument::temporaryStateChanged,
            this, &DiffEditor::documentStateChanged);

    m_contextSpinBox->setValue(m_document->contextLineCount());
    m_whitespaceButtonAction->setChecked(m_document->ignoreWhitespace());

    documentStateChanged();
    documentHasChanged();
}

DiffEditor::DiffEditor(DiffEditorDocument *doc) : DiffEditor()
{
    GuardLocker guard(m_ignoreChanges);
    setDocument(QSharedPointer<DiffEditorDocument>(doc));
    setupView(loadSettings());
}

DiffEditor::~DiffEditor()
{
    delete m_toolBar;
    delete m_widget;
    qDeleteAll(m_views);
}

IEditor *DiffEditor::duplicate()
{
    auto editor = new DiffEditor;
    GuardLocker guard(editor->m_ignoreChanges);

    editor->setDocument(m_document);
    editor->m_sync = m_sync;
    editor->m_showDescription = m_showDescription;

    Id id = currentView()->id();
    IDiffView *view = Utils::findOr(editor->m_views, editor->m_views.at(0),
                                    Utils::equal(&IDiffView::id, id));
    QTC_ASSERT(view, view = editor->currentView());
    editor->setupView(view);

    emit editorDuplicated(editor);

    return editor;
}

IDocument *DiffEditor::document() const
{
    return m_document.data();
}

QWidget *DiffEditor::toolBar()
{
    QTC_ASSERT(m_toolBar, return nullptr);
    return m_toolBar;
}

TextEditorWidget *DiffEditor::descriptionWidget() const
{
    return m_descriptionWidget;
}

TextEditorWidget *DiffEditor::unifiedEditorWidget() const
{
    return m_unifiedView->textEditorWidget();
}

TextEditorWidget *DiffEditor::sideEditorWidget(DiffSide side) const
{
    return m_sideBySideView->sideEditorWidget(side);
}

void DiffEditor::documentHasChanged()
{
    GuardLocker guard(m_ignoreChanges);
    const QList<FileData> &diffFileList = m_document->diffFiles();

    updateDescription();
    currentView()->setDiff(diffFileList);

    m_entriesComboBox->clear();
    const QString startupFile = m_document->startupFile();
    int startupFileIndex = -1;
    for (int i = 0, total = diffFileList.count(); i < total; ++i) {
        const FileData &diffFile = diffFileList.at(i);
        const DiffFileInfo &leftEntry = diffFile.fileInfo[LeftSide];
        const DiffFileInfo &rightEntry = diffFile.fileInfo[RightSide];
        const QString leftShortFileName = FilePath::fromString(leftEntry.fileName).fileName();
        const QString rightShortFileName = FilePath::fromString(rightEntry.fileName).fileName();
        QString itemText;
        QString itemToolTip;
        if (leftEntry.fileName == rightEntry.fileName) {
            itemText = leftShortFileName;

            if (leftEntry.typeInfo.isEmpty() && rightEntry.typeInfo.isEmpty()) {
                itemToolTip = leftEntry.fileName;
            } else {
                itemToolTip = Tr::tr("[%1] vs. [%2] %3").arg(
                    leftEntry.typeInfo, rightEntry.typeInfo, leftEntry.fileName);
            }
        } else {
            if (leftShortFileName == rightShortFileName)
                itemText = leftShortFileName;
            else
                itemText = Tr::tr("%1 vs. %2").arg(leftShortFileName, rightShortFileName);

            if (leftEntry.typeInfo.isEmpty() && rightEntry.typeInfo.isEmpty()) {
                itemToolTip = Tr::tr("%1 vs. %2").arg(leftEntry.fileName, rightEntry.fileName);
            } else {
                itemToolTip = Tr::tr("[%1] %2 vs. [%3] %4").arg(leftEntry.typeInfo,
                    leftEntry.fileName, rightEntry.typeInfo, rightEntry.fileName);
            }
        }
        m_entriesComboBox->addItem(itemText);
        m_entriesComboBox->setItemData(m_entriesComboBox->count() - 1,
                                       leftEntry.fileName, Qt::UserRole);
        m_entriesComboBox->setItemData(m_entriesComboBox->count() - 1,
                                       rightEntry.fileName, Qt::UserRole + 1);
        m_entriesComboBox->setItemData(m_entriesComboBox->count() - 1,
                                       itemToolTip, Qt::ToolTipRole);
        if (startupFileIndex < 0) {
            const bool isStartup = m_currentFileChunk.first.isEmpty()
                                   && m_currentFileChunk.second.isEmpty()
                                   && startupFile.endsWith(rightEntry.fileName);
            const bool isSame = m_currentFileChunk.first == leftEntry.fileName
                                && m_currentFileChunk.second == rightEntry.fileName;
            if (isStartup || isSame)
                startupFileIndex = i;
        }
    }

    currentView()->endOperation();
    m_currentFileChunk = {};
    if (startupFileIndex >= 0)
        setCurrentDiffFileIndex(startupFileIndex);
}

void DiffEditor::toggleDescription()
{
    if (m_ignoreChanges.isLocked())
        return;

    m_showDescription = !m_showDescription;
    saveSetting(descriptionVisibleKeyC, m_showDescription);
    updateDescription();
}

void DiffEditor::updateDescription()
{
    QTC_ASSERT(m_toolBar, return);

    const QString description = m_document->description();
    m_descriptionWidget->setPlainText(description);
    m_descriptionWidget->setVisible(m_showDescription && !description.isEmpty());

    const QString actionText = m_showDescription ? Tr::tr("Hide Change Description")
                                                 : Tr::tr("Show Change Description");
    GuardLocker guard(m_ignoreChanges);
    m_toggleDescriptionAction->setChecked(m_showDescription);
    m_toggleDescriptionAction->setToolTip(actionText);
    m_toggleDescriptionAction->setText(actionText);
    m_toggleDescriptionAction->setVisible(!description.isEmpty());
}

void DiffEditor::contextLineCountHasChanged(int lines)
{
    QTC_ASSERT(!m_document->isContextLineCountForced(), return);
    if (m_ignoreChanges.isLocked() || lines == m_document->contextLineCount())
        return;

    m_document->setContextLineCount(lines);
    saveSetting(contextLineCountKeyC, lines);

    m_document->reload();
}

void DiffEditor::ignoreWhitespaceHasChanged()
{
    const bool ignore = m_whitespaceButtonAction->isChecked();

    if (m_ignoreChanges.isLocked() || ignore == m_document->ignoreWhitespace())
        return;
    m_document->setIgnoreWhitespace(ignore);
    saveSetting(ignoreWhitespaceKeyC, ignore);

    m_document->reload();
}

void DiffEditor::prepareForReload()
{
    documentStateChanged(); // To update actions...

    QTC_ASSERT(currentView(), return);

    if (m_entriesComboBox->count() > 0) {
        m_currentFileChunk
              = {m_entriesComboBox->itemData(m_currentDiffFileIndex, Qt::UserRole).toString(),
                 m_entriesComboBox->itemData(m_currentDiffFileIndex, Qt::UserRole + 1).toString()};
    } else {
        m_currentFileChunk = {};
    }

    {
        GuardLocker guard(m_ignoreChanges);
        m_contextSpinBox->setValue(m_document->contextLineCount());
        m_whitespaceButtonAction->setChecked(m_document->ignoreWhitespace());
    }
    currentView()->beginOperation();
    currentView()->setMessage(Tr::tr("Waiting for data..."));
}

void DiffEditor::reloadHasFinished(bool success)
{
    if (!currentView())
        return;

    if (!success)
        currentView()->setMessage(Tr::tr("Retrieving data failed."));
}

void DiffEditor::updateEntryToolTip()
{
    const QString &toolTip = m_entriesComboBox->itemData(
                m_entriesComboBox->currentIndex(), Qt::ToolTipRole).toString();
    m_entriesComboBox->setToolTip(toolTip);
}

void DiffEditor::currentIndexChanged(int index)
{
    if (m_ignoreChanges.isLocked())
        return;

    GuardLocker guard(m_ignoreChanges);
    setCurrentDiffFileIndex(index);
}

void DiffEditor::setCurrentDiffFileIndex(int index)
{
    QTC_ASSERT((index < 0) != (m_entriesComboBox->count() > 0), return);

    m_currentDiffFileIndex = index;
    currentView()->setCurrentDiffFileIndex(index);

    m_entriesComboBox->setCurrentIndex(m_entriesComboBox->count() > 0 ? qMax(0, index) : -1);
    updateEntryToolTip();
}

void DiffEditor::documentStateChanged()
{
    const bool canReload = m_document->isTemporary();
    const bool contextVisible = !m_document->isContextLineCountForced();

    m_whitespaceButtonAction->setVisible(canReload);
    m_contextLabelAction->setVisible(canReload && contextVisible);
    m_contextSpinBoxAction->setVisible(canReload && contextVisible);
    m_reloadAction->setVisible(canReload);
}

void DiffEditor::updateDiffEditorSwitcher()
{
    if (!m_viewSwitcherAction)
        return;
    IDiffView *next = nextView();
    m_viewSwitcherAction->setIcon(next->icon());
    m_viewSwitcherAction->setToolTip(next->toolTip());
    m_viewSwitcherAction->setText(next->toolTip());
}

void DiffEditor::toggleSync()
{
    if (m_ignoreChanges.isLocked())
        return;

    QTC_ASSERT(currentView(), return);
    m_sync = !m_sync;
    saveSetting(horizontalScrollBarSynchronizationKeyC, m_sync);
    currentView()->setSync(m_sync);
}

IDiffView *DiffEditor::loadSettings()
{
    QTC_ASSERT(currentView(), return nullptr);
    QtcSettings *s = ICore::settings();

    // Read current settings:
    s->beginGroup(settingsGroupC);
    m_showDescription = s->value(descriptionVisibleKeyC, true).toBool();
    m_descriptionHeight = s->value(descriptionHeightKeyC, 8).toInt();
    m_sync = s->value(horizontalScrollBarSynchronizationKeyC, true).toBool();
    m_document->setIgnoreWhitespace(s->value(ignoreWhitespaceKeyC, false).toBool());
    m_document->setContextLineCount(s->value(contextLineCountKeyC, 3).toInt());
    Id id = Id::fromSetting(s->value(diffViewKeyC));
    s->endGroup();

    IDiffView *view = Utils::findOr(m_views, m_views.at(0),
                                    Utils::equal(&IDiffView::id, id));
    QTC_CHECK(view);

    return view;
}

void DiffEditor::saveSetting(const Key &key, const QVariant &value) const
{
    QtcSettings *s = ICore::settings();
    s->beginGroup(settingsGroupC);
    s->setValue(key, value);
    s->endGroup();
}

void DiffEditor::addView(IDiffView *view)
{
    QTC_ASSERT(!m_views.contains(view), return);
    m_views.append(view);
    m_stackedWidget->addWidget(view->widget());
    if (m_views.count() == 1)
        setCurrentView(view);

    connect(view, &IDiffView::currentDiffFileIndexChanged, this, &DiffEditor::currentIndexChanged);
}

IDiffView *DiffEditor::currentView() const
{
    if (m_currentViewIndex < 0)
        return nullptr;
    return m_views.at(m_currentViewIndex);
}

void DiffEditor::setCurrentView(IDiffView *view)
{
    const int pos = Utils::indexOf(m_views, [view](IDiffView *v) { return v == view; });
    QTC_ASSERT(pos >= 0 && pos < m_views.count(), return);
    m_currentViewIndex = pos;
}

IDiffView *DiffEditor::nextView()
{
    int pos = m_currentViewIndex + 1;
    if (pos >= m_views.count())
        pos = 0;

    return m_views.at(pos);
}

void DiffEditor::setupView(IDiffView *view)
{
    QTC_ASSERT(view, return);
    setCurrentView(view);

    saveSetting(diffViewKeyC, currentView()->id().toSetting());

    {
        GuardLocker guard(m_ignoreChanges);
        m_toggleSyncAction->setVisible(currentView()->supportsSync());
        m_toggleSyncAction->setToolTip(currentView()->syncToolTip());
        m_toggleSyncAction->setText(currentView()->syncToolTip());
        m_toggleSyncAction->setChecked(m_sync);
    }

    view->setDocument(m_document.data());
    view->setSync(m_sync);
    view->setCurrentDiffFileIndex(m_currentDiffFileIndex);

    m_stackedWidget->setCurrentWidget(view->widget());

    updateDiffEditorSwitcher();
    if (widget())
        widget()->setFocusProxy(view->widget());
}

void DiffEditor::showDiffView(IDiffView *view)
{
    if (currentView() == view)
        return;

    if (currentView()) // during initialization
        currentView()->setDocument(nullptr);

    QTC_ASSERT(view, return);
    setupView(view);
}

} // namespace DiffEditor::Internal

#include "diffeditor.moc"
