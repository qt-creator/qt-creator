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

#include "diffeditor.h"
#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffeditoricons.h"
#include "diffview.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>

#include <texteditor/displaysettings.h>
#include <texteditor/marginsettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyle>
#include <QTextBlock>
#include <QTextCodec>
#include <QToolBar>
#include <QToolButton>

static const char settingsGroupC[] = "DiffEditor";
static const char descriptionVisibleKeyC[] = "DescriptionVisible";
static const char horizontalScrollBarSynchronizationKeyC[] = "HorizontalScrollBarSynchronization";
static const char contextLineCountKeyC[] = "ContextLineNumbers";
static const char ignoreWhitespaceKeyC[] = "IgnoreWhitespace";

static const char diffViewKeyC[] = "DiffEditorType";

using namespace TextEditor;

namespace DiffEditor {
namespace Internal {

class DescriptionEditorWidget : public TextEditorWidget
{
    Q_OBJECT
public:
    DescriptionEditorWidget(QWidget *parent = 0);
    virtual QSize sizeHint() const override;

signals:
    void requestBranchList();

protected:
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

    void setDisplaySettings(const DisplaySettings &ds) override;
    void setMarginSettings(const MarginSettings &ms) override;

    bool findContentsUnderCursor(const QTextCursor &cursor);
    void highlightCurrentContents();
    void handleCurrentContents();

private:
    QTextCursor m_currentCursor;
};

DescriptionEditorWidget::DescriptionEditorWidget(QWidget *parent)
    : TextEditorWidget(parent)
{
    setupFallBackEditor("DiffEditor.DescriptionEditor");

    DisplaySettings settings = displaySettings();
    settings.m_textWrapping = false;
    settings.m_displayLineNumbers = false;
    settings.m_highlightCurrentLine = false;
    settings.m_displayFoldingMarkers = false;
    settings.m_markTextChanges = false;
    settings.m_highlightBlocks = false;
    TextEditorWidget::setDisplaySettings(settings);

    setCodeFoldingSupported(true);
    setFrameStyle(QFrame::NoFrame);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
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
    TextEditorWidget::setDisplaySettings(settings);
}

void DescriptionEditorWidget::setMarginSettings(const MarginSettings &ms)
{
    Q_UNUSED(ms);
    TextEditorWidget::setMarginSettings(MarginSettings());
}

void DescriptionEditorWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons()) {
        TextEditorWidget::mouseMoveEvent(e);
        return;
    }

    Qt::CursorShape cursorShape;

    const QTextCursor cursor = cursorForPosition(e->pos());
    if (findContentsUnderCursor(cursor)) {
        highlightCurrentContents();
        cursorShape = Qt::PointingHandCursor;
    } else {
        setExtraSelections(OtherSelection, QList<QTextEdit::ExtraSelection>());
        cursorShape = Qt::IBeamCursor;
    }

    TextEditorWidget::mouseMoveEvent(e);
    viewport()->setCursor(cursorShape);
}

void DescriptionEditorWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && !(e->modifiers() & Qt::ShiftModifier)) {
        const QTextCursor cursor = cursorForPosition(e->pos());
        if (findContentsUnderCursor(cursor)) {
            handleCurrentContents();
            e->accept();
            return;
        }
    }

    TextEditorWidget::mouseReleaseEvent(e);
}

bool DescriptionEditorWidget::findContentsUnderCursor(const QTextCursor &cursor)
{
    m_currentCursor = cursor;
    return cursor.block().text() == QLatin1String(Constants::EXPAND_BRANCHES);
}

void DescriptionEditorWidget::highlightCurrentContents()
{
    QTextEdit::ExtraSelection sel;
    sel.cursor = m_currentCursor;
    sel.cursor.select(QTextCursor::LineUnderCursor);
    sel.format.setFontUnderline(true);
    setExtraSelections(TextEditorWidget::OtherSelection,
                       QList<QTextEdit::ExtraSelection>() << sel);
}

void DescriptionEditorWidget::handleCurrentContents()
{
    m_currentCursor.select(QTextCursor::LineUnderCursor);
    m_currentCursor.removeSelectedText();
    m_currentCursor.insertText(QLatin1String("Branches: Expanding..."));
    emit requestBranchList();
}

///////////////////////////////// DiffEditor //////////////////////////////////

