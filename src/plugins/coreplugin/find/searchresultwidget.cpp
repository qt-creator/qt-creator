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

#include "searchresultwidget.h"
#include "searchresulttreeview.h"
#include "searchresulttreemodel.h"
#include "searchresulttreeitems.h"
#include "searchresulttreeitemroles.h"

#include "findplugin.h"
#include "itemviewfind.h"

#include <aggregation/aggregate.h>
#include <coreplugin/coreplugin.h>

#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
#include <utils/fancylineedit.h>

#include <QDir>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

static const int SEARCHRESULT_WARNING_LIMIT = 200000;
static const char SIZE_WARNING_ID[] = "sizeWarningLabel";

using namespace Utils;

namespace Core {
namespace Internal {

class WideEnoughLineEdit : public Utils::FancyLineEdit
{
    Q_OBJECT

public:
    WideEnoughLineEdit(QWidget *parent) : Utils::FancyLineEdit(parent)
    {
        setFiltering(true);
        setPlaceholderText(QString());
        connect(this, &QLineEdit::textChanged, this, &QLineEdit::updateGeometry);

    }

    QSize sizeHint() const
    {
        QSize sh = QLineEdit::minimumSizeHint();
        sh.rwidth() += qMax(25 * fontMetrics().width(QLatin1Char('x')),
                            fontMetrics().width(text()));
        return sh;
    }
};

SearchResultWidget::SearchResultWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);

    QFrame *topWidget = new QFrame;
    QPalette pal;
    pal.setColor(QPalette::Window,     creatorTheme()->color(Theme::InfoBarBackground));
    pal.setColor(QPalette::WindowText, creatorTheme()->color(Theme::InfoBarText));
    topWidget->setPalette(pal);
    if (creatorTheme()->flag(Theme::DrawSearchResultWidgetFrame)) {
        topWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
        topWidget->setLineWidth(1);
    }
    topWidget->setAutoFillBackground(true);
    auto topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(2, 2, 2, 2);
    topLayout->setSpacing(2);
    topWidget->setLayout(topLayout);
    layout->addWidget(topWidget);

    auto topFindWidget = new QWidget(topWidget);
    auto topFindLayout = new QHBoxLayout(topFindWidget);
    topFindLayout->setMargin(0);
    topFindWidget->setLayout(topFindLayout);
    topLayout->addWidget(topFindWidget);

    m_topReplaceWidget = new QWidget(topWidget);
    auto topReplaceLayout = new QHBoxLayout(m_topReplaceWidget);
    topReplaceLayout->setMargin(0);
    m_topReplaceWidget->setLayout(topReplaceLayout);
    topLayout->addWidget(m_topReplaceWidget);

    m_messageWidget = new QFrame;
    pal.setColor(QPalette::WindowText, creatorTheme()->color(Theme::CanceledSearchTextColor));
    m_messageWidget->setPalette(pal);
    if (creatorTheme()->flag(Theme::DrawSearchResultWidgetFrame)) {
        m_messageWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
        m_messageWidget->setLineWidth(1);
    }
    m_messageWidget->setAutoFillBackground(true);
    QHBoxLayout *messageLayout = new QHBoxLayout(m_messageWidget);
    messageLayout->setMargin(2);
    m_messageWidget->setLayout(messageLayout);
    QLabel *messageLabel = new QLabel(tr("Search was canceled."));
    messageLabel->setPalette(pal);
    messageLayout->addWidget(messageLabel);
    layout->addWidget(m_messageWidget);
    m_messageWidget->setVisible(false);

    m_searchResultTreeView = new SearchResultTreeView(this);
    m_searchResultTreeView->setFrameStyle(QFrame::NoFrame);
    m_searchResultTreeView->setAttribute(Qt::WA_MacShowFocusRect, false);
    Aggregation::Aggregate * agg = new Aggregation::Aggregate;
    agg->add(m_searchResultTreeView);
    agg->add(new ItemViewFind(m_searchResultTreeView,
                                      ItemDataRoles::ResultLineRole));
    layout->addWidget(m_searchResultTreeView);

    m_infoBarDisplay.setTarget(layout, 2);
    m_infoBarDisplay.setInfoBar(&m_infoBar);

