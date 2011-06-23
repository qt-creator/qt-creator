/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "genericproposalwidget.h"
#include "iassistprovider.h"
#include "igenericproposalmodel.h"
#include "iassistproposalitem.h"
#include "genericproposal.h"
#include "codeassistant.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/completionsettings.h>

#include <utils/faketooltip.h>

#include <QtCore/QRect>
#include <QtCore/QLatin1String>
#include <QtCore/QAbstractListModel>
#include <QtCore/QPointer>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QVBoxLayout>
#include <QtGui/QListView>
#include <QtGui/QAbstractItemView>
#include <QtGui/QScrollBar>
#include <QtGui/QKeyEvent>
#include <QtGui/QDesktopWidget>
#include <QtGui/QLabel>


namespace TextEditor {

namespace {

QString cleanText(const QString &original)
{
    QString clean = original;
    int ignore = 0;
    for (int i = clean.length() - 1; i >= 0; --i, ++ignore) {
        const QChar &c = clean.at(i);
        if (c.isLetterOrNumber() || c == QLatin1Char('_'))
            break;
    }
    if (ignore)
        clean.chop(ignore);
    return clean;
}

bool isPerfectMatch(const QString &prefix, const IGenericProposalModel *model)
{
    if (prefix.isEmpty())
        return false;

    for (int i = 0; i < model->size(); ++i) {
        const QString &current = cleanText(model->text(i));
        if (!current.isEmpty()) {
            TextEditor::CaseSensitivity cs =
                TextEditor::TextEditorSettings::instance()->completionSettings().m_caseSensitivity;
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

}

// ------------
// ModelAdapter
// ------------
class ModelAdapter : public QAbstractListModel
{
    Q_OBJECT

public:
    ModelAdapter(IGenericProposalModel *completionModel, QWidget *parent);

    virtual int rowCount(const QModelIndex &) const;
    virtual QVariant data(const QModelIndex &index, int role) const;

private:
    IGenericProposalModel *m_completionModel;
};

ModelAdapter::ModelAdapter(IGenericProposalModel *completionModel, QWidget *parent)
    : QAbstractListModel(parent)
    , m_completionModel(completionModel)
{}

int ModelAdapter::rowCount(const QModelIndex &) const
{
    return m_completionModel->size();
}

QVariant ModelAdapter::data(const QModelIndex &index, int role) const
{
    if (index.row() >= m_completionModel->size())
        return QVariant();

    if (role == Qt::DisplayRole) {
        return m_completionModel->text(index.row());
    } else if (role == Qt::DecorationRole) {
        return m_completionModel->icon(index.row());
    } else if (role == Qt::WhatsThisRole) {
        return m_completionModel->detail(index.row());
    }

    return QVariant();
}

// ------------------------
// GenericProposalInfoFrame
// ------------------------
class GenericProposalInfoFrame : public Utils::FakeToolTip
{
public:
    GenericProposalInfoFrame(QWidget *parent = 0)
        : Utils::FakeToolTip(parent), m_label(new QLabel(this))
    {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->setSpacing(0);
        layout->addWidget(m_label);

        m_label->setForegroundRole(QPalette::ToolTipText);
        m_label->setBackgroundRole(QPalette::ToolTipBase);
    }

    void setText(const QString &text)
    {
        m_label->setText(text);
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
    GenericProposalListView(QWidget *parent) : QListView(parent) {}

    QSize calculateSize() const;
    QPoint infoFramePos() const;

    int rowSelected() const { return currentIndex().row(); }
    bool isFirstRowSelected() const { return rowSelected() == 0; }
    bool isLastRowSelected() const { return rowSelected() == model()->rowCount() - 1; }
    void selectRow(int row) { setCurrentIndex(model()->index(row, 0)); }
    void selectFirstRow() { selectRow(0); }
    void selectLastRow() { selectRow(model()->rowCount() - 1); }
};

QSize GenericProposalListView::calculateSize() const
{
    static const int maxVisibleItems = 10;

    // Determine size by calculating the space of the visible items
    int visibleItems = model()->rowCount();
    if (visibleItems > maxVisibleItems)
        visibleItems = maxVisibleItems;

    const QStyleOptionViewItem &option = viewOptions();
    QSize shint;
    for (int i = 0; i < visibleItems; ++i) {
        QSize tmp = itemDelegate()->sizeHint(option, model()->index(i, 0));
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

    const QWidget *m_underlyingWidget;
    GenericProposalListView *m_completionListView;
    IGenericProposalModel *m_model;
    QRect m_displayRect;
    bool m_isSynchronized;
    bool m_explicitlySelected;
    AssistReason m_reason;
    AssistKind m_kind;
    bool m_justInvoked;
    QPointer<GenericProposalInfoFrame> m_infoFrame;
    QTimer m_infoTimer;
    CodeAssistant *m_assistant;

public slots:
    void handleActivation(const QModelIndex &modelIndex);
    void maybeShowInfoTip();
};

GenericProposalWidgetPrivate::GenericProposalWidgetPrivate(QWidget *completionWidget)
    : m_underlyingWidget(0)
    , m_completionListView(new GenericProposalListView(completionWidget))
    , m_model(0)
    , m_isSynchronized(true)
    , m_explicitlySelected(false)
    , m_justInvoked(false)
    , m_assistant(0)
{
    connect(m_completionListView, SIGNAL(activated(QModelIndex)),
            this, SLOT(handleActivation(QModelIndex)));

    m_infoTimer.setInterval(1000);
    m_infoTimer.setSingleShot(true);
    connect(&m_infoTimer, SIGNAL(timeout()), SLOT(maybeShowInfoTip()));
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
    m_infoFrame->adjustSize();
    m_infoFrame->show();
    m_infoFrame->raise();

    m_infoTimer.setInterval(0);
}

// ------------------------
// GenericProposalWidget
// ------------------------
GenericProposalWidget::GenericProposalWidget()
    : m_d(new GenericProposalWidgetPrivate(this))
{
#ifdef Q_WS_MAC
    if (m_d->m_completionListView->horizontalScrollBar())
        m_d->m_completionListView->horizontalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
    if (m_d->m_completionListView->verticalScrollBar())
        m_d->m_completionListView->verticalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
#else
    // This improves the look with QGTKStyle.
    setFrameStyle(m_d->m_completionListView->frameStyle());
#endif
    m_d->m_completionListView->setFrameStyle(QFrame::NoFrame);
    m_d->m_completionListView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_d->m_completionListView->setUniformItemSizes(true);
    m_d->m_completionListView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_d->m_completionListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_d->m_completionListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_d->m_completionListView->setMinimumSize(1, 1);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_d->m_completionListView);

    m_d->m_completionListView->installEventFilter(this);

    setObjectName(QLatin1String("m_popupFrame"));
    setMinimumSize(1, 1);
}

GenericProposalWidget::~GenericProposalWidget()
{
    delete m_d->m_model;
}

void GenericProposalWidget::setAssistant(CodeAssistant *assistant)
{
    m_d->m_assistant = assistant;
}

void GenericProposalWidget::setReason(AssistReason reason)
{
    m_d->m_reason = reason;
    if (m_d->m_reason == ExplicitlyInvoked)
        m_d->m_justInvoked = true;
}

void GenericProposalWidget::setKind(AssistKind kind)
{
    m_d->m_kind = kind;
}

void GenericProposalWidget::setUnderlyingWidget(const QWidget *underlyingWidget)
{
    setFont(underlyingWidget->font());
    m_d->m_underlyingWidget = underlyingWidget;
}

void GenericProposalWidget::setModel(IAssistProposalModel *model)
{
    delete m_d->m_model;
    m_d->m_model = static_cast<IGenericProposalModel *>(model);
    m_d->m_completionListView->setModel(new ModelAdapter(m_d->m_model, m_d->m_completionListView));

    connect(m_d->m_completionListView->selectionModel(),
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            &m_d->m_infoTimer,
            SLOT(start()));
}

void GenericProposalWidget::setDisplayRect(const QRect &rect)
{
    m_d->m_displayRect = rect;
}

void GenericProposalWidget::setIsSynchronized(bool isSync)
{
    m_d->m_isSynchronized = isSync;
}

void GenericProposalWidget::showProposal(const QString &prefix)
{
    ensurePolished();
    m_d->m_model->removeDuplicates();
    if (!updateAndCheck(prefix))
        return;
    show();
    m_d->m_completionListView->setFocus();
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
    emit proposalItemActivated(m_d->m_model->proposalItem(index));
}

void GenericProposalWidget::abort()
{
    if (isVisible())
        close();
    deleteLater();
}

bool GenericProposalWidget::updateAndCheck(const QString &prefix)
{
    // Keep track in the case there has been an explicit selection.
    int preferredItemId = -1;
    if (m_d->m_explicitlySelected)
        preferredItemId =
                m_d->m_model->persistentId(m_d->m_completionListView->currentIndex().row());

    // Filter, sort, etc.
    m_d->m_model->reset();
    if (!prefix.isEmpty())
        m_d->m_model->filter(prefix);
    if (m_d->m_model->size() == 0
            || (!m_d->m_model->keepPerfectMatch(m_d->m_reason)
                && isPerfectMatch(prefix, m_d->m_model))) {
        abort();
        return false;
    }
    if (m_d->m_model->isSortable())
        m_d->m_model->sort();
    m_d->m_completionListView->reset();

    // Try to find the previosly explicit selection (if any). If we can find the item set it
    // as the current. Otherwise (it might have been filtered out) select the first row.
    if (m_d->m_explicitlySelected) {
        Q_ASSERT(preferredItemId != -1);
        for (int i = 0; i < m_d->m_model->size(); ++i) {
            if (m_d->m_model->persistentId(i) == preferredItemId) {
                m_d->m_completionListView->selectRow(i);
                break;
            }
        }
    }
    if (!m_d->m_completionListView->currentIndex().isValid()) {
        m_d->m_completionListView->selectFirstRow();
        if (m_d->m_explicitlySelected)
            m_d->m_explicitlySelected = false;
    }

    if (TextEditorSettings::instance()->completionSettings().m_partiallyComplete
            && m_d->m_kind == Completion
            && m_d->m_justInvoked
            && m_d->m_isSynchronized) {
        if (m_d->m_model->size() == 1) {
            IAssistProposalItem *item = m_d->m_model->proposalItem(0);
            if (item->implicitlyApplies()) {
                abort();
                emit proposalItemActivated(item);
                return false;
            }
        }
        if (m_d->m_model->supportsPrefixExpansion()) {
            const QString &proposalPrefix = m_d->m_model->proposalPrefix();
            if (proposalPrefix.length() > prefix.length())
                emit prefixExpanded(proposalPrefix);
        }
    }

    if (m_d->m_justInvoked)
        m_d->m_justInvoked = false;

    updatePositionAndSize();
    return true;
}

void GenericProposalWidget::updatePositionAndSize()
{
    const QSize &shint = m_d->m_completionListView->calculateSize();
    const int fw = frameWidth();
    const int width = shint.width() + fw * 2 + 30;
    const int height = shint.height() + fw * 2;

    // Determine the position, keeping the popup on the screen
    const QDesktopWidget *desktop = QApplication::desktop();
#ifdef Q_WS_MAC
    const QRect screen = desktop->availableGeometry(desktop->screenNumber(m_d->m_underlyingWidget));
#else
    const QRect screen = desktop->screenGeometry(desktop->screenNumber(m_d->m_underlyingWidget));
#endif

    QPoint pos = m_d->m_displayRect.bottomLeft();
    pos.rx() -= 16 + fw;    // Space for the icons
    if (pos.y() + height > screen.bottom())
        pos.setY(m_d->m_displayRect.top() - height);
    if (pos.x() + width > screen.right())
        pos.setX(screen.right() - width);
    setGeometry(pos.x(), pos.y(), width, height);
}

bool GenericProposalWidget::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::FocusOut) {
        abort();
#if defined(Q_OS_DARWIN) && ! defined(QT_MAC_USE_COCOA)
        QFocusEvent *fe = static_cast<QFocusEvent *>(e);
        if (fe->reason() == Qt::OtherFocusReason) {
            // Qt/carbon workaround
            // focus out is received before the key press event.
            if (m_d->m_completionListView->currentIndex().isValid())
                emit proposalItemActivated(m_d->m_model->proposalItem(
                                              m_d->m_completionListView->currentIndex().row()));
        }
#endif
        if (m_d->m_infoFrame)
            m_d->m_infoFrame->close();
        return true;
    } else if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
        case Qt::Key_N:
        case Qt::Key_P:
            if (ke->modifiers() == Qt::ControlModifier) {
                e->accept();
                return true;
            }
        }
    } else if (e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
        case Qt::Key_Escape:
            abort();
            return true;

        case Qt::Key_N:
        case Qt::Key_P:
            // select next/previous completion
            m_d->m_explicitlySelected = true;
            if (ke->modifiers() == Qt::ControlModifier) {
                int change = (ke->key() == Qt::Key_N) ? 1 : -1;
                int nrows = m_d->m_model->size();
                int row = m_d->m_completionListView->currentIndex().row();
                int newRow = (row + change + nrows) % nrows;
                if (newRow == row + change || !ke->isAutoRepeat())
                    m_d->m_completionListView->selectRow(newRow);
                return true;
            }
            break;

        case Qt::Key_Tab:
        case Qt::Key_Return:
#if defined(QT_MAC_USE_COCOA) || !defined(Q_OS_DARWIN)
            abort();
            if (m_d->m_completionListView->currentIndex().isValid())
                emit proposalItemActivated(m_d->m_model->proposalItem(
                                              m_d->m_completionListView->currentIndex().row()));
#endif
            return true;

        case Qt::Key_Up:
            m_d->m_explicitlySelected = true;
            if (!ke->isAutoRepeat() && m_d->m_completionListView->isFirstRowSelected()) {
                m_d->m_completionListView->selectLastRow();
                return true;
            }
            return false;

        case Qt::Key_Down:
            m_d->m_explicitlySelected = true;
            if (!ke->isAutoRepeat() && m_d->m_completionListView->isLastRowSelected()) {
                m_d->m_completionListView->selectFirstRow();
                return true;
            }
            return false;

        case Qt::Key_Enter:
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
            if (ke->text().isEmpty())
                return true;
            break;
        }

        if (ke->text().length() == 1
                && m_d->m_completionListView->currentIndex().isValid()
                && qApp->focusWidget() == o) {
            const QChar &typedChar = ke->text().at(0);
            IAssistProposalItem *item =
                m_d->m_model->proposalItem(m_d->m_completionListView->currentIndex().row());
            if (item->prematurelyApplies(typedChar)
                    && (m_d->m_reason == ExplicitlyInvoked || item->text().endsWith(typedChar))) {
                abort();
                emit proposalItemActivated(item);
                return true;
            }
        }

        QApplication::sendEvent(const_cast<QWidget *>(m_d->m_underlyingWidget), e);
        if (isVisible())
            m_d->m_assistant->notifyChange();

        return true;
    }
    return false;
}

#include "genericproposalwidget.moc"

} // TextEditor