DiffEditor::DiffEditor()
    : m_document(0)
    , m_descriptionWidget(0)
    , m_stackedWidget(0)
    , m_toolBar(0)
    , m_entriesComboBox(0)
    , m_contextSpinBox(0)
    , m_toggleSyncAction(0)
    , m_whitespaceButtonAction(0)
    , m_toggleDescriptionAction(0)
    , m_reloadAction(0)
    , m_viewSwitcherAction(0)
    , m_currentViewIndex(-1)
    , m_currentDiffFileIndex(-1)
    , m_sync(false)
    , m_showDescription(true)
{
    // Editor:
    setDuplicateSupported(true);

    // Widget:
    QSplitter *splitter = new Core::MiniSplitter(Qt::Vertical);

    m_descriptionWidget = new DescriptionEditorWidget(splitter);
    m_descriptionWidget->setReadOnly(true);
    splitter->addWidget(m_descriptionWidget);

    m_stackedWidget = new QStackedWidget(splitter);
    splitter->addWidget(m_stackedWidget);

    addView(new SideBySideView);
    addView(new UnifiedView);

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
    connect(m_entriesComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &DiffEditor::setCurrentDiffFileIndex);
    m_toolBar->addWidget(m_entriesComboBox);

    QLabel *contextLabel = new QLabel(m_toolBar);
    contextLabel->setText(tr("Context lines:"));
    contextLabel->setContentsMargins(6, 0, 6, 0);
    m_contextLabelAction = m_toolBar->addWidget(contextLabel);

    m_contextSpinBox = new QSpinBox(m_toolBar);
    m_contextSpinBox->setRange(1, 100);
    m_contextSpinBox->setFrame(false);
    m_contextSpinBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding); // Mac Qt5
    m_contextSpinBoxAction = m_toolBar->addWidget(m_contextSpinBox);

    m_whitespaceButtonAction = m_toolBar->addAction(tr("Ignore Whitespace"));
    m_whitespaceButtonAction->setCheckable(true);

    m_toggleDescriptionAction = m_toolBar->addAction(Icons::TOP_BAR.icon(), QString());
    m_toggleDescriptionAction->setCheckable(true);

    m_reloadAction = m_toolBar->addAction(Utils::Icons::RELOAD.icon(), tr("Reload Diff"));
    m_reloadAction->setToolTip(tr("Reload Diff"));

    m_toggleSyncAction = m_toolBar->addAction(Utils::Icons::LINK.icon(), QString());
    m_toggleSyncAction->setCheckable(true);

    m_viewSwitcherAction = m_toolBar->addAction(QIcon(), QString());

    connect(m_whitespaceButtonAction, &QAction::toggled, this, &DiffEditor::ignoreWhitespaceHasChanged);
    connect(m_contextSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &DiffEditor::contextLineCountHasChanged);
    connect(m_toggleSyncAction, &QAction::toggled, this, &DiffEditor::toggleSync);
    connect(m_toggleDescriptionAction, &QAction::toggled, this, &DiffEditor::toggleDescription);
    connect(m_viewSwitcherAction, &QAction::triggered, this, [this]() { showDiffView(nextView()); });
}

void DiffEditor::setDocument(QSharedPointer<DiffEditorDocument>(doc))
{
    QTC_ASSERT(m_document.isNull(), return);
    QTC_ASSERT(doc, return);

    m_document = QSharedPointer<DiffEditorDocument>(doc);

    connect(m_descriptionWidget, &DescriptionEditorWidget::requestBranchList,
            m_document.data(), &DiffEditorDocument::requestMoreInformation);
    connect(m_document.data(), &DiffEditorDocument::documentChanged,
            this, &DiffEditor::documentHasChanged);
    connect(m_document.data(), &DiffEditorDocument::descriptionChanged,
            this, &DiffEditor::updateDescription);
    connect(m_document.data(), &DiffEditorDocument::aboutToReload,
            this, &DiffEditor::prepareForReload);
    connect(m_document.data(), &DiffEditorDocument::reloadFinished,
            this, &DiffEditor::reloadHasFinished);

    connect(m_reloadAction, &QAction::triggered, this, [this]() { m_document->reload(); });
    connect(m_document.data(), &DiffEditorDocument::temporaryStateChanged,
            this, &DiffEditor::documentStateChanged);

    m_contextSpinBox->setValue(m_document->contextLineCount());
    m_whitespaceButtonAction->setChecked(m_document->ignoreWhitespace());

    documentStateChanged();
    documentHasChanged();
}

DiffEditor::DiffEditor(DiffEditorDocument *doc) : DiffEditor()
{
    Utils::GuardLocker guard(m_ignoreChanges);
    setDocument(QSharedPointer<DiffEditorDocument>(doc));
    setupView(loadSettings());
}

DiffEditor::~DiffEditor()
{
    delete m_toolBar;
    delete m_widget;
}