    m_descriptionContainer = new QWidget(topFindWidget);
    QHBoxLayout *descriptionLayout = new QHBoxLayout(m_descriptionContainer);
    m_descriptionContainer->setLayout(descriptionLayout);
    descriptionLayout->setMargin(0);
    m_descriptionContainer->setMinimumWidth(200);
    m_descriptionContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_label = new QLabel(m_descriptionContainer);
    m_label->setVisible(false);
    m_searchTerm = new QLabel(m_descriptionContainer);
    m_searchTerm->setTextFormat(Qt::PlainText);
    m_searchTerm->setVisible(false);
    descriptionLayout->addWidget(m_label);
    descriptionLayout->addWidget(m_searchTerm);
    m_cancelButton = new QToolButton(topFindWidget);
    m_cancelButton->setText(tr("Cancel"));
    m_cancelButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    connect(m_cancelButton, &QAbstractButton::clicked, this, &SearchResultWidget::cancel);
    m_searchAgainButton = new QToolButton(topFindWidget);
    m_searchAgainButton->setToolTip(tr("Repeat the search with same parameters."));
    m_searchAgainButton->setText(tr("&Search Again"));
    m_searchAgainButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_searchAgainButton->setVisible(false);
    connect(m_searchAgainButton, &QAbstractButton::clicked, this, &SearchResultWidget::searchAgain);

    m_replaceLabel = new QLabel(tr("Repla&ce with:"), m_topReplaceWidget);
    m_replaceTextEdit = new WideEnoughLineEdit(m_topReplaceWidget);
    m_replaceLabel->setBuddy(m_replaceTextEdit);
    m_replaceTextEdit->setMinimumWidth(120);
    m_replaceTextEdit->setEnabled(false);
    m_replaceTextEdit->setTabOrder(m_replaceTextEdit, m_searchResultTreeView);
    m_preserveCaseCheck = new QCheckBox(m_topReplaceWidget);
    m_preserveCaseCheck->setText(tr("Preser&ve case"));
    m_preserveCaseCheck->setEnabled(false);
    m_renameFilesCheckBox = new QCheckBox(m_topReplaceWidget);
    m_renameFilesCheckBox->setVisible(false);
    m_replaceButton = new QToolButton(m_topReplaceWidget);
    m_replaceButton->setToolTip(tr("Replace all occurrences."));
    m_replaceButton->setText(tr("&Replace"));
    m_replaceButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_replaceButton->setEnabled(false);

    m_preserveCaseCheck->setChecked(Find::hasFindFlag(FindPreserveCase));
    connect(m_preserveCaseCheck, &QAbstractButton::clicked, Find::instance(), &Find::setPreserveCase);

    m_matchesFoundLabel = new QLabel(topFindWidget);
    updateMatchesFoundLabel();

    topFindLayout->addWidget(m_descriptionContainer);
    topFindLayout->addWidget(m_cancelButton);
    topFindLayout->addWidget(m_searchAgainButton);
    topFindLayout->addStretch(2);
    topFindLayout->addWidget(m_matchesFoundLabel);
    topReplaceLayout->addWidget(m_replaceLabel);
    topReplaceLayout->addWidget(m_replaceTextEdit);
    topReplaceLayout->addWidget(m_preserveCaseCheck);
    topReplaceLayout->addWidget(m_renameFilesCheckBox);
    topReplaceLayout->addWidget(m_replaceButton);
    topReplaceLayout->addStretch(2);
    setShowReplaceUI(m_replaceSupported);
    setSupportPreserveCase(true);

    connect(m_searchResultTreeView, &SearchResultTreeView::jumpToSearchResult,
            this, &SearchResultWidget::handleJumpToSearchResult);
    connect(m_replaceTextEdit, &QLineEdit::returnPressed,
            this, &SearchResultWidget::handleReplaceButton);
    connect(m_replaceButton, &QAbstractButton::clicked,
            this, &SearchResultWidget::handleReplaceButton);
}

SearchResultWidget::~SearchResultWidget()
{
    if (m_infoBar.containsInfo(Id(SIZE_WARNING_ID)))
        cancelAfterSizeWarning();
}

void SearchResultWidget::setInfo(const QString &label, const QString &toolTip, const QString &term)
{
    m_label->setText(label);
    m_label->setVisible(!label.isEmpty());
    m_descriptionContainer->setToolTip(toolTip);
    m_searchTerm->setText(term);
    m_searchTerm->setVisible(!term.isEmpty());
}

QWidget *SearchResultWidget::additionalReplaceWidget() const
{
    return m_renameFilesCheckBox;
}

void SearchResultWidget::addResult(const QString &fileName,
                                   const QString &rowText,
                                   Search::TextRange mainRange,
                                   const QVariant &userData)
{
    SearchResultItem item;
    item.path = QStringList({QDir::toNativeSeparators(fileName)});
    item.mainRange = mainRange;
    item.text = rowText;
    item.useTextEditorFont = true;
    item.userData = userData;
    addResults(QList<SearchResultItem>() << item, SearchResult::AddOrdered);
}

