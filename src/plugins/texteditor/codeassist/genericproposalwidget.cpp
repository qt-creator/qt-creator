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

#include "genericproposalwidget.h"
#include "genericproposalmodel.h"
#include "assistproposalitem.h"
#include "codeassistant.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/completionsettings.h>
#include <texteditor/texteditorconstants.h>

#include <utils/faketooltip.h>
#include <utils/hostosinfo.h>

#include <QRect>
#include <QLatin1String>
#include <QAbstractListModel>
#include <QPointer>
#include <QDebug>
#include <QTimer>
#include <QApplication>
#include <QVBoxLayout>
#include <QListView>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QKeyEvent>
#include <QDesktopWidget>
#include <QLabel>

using namespace Utils;

namespace TextEditor {

static QString cleanText(const QString &original)
{
    QString clean = original;
    int ignore = 0;
    for (int i = clean.length() - 1; i >= 0; --i, ++ignore) {
        const QChar &c = clean.at(i);
        if (c.isLetterOrNumber() || c == QLatin1Char('_')
                || c.isHighSurrogate() || c.isLowSurrogate()) {
            break;
        }
    }
    if (ignore)
        clean.chop(ignore);
    return clean;
}

static bool isPerfectMatch(const QString &prefix, const GenericProposalModel *model)
{
    if (prefix.isEmpty())
        return false;

    for (int i = 0; i < model->size(); ++i) {
        const QString &current = cleanText(model->text(i));
        if (!current.isEmpty()) {
            CaseSensitivity cs = TextEditorSettings::completionSettings().m_caseSensitivity;
            if (cs == TextEditor::CaseSensitive) {
                if (prefix == current)
                    return true;
            } else if (cs == TextEditor::CaseInsensitive) {
                if (prefix.compare(current, Qt::CaseInsensitive) == 0)
                    return true;
            } else if (cs == TextEditor::FirstLetterCaseSensitive) {
                if (prefix.at(0) == current.at(0)
                        && prefix.midRef(1).compare(current.midRef(1), Qt::CaseInsensitive) == 0)
                    return true;
            }
        }
    }
    return false;
}

// ------------
// ModelAdapter
// ------------
class ModelAdapter : public QAbstractListModel
{
    Q_OBJECT

public:
    ModelAdapter(GenericProposalModel *completionModel, QWidget *parent);

    virtual int rowCount(const QModelIndex &) const;
    virtual QVariant data(const QModelIndex &index, int role) const;

private:
    GenericProposalModel *m_completionModel;
};

ModelAdapter::ModelAdapter(GenericProposalModel *completionModel, QWidget *parent)
    : QAbstractListModel(parent)
    , m_completionModel(completionModel)
{}

int ModelAdapter::rowCount(const QModelIndex &index) const
{
    return index.isValid() ? 0 : m_completionModel->size();
}

QVariant ModelAdapter::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_completionModel->size())
        return QVariant();

    if (role == Qt::DisplayRole)
        return m_completionModel->text(index.row());
    else if (role == Qt::DecorationRole)
        return m_completionModel->icon(index.row());
    else if (role == Qt::WhatsThisRole)
        return m_completionModel->detail(index.row());

    return QVariant();
}

// ------------------------
// GenericProposalInfoFrame
// ------------------------
class GenericProposalInfoFrame : public FakeToolTip
{
public:
    GenericProposalInfoFrame(QWidget *parent = 0)
        : FakeToolTip(parent), m_label(new QLabel(this))
    {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->setSpacing(0);
        layout->addWidget(m_label);

        // Limit horizontal width
        m_label->setSizePolicy(QSizePolicy::Fixed, m_label->sizePolicy().verticalPolicy());

        m_label->setTextFormat(Qt::RichText);
        m_label->setForegroundRole(QPalette::ToolTipText);
        m_label->setBackgroundRole(QPalette::ToolTipBase);
    }

    void setText(const QString &text)
    {
        m_label->setText(text);
    }

