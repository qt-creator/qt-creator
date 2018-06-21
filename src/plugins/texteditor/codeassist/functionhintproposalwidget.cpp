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

#include "functionhintproposalwidget.h"
#include "ifunctionhintproposalmodel.h"
#include "codeassistant.h"

#include <utils/algorithm.h>
#include <utils/faketooltip.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QApplication>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QPointer>

namespace TextEditor {

static const int maxSelectedFunctionHints = 20;

class SelectedFunctionHints
{
public:
    void insert(int basePosition, const QString &hintId)
    {
        if (basePosition < 0 || hintId.isEmpty())
            return;

        const int index = indexOf(basePosition);

        // Add new item
        if (index == -1) {
            if (m_items.size() + 1 > maxSelectedFunctionHints)
                m_items.removeLast();
            m_items.prepend(FunctionHintItem(basePosition, hintId));
            return;
        }

        // Update existing item
        m_items[index].hintId = hintId;
    }

    QString hintId(int basePosition) const
    {
        const int index = indexOf(basePosition);
        return index == -1 ? QString() : m_items.at(index).hintId;
    }

private:
    int indexOf(int basePosition) const
    {
        return Utils::indexOf(m_items, [&](const FunctionHintItem &item) {
            return item.basePosition == basePosition;
        });
    }

private:
    struct FunctionHintItem {
        FunctionHintItem(int basePosition, const QString &hintId)
            : basePosition(basePosition), hintId(hintId) {}

        int basePosition = -1;
        QString hintId;
    };

    QList<FunctionHintItem> m_items;
};

// -------------------------
// HintProposalWidgetPrivate
// -------------------------
struct FunctionHintProposalWidgetPrivate
{
    FunctionHintProposalWidgetPrivate();

    const QWidget *m_underlyingWidget = nullptr;
    CodeAssistant *m_assistant = nullptr;
    FunctionHintProposalModelPtr m_model;
    QPointer<Utils::FakeToolTip> m_popupFrame; // guard WA_DeleteOnClose widget
    QLabel *m_numberLabel = nullptr;
    QLabel *m_hintLabel = nullptr;
    QWidget *m_pager = nullptr;
    QRect m_displayRect;
    int m_currentHint = -1;
    int m_totalHints = 0;
    int m_currentArgument = -1;
    bool m_escapePressed = false;
};

FunctionHintProposalWidgetPrivate::FunctionHintProposalWidgetPrivate()
    : m_popupFrame(new Utils::FakeToolTip)
    , m_numberLabel(new QLabel)
    , m_hintLabel(new QLabel)
    , m_pager(new QWidget)
{
    m_hintLabel->setTextFormat(Qt::RichText);
}

// ------------------
// HintProposalWidget
// ------------------
FunctionHintProposalWidget::FunctionHintProposalWidget()
    : d(new FunctionHintProposalWidgetPrivate)
{
    auto downArrow = new QToolButton;
    downArrow->setArrowType(Qt::DownArrow);
    downArrow->setFixedSize(16, 16);
    downArrow->setAutoRaise(true);

    auto upArrow = new QToolButton;
    upArrow->setArrowType(Qt::UpArrow);
    upArrow->setFixedSize(16, 16);
    upArrow->setAutoRaise(true);

    auto pagerLayout = new QHBoxLayout(d->m_pager);
    pagerLayout->setMargin(0);
    pagerLayout->setSpacing(0);
    pagerLayout->addWidget(upArrow);
    pagerLayout->addWidget(d->m_numberLabel);
    pagerLayout->addWidget(downArrow);

    auto popupLayout = new QHBoxLayout(d->m_popupFrame);
    popupLayout->setMargin(0);
    popupLayout->setSpacing(0);
    popupLayout->addWidget(d->m_pager);
    popupLayout->addWidget(d->m_hintLabel);

    connect(upArrow, &QAbstractButton::clicked, this, &FunctionHintProposalWidget::previousPage);
    connect(downArrow, &QAbstractButton::clicked, this, &FunctionHintProposalWidget::nextPage);
    connect(d->m_popupFrame.data(), &QObject::destroyed, this, &FunctionHintProposalWidget::abort);

    setFocusPolicy(Qt::NoFocus);
}

FunctionHintProposalWidget::~FunctionHintProposalWidget()
{
    delete d;
}

void FunctionHintProposalWidget::setAssistant(CodeAssistant *assistant)
{
    d->m_assistant = assistant;
}

void FunctionHintProposalWidget::setReason(AssistReason)
{}

void FunctionHintProposalWidget::setKind(AssistKind)
{}

void FunctionHintProposalWidget::setUnderlyingWidget(const QWidget *underlyingWidget)
{
    d->m_underlyingWidget = underlyingWidget;
}

void FunctionHintProposalWidget::setModel(ProposalModelPtr model)
{
    d->m_model = model.staticCast<IFunctionHintProposalModel>();
}

void FunctionHintProposalWidget::setDisplayRect(const QRect &rect)
{
    d->m_displayRect = rect;
}

void FunctionHintProposalWidget::setIsSynchronized(bool)
{}

void FunctionHintProposalWidget::showProposal(const QString &prefix)
{
    QTC_ASSERT(d->m_model && d->m_assistant, abort(); return; );

    d->m_totalHints = d->m_model->size();
    QTC_ASSERT(d->m_totalHints != 0, abort(); return; );

    d->m_pager->setVisible(d->m_totalHints > 1);
    d->m_currentHint = loadSelectedHint();
    if (!updateAndCheck(prefix))
        return;

    qApp->installEventFilter(this);
    d->m_popupFrame->show();
}

void FunctionHintProposalWidget::updateProposal(const QString &prefix)
{
    updateAndCheck(prefix);
}

void FunctionHintProposalWidget::closeProposal()
{
    abort();
}

void FunctionHintProposalWidget::abort()
{
    qApp->removeEventFilter(this);
    if (d->m_popupFrame->isVisible())
        d->m_popupFrame->close();
    deleteLater();
}

static SelectedFunctionHints selectedFunctionHints(CodeAssistant &codeAssistant)
{
    const QVariant variant = codeAssistant.userData();
    return variant.value<SelectedFunctionHints>();
}

int FunctionHintProposalWidget::loadSelectedHint() const
{
    const QString hintId = selectedFunctionHints(*d->m_assistant).hintId(basePosition());

    for (int i = 0; i < d->m_model->size(); ++i) {
        if (d->m_model->id(i) == hintId)
            return i;
    }

    return 0;
}

void FunctionHintProposalWidget::storeSelectedHint()
{
    SelectedFunctionHints table = selectedFunctionHints(*d->m_assistant);
    table.insert(basePosition(), d->m_model->id(d->m_currentHint));

    d->m_assistant->setUserData(QVariant::fromValue(table));
}

bool FunctionHintProposalWidget::eventFilter(QObject *obj, QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            d->m_escapePressed = true;
            e->accept();
        }
        break;
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            d->m_escapePressed = true;
            e->accept();
        }
        QTC_CHECK(d->m_model);
        if (d->m_model && d->m_model->size() > 1) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->key() == Qt::Key_Up) {
                previousPage();
                return true;
            } else if (ke->key() == Qt::Key_Down) {
                nextPage();
                return true;
            }
            return false;
        }
        break;
    case QEvent::KeyRelease: {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->key() == Qt::Key_Escape && d->m_escapePressed) {
                abort();
                emit explicitlyAborted();
                return false;
            } else if (ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down) {
                QTC_CHECK(d->m_model);
                if (d->m_model && d->m_model->size() > 1)
                    return false;
            }
            if (QTC_GUARD(d->m_assistant))
                d->m_assistant->notifyChange();
        }
        break;
    case QEvent::WindowDeactivate:
    case QEvent::FocusOut:
        if (obj != d->m_underlyingWidget)
            break;
        abort();
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
        if (QWidget *widget = qobject_cast<QWidget *>(obj)) {
            if (d->m_popupFrame && !d->m_popupFrame->isAncestorOf(widget)) {
                abort();
            } else if (e->type() == QEvent::Wheel) {
                if (static_cast<QWheelEvent*>(e)->delta() > 0)
                    previousPage();
                else
                    nextPage();
                return true;
            }
        }
        break;
    default:
        break;
    }
    return false;
}