void SearchResultWidget::addResults(const QList<SearchResultItem> &items, SearchResult::AddMode mode)
{
    bool firstItems = (m_count == 0);
    m_count += items.size();
    m_searchResultTreeView->addResults(items, mode);
    updateMatchesFoundLabel();
    if (firstItems) {
        if (!m_dontAskAgainGroup.isEmpty()) {
            Id undoWarningId = Id("warninglabel/").withSuffix(m_dontAskAgainGroup);
            if (m_infoBar.canInfoBeAdded(undoWarningId)) {
                InfoBarEntry info(undoWarningId, tr("This change cannot be undone."),
                                  InfoBarEntry::GlobalSuppressionEnabled);
                m_infoBar.addInfo(info);
            }
        }

        m_replaceTextEdit->setEnabled(true);
        // We didn't have an item before, set the focus to the search widget or replace text edit
        setShowReplaceUI(m_replaceSupported);
        if (m_replaceSupported) {
            m_replaceTextEdit->setFocus();
            m_replaceTextEdit->selectAll();
        } else {
            m_searchResultTreeView->setFocus();
        }
        m_searchResultTreeView->selectionModel()->select(m_searchResultTreeView->model()->index(0, 0, QModelIndex()), QItemSelectionModel::Select);
        emit navigateStateChanged();
    } else if (m_count <= SEARCHRESULT_WARNING_LIMIT) {
        return;
    } else {
        Id sizeWarningId(SIZE_WARNING_ID);
        if (!m_infoBar.canInfoBeAdded(sizeWarningId))
            return;
        emit paused(true);
        InfoBarEntry info(sizeWarningId,
                          tr("The search resulted in more than %n items, do you still want to continue?",
                             0, SEARCHRESULT_WARNING_LIMIT));
        info.setCancelButtonInfo(tr("Cancel"), [this]() { cancelAfterSizeWarning(); });
        info.setCustomButtonInfo(tr("Continue"), [this]() { continueAfterSizeWarning(); });
        m_infoBar.addInfo(info);
        emit requestPopup(false/*no focus*/);
    }
}

int SearchResultWidget::count() const
{
    return m_count;
}

void SearchResultWidget::setSupportsReplace(bool replaceSupported, const QString &group)
{
    m_replaceSupported = replaceSupported;
    setShowReplaceUI(replaceSupported);
    m_dontAskAgainGroup = group;
}

void SearchResultWidget::setTextToReplace(const QString &textToReplace)
{
    m_replaceTextEdit->setText(textToReplace);
}

QString SearchResultWidget::textToReplace() const
{
    return m_replaceTextEdit->text();
}

void SearchResultWidget::setSupportPreserveCase(bool enabled)
{
    m_preserveCaseSupported = enabled;
    m_preserveCaseCheck->setVisible(m_preserveCaseSupported);
}

void SearchResultWidget::setShowReplaceUI(bool visible)
{
    m_searchResultTreeView->model()->setShowReplaceUI(visible);
    m_topReplaceWidget->setVisible(visible);
    m_isShowingReplaceUI = visible;
}

bool SearchResultWidget::hasFocusInternally() const
{
    return m_searchResultTreeView->hasFocus() || (m_isShowingReplaceUI && m_replaceTextEdit->hasFocus());
}

void SearchResultWidget::setFocusInternally()
{
    if (m_count > 0) {
        if (m_isShowingReplaceUI) {
            if (!focusWidget() || focusWidget() == m_replaceTextEdit) {
                m_replaceTextEdit->setFocus();
                m_replaceTextEdit->selectAll();
            } else {
                m_searchResultTreeView->setFocus();
            }
        } else {
            m_searchResultTreeView->setFocus();
        }
    }
}

bool SearchResultWidget::canFocusInternally() const
{
    return m_count > 0;
}

void SearchResultWidget::notifyVisibilityChanged(bool visible)
{
    emit visibilityChanged(visible);
}

void SearchResultWidget::setTextEditorFont(const QFont &font, const SearchResultColor &color)
{
    m_searchResultTreeView->setTextEditorFont(font, color);
}

void SearchResultWidget::setTabWidth(int tabWidth)
{
    m_searchResultTreeView->setTabWidth(tabWidth);
}

void SearchResultWidget::setAutoExpandResults(bool expand)
{
    m_searchResultTreeView->setAutoExpandResults(expand);
}

void SearchResultWidget::expandAll()
{
    m_searchResultTreeView->expandAll();
}

void SearchResultWidget::collapseAll()
{
    m_searchResultTreeView->collapseAll();
}

void SearchResultWidget::goToNext()
{
    if (m_count == 0)
        return;
    QModelIndex idx = m_searchResultTreeView->model()->next(m_searchResultTreeView->currentIndex());
    if (idx.isValid()) {
        m_searchResultTreeView->setCurrentIndex(idx);
        m_searchResultTreeView->emitJumpToSearchResult(idx);
    }
}