    // Workaround QTCREATORBUG-11653
    void calculateMaximumWidth()
    {
        const QDesktopWidget *desktopWidget = QApplication::desktop();
        const int desktopWidth = desktopWidget->isVirtualDesktop()
                ? desktopWidget->width()
                : desktopWidget->availableGeometry(desktopWidget->primaryScreen()).width();
        const QMargins widgetMargins = contentsMargins();
        const QMargins layoutMargins = layout()->contentsMargins();
        const int margins = widgetMargins.left() + widgetMargins.right()
                + layoutMargins.left() + layoutMargins.right();
        m_label->setMaximumWidth(desktopWidth - this->pos().x() - margins);
    }

private:
    QLabel *m_label;
};

// -----------------------
// GenericProposalListView
// -----------------------
class GenericProposalListView : public QListView
{
public:
    GenericProposalListView(QWidget *parent);

    QSize calculateSize() const;
    QPoint infoFramePos() const;

    int rowSelected() const { return currentIndex().row(); }
    bool isFirstRowSelected() const { return rowSelected() == 0; }
    bool isLastRowSelected() const { return rowSelected() == model()->rowCount() - 1; }
    void selectRow(int row) { setCurrentIndex(model()->index(row, 0)); }
    void selectFirstRow() { selectRow(0); }
    void selectLastRow() { selectRow(model()->rowCount() - 1); }
};

GenericProposalListView::GenericProposalListView(QWidget *parent)
    : QListView(parent)
{
    setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
}

QSize GenericProposalListView::calculateSize() const
{
    static const int maxVisibleItems = 10;

    // Determine size by calculating the space of the visible items
    const int visibleItems = qMin(model()->rowCount(), maxVisibleItems);
    const int firstVisibleRow = verticalScrollBar()->value();

    const QStyleOptionViewItem &option = viewOptions();
    QSize shint;
    for (int i = 0; i < visibleItems; ++i) {
        QSize tmp = itemDelegate()->sizeHint(option, model()->index(i + firstVisibleRow, 0));
        if (shint.width() < tmp.width())
            shint = tmp;
    }
    shint.rheight() *= visibleItems;

    return shint;
}

QPoint GenericProposalListView::infoFramePos() const
{
    const QRect &r = rectForIndex(currentIndex());
    QPoint p((parentWidget()->mapToGlobal(
                    parentWidget()->rect().topRight())).x() + 3,
            mapToGlobal(r.topRight()).y() - verticalOffset()
            );
    return p;
}

// ----------------------------
// GenericProposalWidgetPrivate
// ----------------------------
class GenericProposalWidgetPrivate : public QObject
{
    Q_OBJECT

public:
    GenericProposalWidgetPrivate(QWidget *completionWidget);

    const QWidget *m_underlyingWidget = nullptr;
    GenericProposalListView *m_completionListView;
    GenericProposalModel *m_model = nullptr;
    QRect m_displayRect;
    bool m_isSynchronized = true;
    bool m_explicitlySelected = false;
    AssistReason m_reason = IdleEditor;
    AssistKind m_kind = Completion;
    bool m_justInvoked = false;
    QPointer<GenericProposalInfoFrame> m_infoFrame;
    QTimer m_infoTimer;
    CodeAssistant *m_assistant = nullptr;
    bool m_autoWidth = true;

    void handleActivation(const QModelIndex &modelIndex);
    void maybeShowInfoTip();
};

GenericProposalWidgetPrivate::GenericProposalWidgetPrivate(QWidget *completionWidget)
    : m_completionListView(new GenericProposalListView(completionWidget))
{
    m_completionListView->setIconSize(QSize(16, 16));
    connect(m_completionListView, &QAbstractItemView::activated,
            this, &GenericProposalWidgetPrivate::handleActivation);

    m_infoTimer.setInterval(Constants::COMPLETION_ASSIST_TOOLTIP_DELAY);
    m_infoTimer.setSingleShot(true);
    connect(&m_infoTimer, &QTimer::timeout, this, &GenericProposalWidgetPrivate::maybeShowInfoTip);
}