Core::IEditor *DiffEditor::duplicate()
{
    DiffEditor *editor = new DiffEditor();
    Utils::GuardLocker guard(editor->m_ignoreChanges);

    editor->setDocument(m_document);
    editor->m_sync = m_sync;
    editor->m_showDescription = m_showDescription;

    Core::Id id = currentView()->id();
    IDiffView *view = Utils::findOr(editor->m_views, editor->m_views.at(0),
                                    Utils::equal(&IDiffView::id, id));
    QTC_ASSERT(view, view = editor->currentView());
    editor->setupView(view);

    return editor;
}

Core::IDocument *DiffEditor::document()
{
    return m_document.data();
}

QWidget *DiffEditor::toolBar()
{
    QTC_ASSERT(m_toolBar, return 0);
    return m_toolBar;
}

void DiffEditor::documentHasChanged()
{
    Utils::GuardLocker guard(m_ignoreChanges);
    const QList<FileData> diffFileList = m_document->diffFiles();

    updateDescription();
    currentView()->setDiff(diffFileList, m_document->baseDirectory());

    m_entriesComboBox->clear();
    const int count = diffFileList.count();
    for (int i = 0; i < count; i++) {
        const DiffFileInfo leftEntry = diffFileList.at(i).leftFileInfo;
        const DiffFileInfo rightEntry = diffFileList.at(i).rightFileInfo;
        const QString leftShortFileName = Utils::FileName::fromString(leftEntry.fileName).fileName();
        const QString rightShortFileName = Utils::FileName::fromString(rightEntry.fileName).fileName();
        QString itemText;
        QString itemToolTip;
        if (leftEntry.fileName == rightEntry.fileName) {
            itemText = leftShortFileName;

            if (leftEntry.typeInfo.isEmpty() && rightEntry.typeInfo.isEmpty()) {
                itemToolTip = leftEntry.fileName;
            } else {
                itemToolTip = tr("[%1] vs. [%2] %3")
                        .arg(leftEntry.typeInfo,
                             rightEntry.typeInfo,
                             leftEntry.fileName);
            }
        } else {
            if (leftShortFileName == rightShortFileName) {
                itemText = leftShortFileName;
            } else {
                itemText = tr("%1 vs. %2")
                        .arg(leftShortFileName,
                             rightShortFileName);
            }

            if (leftEntry.typeInfo.isEmpty() && rightEntry.typeInfo.isEmpty()) {
                itemToolTip = tr("%1 vs. %2")
                        .arg(leftEntry.fileName,
                             rightEntry.fileName);
            } else {
                itemToolTip = tr("[%1] %2 vs. [%3] %4")
                        .arg(leftEntry.typeInfo,
                             leftEntry.fileName,
                             rightEntry.typeInfo,
                             rightEntry.fileName);
            }
        }
        m_entriesComboBox->addItem(itemText);
        m_entriesComboBox->setItemData(m_entriesComboBox->count() - 1,
                                       leftEntry.fileName, Qt::UserRole);
        m_entriesComboBox->setItemData(m_entriesComboBox->count() - 1,
                                       rightEntry.fileName, Qt::UserRole + 1);
        m_entriesComboBox->setItemData(m_entriesComboBox->count() - 1,
                                       itemToolTip, Qt::ToolTipRole);
    }
}

void DiffEditor::toggleDescription()
{
    if (m_ignoreChanges.isLocked())
        return;

    m_showDescription = !m_showDescription;
    saveSetting(QLatin1String(descriptionVisibleKeyC), m_showDescription);
    updateDescription();
}

void DiffEditor::updateDescription()
{
    QTC_ASSERT(m_toolBar, return);

    QString description = m_document->description();
    m_descriptionWidget->setPlainText(description);
    m_descriptionWidget->setVisible(m_showDescription && !description.isEmpty());

    Utils::GuardLocker guard(m_ignoreChanges);
    m_toggleDescriptionAction->setChecked(m_showDescription);
    m_toggleDescriptionAction->setToolTip(m_showDescription ? tr("Hide Change Description")
                                                      : tr("Show Change Description"));
    m_toggleDescriptionAction->setText(m_showDescription ? tr("Hide Change Description")
                                                   : tr("Show Change Description"));
    m_toggleDescriptionAction->setVisible(!description.isEmpty());
}

void DiffEditor::contextLineCountHasChanged(int lines)
{
    QTC_ASSERT(!m_document->isContextLineCountForced(), return);
    if (m_ignoreChanges.isLocked() || lines == m_document->contextLineCount())
        return;

    m_document->setContextLineCount(lines);
    saveSetting(QLatin1String(contextLineCountKeyC), lines);

    m_document->reload();
}