void SearchResultWidget::goToPrevious()
{
    if (!m_searchResultTreeView->model()->rowCount())
        return;
    QModelIndex idx = m_searchResultTreeView->model()->prev(m_searchResultTreeView->currentIndex());
    if (idx.isValid()) {
        m_searchResultTreeView->setCurrentIndex(idx);
        m_searchResultTreeView->emitJumpToSearchResult(idx);
    }
}

void SearchResultWidget::restart()
{
    m_replaceTextEdit->setEnabled(false);
    m_replaceButton->setEnabled(false);
    m_searchResultTreeView->clear();
    m_count = 0;
    Id sizeWarningId(SIZE_WARNING_ID);
    m_infoBar.removeInfo(sizeWarningId);
    m_infoBar.enableInfo(sizeWarningId);
    m_cancelButton->setVisible(true);
    m_searchAgainButton->setVisible(false);
    m_messageWidget->setVisible(false);
    updateMatchesFoundLabel();
    emit restarted();
}

void SearchResultWidget::setSearchAgainSupported(bool supported)
{
    m_searchAgainSupported = supported;
    m_searchAgainButton->setVisible(supported && !m_cancelButton->isVisible());
}

void SearchResultWidget::setSearchAgainEnabled(bool enabled)
{
    m_searchAgainButton->setEnabled(enabled);
}

void SearchResultWidget::finishSearch(bool canceled)
{
    Id sizeWarningId(SIZE_WARNING_ID);
    m_infoBar.removeInfo(sizeWarningId);
    m_infoBar.enableInfo(sizeWarningId);
    m_replaceTextEdit->setEnabled(m_count > 0);
    m_replaceButton->setEnabled(m_count > 0);
    m_preserveCaseCheck->setEnabled(m_count > 0);
    m_cancelButton->setVisible(false);
    m_messageWidget->setVisible(canceled);
    m_searchAgainButton->setVisible(m_searchAgainSupported);
}

void SearchResultWidget::sendRequestPopup()
{
    emit requestPopup(true/*focus*/);
}

void SearchResultWidget::continueAfterSizeWarning()
{
    m_infoBar.suppressInfo(Id(SIZE_WARNING_ID));
    emit paused(false);
}

void SearchResultWidget::cancelAfterSizeWarning()
{
    m_infoBar.suppressInfo(Id(SIZE_WARNING_ID));
    emit cancelled();
    emit paused(false);
}

void SearchResultWidget::handleJumpToSearchResult(const SearchResultItem &item)
{
    emit activated(item);
}

void SearchResultWidget::handleReplaceButton()
{
    // check if button is actually enabled, because this is also triggered
    // by pressing return in replace line edit
    if (m_replaceButton->isEnabled()) {
        m_infoBar.clear();
        setShowReplaceUI(false);
        emit replaceButtonClicked(m_replaceTextEdit->text(), checkedItems(),
                                  m_preserveCaseSupported && m_preserveCaseCheck->isChecked());
    }
}

void SearchResultWidget::cancel()
{
    m_cancelButton->setVisible(false);
    if (m_infoBar.containsInfo(Id(SIZE_WARNING_ID)))
        cancelAfterSizeWarning();
    else
        emit cancelled();
}

void SearchResultWidget::searchAgain()
{
    emit searchAgainRequested();
}

QList<SearchResultItem> SearchResultWidget::checkedItems() const
{
    QList<SearchResultItem> result;
    SearchResultTreeModel *model = m_searchResultTreeView->model();
    const int fileCount = model->rowCount();
    for (int i = 0; i < fileCount; ++i) {
        QModelIndex fileIndex = model->index(i, 0);
        SearchResultTreeItem *fileItem = static_cast<SearchResultTreeItem *>(fileIndex.internalPointer());
        QTC_ASSERT(fileItem != 0, continue);
        for (int rowIndex = 0; rowIndex < fileItem->childrenCount(); ++rowIndex) {
            QModelIndex textIndex = model->index(rowIndex, 0, fileIndex);
            SearchResultTreeItem *rowItem = static_cast<SearchResultTreeItem *>(textIndex.internalPointer());
            QTC_ASSERT(rowItem != 0, continue);
            if (rowItem->checkState())
                result << rowItem->item;
        }
    }
    return result;
}

void SearchResultWidget::updateMatchesFoundLabel()
{
    if (m_count == 0)
        m_matchesFoundLabel->setText(tr("No matches found."));
    else
        m_matchesFoundLabel->setText(tr("%n matches found.", 0, m_count));
}

} // namespace Internal
} // namespace Core

#include "searchresultwidget.moc"