void GenericProposalWidgetPrivate::handleActivation(const QModelIndex &modelIndex)
{
    static_cast<GenericProposalWidget *>
            (m_completionListView->parent())->notifyActivation(modelIndex.row());
}

void GenericProposalWidgetPrivate::maybeShowInfoTip()
{
    const QModelIndex &current = m_completionListView->currentIndex();
    if (!current.isValid())
        return;

    const QString &infoTip = current.data(Qt::WhatsThisRole).toString();
    if (infoTip.isEmpty()) {
        delete m_infoFrame.data();
        m_infoTimer.setInterval(200);
        return;
    }

    if (m_infoFrame.isNull())
        m_infoFrame = new GenericProposalInfoFrame(m_completionListView);

    m_infoFrame->move(m_completionListView->infoFramePos());
    m_infoFrame->setText(infoTip);
    m_infoFrame->calculateMaximumWidth();
    m_infoFrame->adjustSize();
    m_infoFrame->show();
    m_infoFrame->raise();

    m_infoTimer.setInterval(0);
}

// ------------------------
// GenericProposalWidget
// ------------------------
GenericProposalWidget::GenericProposalWidget()
    : d(new GenericProposalWidgetPrivate(this))
{
    if (HostOsInfo::isMacHost()) {
        if (d->m_completionListView->horizontalScrollBar())
            d->m_completionListView->horizontalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
        if (d->m_completionListView->verticalScrollBar())
            d->m_completionListView->verticalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
    } else {
        // This improves the look with QGTKStyle.
        setFrameStyle(d->m_completionListView->frameStyle());
    }
    d->m_completionListView->setFrameStyle(QFrame::NoFrame);
    d->m_completionListView->setAttribute(Qt::WA_MacShowFocusRect, false);
    d->m_completionListView->setUniformItemSizes(true);
    d->m_completionListView->setSelectionBehavior(QAbstractItemView::SelectItems);
    d->m_completionListView->setSelectionMode(QAbstractItemView::SingleSelection);
    d->m_completionListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->m_completionListView->setMinimumSize(1, 1);
    connect(d->m_completionListView->verticalScrollBar(), &QAbstractSlider::valueChanged,
            this, &GenericProposalWidget::updatePositionAndSize);
    connect(d->m_completionListView->verticalScrollBar(), &QAbstractSlider::sliderPressed,
            this, &GenericProposalWidget::turnOffAutoWidth);
    connect(d->m_completionListView->verticalScrollBar(), &QAbstractSlider::sliderReleased,
            this, &GenericProposalWidget::turnOnAutoWidth);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(d->m_completionListView);

    d->m_completionListView->installEventFilter(this);

    setObjectName(QLatin1String("m_popupFrame"));
    setMinimumSize(1, 1);
}

GenericProposalWidget::~GenericProposalWidget()
{
    delete d->m_model;
    delete d;
}

void GenericProposalWidget::setAssistant(CodeAssistant *assistant)
{
    d->m_assistant = assistant;
}

void GenericProposalWidget::setReason(AssistReason reason)
{
    d->m_reason = reason;
    if (d->m_reason == ExplicitlyInvoked)
        d->m_justInvoked = true;
}

void GenericProposalWidget::setKind(AssistKind kind)
{
    d->m_kind = kind;
}

void GenericProposalWidget::setUnderlyingWidget(const QWidget *underlyingWidget)
{
    setFont(underlyingWidget->font());
    d->m_underlyingWidget = underlyingWidget;
}