void FunctionHintProposalWidget::nextPage()
{
    d->m_currentHint = (d->m_currentHint + 1) % d->m_totalHints;

    storeSelectedHint();
    updateContent();
}

void FunctionHintProposalWidget::previousPage()
{
    if (d->m_currentHint == 0)
        d->m_currentHint = d->m_totalHints - 1;
    else
        --d->m_currentHint;

    storeSelectedHint();
    updateContent();
}

bool FunctionHintProposalWidget::updateAndCheck(const QString &prefix)
{
    const int activeArgument = d->m_model->activeArgument(prefix);
    if (activeArgument == -1) {
        abort();
        return false;
    } else if (activeArgument != d->m_currentArgument) {
        d->m_currentArgument = activeArgument;
        updateContent();
    }

    return true;
}

void FunctionHintProposalWidget::updateContent()
{
    d->m_hintLabel->setText(d->m_model->text(d->m_currentHint));
    d->m_numberLabel->setText(tr("%1 of %2").arg(d->m_currentHint + 1).arg(d->m_totalHints));
    updatePosition();
}

void FunctionHintProposalWidget::updatePosition()
{
    const QDesktopWidget *desktop = QApplication::desktop();
    const QRect &screen = Utils::HostOsInfo::isMacHost()
            ? desktop->availableGeometry(desktop->screenNumber(d->m_underlyingWidget))
            : desktop->screenGeometry(desktop->screenNumber(d->m_underlyingWidget));

    d->m_pager->setFixedWidth(d->m_pager->minimumSizeHint().width());

    d->m_hintLabel->setWordWrap(false);
    const int maxDesiredWidth = screen.width() - 10;
    const QSize &minHint = d->m_popupFrame->minimumSizeHint();
    if (minHint.width() > maxDesiredWidth) {
        d->m_hintLabel->setWordWrap(true);
        d->m_popupFrame->setFixedWidth(maxDesiredWidth);
        const int extra = d->m_popupFrame->contentsMargins().bottom() +
            d->m_popupFrame->contentsMargins().top();
        d->m_popupFrame->setFixedHeight(
            d->m_hintLabel->heightForWidth(maxDesiredWidth - d->m_pager->width()) + extra);
    } else {
        d->m_popupFrame->setFixedSize(minHint);
    }

    const QSize &sz = d->m_popupFrame->size();
    QPoint pos = d->m_displayRect.topLeft();
    pos.setY(pos.y() - sz.height() - 1);
    if (pos.x() + sz.width() > screen.right())
        pos.setX(screen.right() - sz.width());
    d->m_popupFrame->move(pos);
}

} // TextEditor

Q_DECLARE_METATYPE(TextEditor::SelectedFunctionHints)