void DiffEditor::ignoreWhitespaceHasChanged()
{
    const bool ignore = m_whitespaceButtonAction->isChecked();

    if (m_ignoreChanges.isLocked() || ignore == m_document->ignoreWhitespace())
        return;
    m_document->setIgnoreWhitespace(ignore);
    saveSetting(QLatin1String(ignoreWhitespaceKeyC), ignore);

    m_document->reload();
}

void DiffEditor::prepareForReload()
{
    documentStateChanged(); // To update actions...

    QTC_ASSERT(currentView(), return);

    if (m_entriesComboBox->count() > 0) {
        m_currentFileChunk
                = qMakePair(m_entriesComboBox->itemData(m_currentDiffFileIndex, Qt::UserRole).toString(),
                            m_entriesComboBox->itemData(m_currentDiffFileIndex, Qt::UserRole + 1).toString());
    } else {
        m_currentFileChunk = qMakePair(QString(), QString());
    }

    {
        Utils::GuardLocker guard(m_ignoreChanges);
        m_contextSpinBox->setValue(m_document->contextLineCount());
        m_whitespaceButtonAction->setChecked(m_document->ignoreWhitespace());
    }
    currentView()->beginOperation();
}

void DiffEditor::reloadHasFinished(bool success)
{
    if (!currentView())
        return;

    currentView()->endOperation(success);

    int index = -1;
    const QString startupFile = m_document->startupFile();
    const QList<FileData> diffFileList = m_document->diffFiles();
    const int count = diffFileList.count();
    for (int i = 0; i < count; i++) {
        const DiffFileInfo leftEntry = diffFileList.at(i).leftFileInfo;
        const DiffFileInfo rightEntry = diffFileList.at(i).rightFileInfo;
        if ((m_currentFileChunk.first.isEmpty()
             && m_currentFileChunk.second.isEmpty()
             && startupFile.endsWith(rightEntry.fileName))
                || (m_currentFileChunk.first == leftEntry.fileName
                    && m_currentFileChunk.second == rightEntry.fileName)) {
            index = i;
            break;
        }
    }

    m_currentFileChunk = qMakePair(QString(), QString());
    if (index >= 0)
        setCurrentDiffFileIndex(index);
}

void DiffEditor::updateEntryToolTip()
{
    const QString &toolTip = m_entriesComboBox->itemData(
                m_entriesComboBox->currentIndex(), Qt::ToolTipRole).toString();
    m_entriesComboBox->setToolTip(toolTip);
}

void DiffEditor::setCurrentDiffFileIndex(int index)
{
    if (m_ignoreChanges.isLocked())
        return;

    QTC_ASSERT((index < 0) != (m_entriesComboBox->count() > 0), return);

    Utils::GuardLocker guard(m_ignoreChanges);
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
    saveSetting(QLatin1String(horizontalScrollBarSynchronizationKeyC), m_sync);
    currentView()->setSync(m_sync);
}

IDiffView *DiffEditor::loadSettings()
{
    QTC_ASSERT(currentView(), return 0);
    QSettings *s = Core::ICore::settings();

    // Read current settings:
    s->beginGroup(QLatin1String(settingsGroupC));
    m_showDescription = s->value(QLatin1String(descriptionVisibleKeyC), true).toBool();
    m_sync = s->value(QLatin1String(horizontalScrollBarSynchronizationKeyC), true).toBool();
    m_document->setIgnoreWhitespace(s->value(QLatin1String(ignoreWhitespaceKeyC), false).toBool());
    m_document->setContextLineCount(s->value(QLatin1String(contextLineCountKeyC), 3).toInt());
    Core::Id id = Core::Id::fromSetting(s->value(QLatin1String(diffViewKeyC)));
    s->endGroup();

    IDiffView *view = Utils::findOr(m_views, m_views.at(0),
                                    Utils::equal(&IDiffView::id, id));
    QTC_CHECK(view);

    return view;
}

void DiffEditor::saveSetting(const QString &key, const QVariant &value) const
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
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

    connect(view, &IDiffView::currentDiffFileIndexChanged, this, &DiffEditor::setCurrentDiffFileIndex);
}

IDiffView *DiffEditor::currentView() const
{
    if (m_currentViewIndex < 0)
        return 0;
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

    saveSetting(QLatin1String(diffViewKeyC), currentView()->id().toSetting());

    {
        Utils::GuardLocker guard(m_ignoreChanges);
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
        currentView()->setDocument(0);

    QTC_ASSERT(view, return);
    setupView(view);
}

} // namespace Internal
} // namespace DiffEditor

#include "diffeditor.moc"