void GenericProposalWidget::setModel(IAssistProposalModel *model)
{
    delete d->m_model;
    d->m_model = static_cast<GenericProposalModel *>(model);
    d->m_completionListView->setModel(new ModelAdapter(d->m_model, d->m_completionListView));

    connect(d->m_completionListView->selectionModel(), &QItemSelectionModel::currentChanged,
            &d->m_infoTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
}

void GenericProposalWidget::setDisplayRect(const QRect &rect)
{
    d->m_displayRect = rect;
}

void GenericProposalWidget::setIsSynchronized(bool isSync)
{
    d->m_isSynchronized = isSync;
}

void GenericProposalWidget::showProposal(const QString &prefix)
{
    ensurePolished();
    if (d->m_model->containsDuplicates())
        d->m_model->removeDuplicates();
    if (!updateAndCheck(prefix))
        return;
    show();
    d->m_completionListView->setFocus();
}

void GenericProposalWidget::updateProposal(const QString &prefix)
{
    if (!isVisible())
        return;
    updateAndCheck(prefix);
}

void GenericProposalWidget::closeProposal()
{
    abort();
}

void GenericProposalWidget::notifyActivation(int index)
{
    abort();
    emit proposalItemActivated(d->m_model->proposalItem(index));
}

void GenericProposalWidget::abort()
{
    deleteLater();
    if (isVisible())
        close();
}

bool GenericProposalWidget::updateAndCheck(const QString &prefix)
{
    // Keep track in the case there has been an explicit selection.
    int preferredItemId = -1;
    if (d->m_explicitlySelected)
        preferredItemId =
                d->m_model->persistentId(d->m_completionListView->currentIndex().row());

    // Filter, sort, etc.
    d->m_model->reset();
    if (!prefix.isEmpty())
        d->m_model->filter(prefix);
    if (d->m_model->size() == 0
            || (!d->m_model->keepPerfectMatch(d->m_reason)
                && isPerfectMatch(prefix, d->m_model))) {
        d->m_completionListView->reset();
        abort();
        return false;
    }
    if (d->m_model->isSortable(prefix))
        d->m_model->sort(prefix);
    d->m_completionListView->reset();

    // Try to find the previosly explicit selection (if any). If we can find the item set it
    // as the current. Otherwise (it might have been filtered out) select the first row.
    if (d->m_explicitlySelected) {
        Q_ASSERT(preferredItemId != -1);
        for (int i = 0; i < d->m_model->size(); ++i) {
            if (d->m_model->persistentId(i) == preferredItemId) {
                d->m_completionListView->selectRow(i);
                break;
            }
        }
    }
    if (!d->m_completionListView->currentIndex().isValid()) {
        d->m_completionListView->selectFirstRow();
        if (d->m_explicitlySelected)
            d->m_explicitlySelected = false;
    }

    if (TextEditorSettings::completionSettings().m_partiallyComplete
            && d->m_kind == Completion
            && d->m_justInvoked
            && d->m_isSynchronized) {
        if (d->m_model->size() == 1) {
            AssistProposalItemInterface *item = d->m_model->proposalItem(0);
            if (item->implicitlyApplies()) {
                d->m_completionListView->reset();
                abort();
                emit proposalItemActivated(item);
                return false;
            }
        }
        if (d->m_model->supportsPrefixExpansion()) {
            const QString &proposalPrefix = d->m_model->proposalPrefix();
            if (proposalPrefix.length() > prefix.length())
                emit prefixExpanded(proposalPrefix);
        }
    }

    if (d->m_justInvoked)
        d->m_justInvoked = false;

    updatePositionAndSize();
    return true;
}

void GenericProposalWidget::updatePositionAndSize()
{
    if (!d->m_autoWidth)
        return;

    const QSize &shint = d->m_completionListView->calculateSize();
    const int fw = frameWidth();
    const int width = shint.width() + fw * 2 + 30;
    const int height = shint.height() + fw * 2;

    // Determine the position, keeping the popup on the screen
    const QDesktopWidget *desktop = QApplication::desktop();
    const QRect screen = HostOsInfo::isMacHost()
            ? desktop->availableGeometry(desktop->screenNumber(d->m_underlyingWidget))
            : desktop->screenGeometry(desktop->screenNumber(d->m_underlyingWidget));

    QPoint pos = d->m_displayRect.bottomLeft();
    pos.rx() -= 16 + fw;    // Space for the icons
    if (pos.y() + height > screen.bottom())
        pos.setY(qMax(0, d->m_displayRect.top() - height));
    if (pos.x() + width > screen.right())
        pos.setX(qMax(0, screen.right() - width));
    setGeometry(pos.x(), pos.y(), qMin(width, screen.width()), qMin(height, screen.height()));
}

void GenericProposalWidget::turnOffAutoWidth()
{
    d->m_autoWidth = false;
}

void GenericProposalWidget::turnOnAutoWidth()
{
    d->m_autoWidth = true;
    updatePositionAndSize();
}

bool GenericProposalWidget::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::FocusOut) {
        abort();
        if (d->m_infoFrame)
            d->m_infoFrame->close();
        return true;
    } else if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
        case Qt::Key_N:
        case Qt::Key_P:
            if (ke->modifiers() == Qt::KeyboardModifiers(HostOsInfo::controlModifier())) {
                e->accept();
                return true;
            }
        }
    } else if (e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
        case Qt::Key_Escape:
            abort();
            emit explicitlyAborted();
            e->accept();
            return true;

        case Qt::Key_N:
        case Qt::Key_P:
            // select next/previous completion
            d->m_explicitlySelected = true;
            if (ke->modifiers() == Qt::KeyboardModifiers(HostOsInfo::controlModifier())) {
                int change = (ke->key() == Qt::Key_N) ? 1 : -1;
                int nrows = d->m_model->size();
                int row = d->m_completionListView->currentIndex().row();
                int newRow = (row + change + nrows) % nrows;
                if (newRow == row + change || !ke->isAutoRepeat())
                    d->m_completionListView->selectRow(newRow);
                return true;
            }
            break;

        case Qt::Key_Tab:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            abort();
            activateCurrentProposalItem();
            return true;

        case Qt::Key_Up:
            d->m_explicitlySelected = true;
            if (!ke->isAutoRepeat() && d->m_completionListView->isFirstRowSelected()) {
                d->m_completionListView->selectLastRow();
                return true;
            }
            return false;

        case Qt::Key_Down:
            d->m_explicitlySelected = true;
            if (!ke->isAutoRepeat() && d->m_completionListView->isLastRowSelected()) {
                d->m_completionListView->selectFirstRow();
                return true;
            }
            return false;

        case Qt::Key_PageDown:
        case Qt::Key_PageUp:
            return false;

        case Qt::Key_Right:
        case Qt::Key_Left:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Backspace:
            // We want these navigation keys to work in the editor.
            break;

        default:
            // Only forward keys that insert text and refine the completion.
            if (ke->text().isEmpty() && !(ke == QKeySequence::Paste))
                return true;
            break;
        }

        if (ke->text().length() == 1
                && d->m_completionListView->currentIndex().isValid()
                && qApp->focusWidget() == o) {
            const QChar &typedChar = ke->text().at(0);
            AssistProposalItemInterface *item =
                d->m_model->proposalItem(d->m_completionListView->currentIndex().row());
            if (item->prematurelyApplies(typedChar)
                    && (d->m_reason == ExplicitlyInvoked || item->text().endsWith(typedChar))) {
                abort();
                emit proposalItemActivated(item);
                return true;
            }
        }

        QApplication::sendEvent(const_cast<QWidget *>(d->m_underlyingWidget), e);
        if (isVisible())
            d->m_assistant->notifyChange();

        return true;
    }
    return false;
}

bool GenericProposalWidget::activateCurrentProposalItem()
{
    if (d->m_completionListView->currentIndex().isValid()) {
        const int currentRow = d->m_completionListView->currentIndex().row();
        emit proposalItemActivated(d->m_model->proposalItem(currentRow));
        return true;
    }
    return false;
}

GenericProposalModel *GenericProposalWidget::model()
{
    return d->m_model;
}

} // namespace TextEditor

#include "genericproposalwidget.moc"
