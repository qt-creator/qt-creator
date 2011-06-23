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

#include "functionhintproposalwidget.h"
#include "ifunctionhintproposalmodel.h"
#include "codeassistant.h"

#include <utils/faketooltip.h>

#include <QtCore/QDebug>
#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QDesktopWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QShortcutEvent>

namespace TextEditor {

// -------------------------
// HintProposalWidgetPrivate
// -------------------------
struct FunctionHintProposalWidgetPrivate
{
    FunctionHintProposalWidgetPrivate();

    const QWidget *m_underlyingWidget;
    CodeAssistant *m_assistant;
    IFunctionHintProposalModel *m_model;
    Utils::FakeToolTip *m_popupFrame;
    QLabel *m_numberLabel;
    QLabel *m_hintLabel;
    QWidget *m_pager;
    QRect m_displayRect;
    int m_currentHint;
    int m_totalHints;
    int m_currentArgument;
    bool m_escapePressed;
};

FunctionHintProposalWidgetPrivate::FunctionHintProposalWidgetPrivate()
    : m_underlyingWidget(0)
    , m_assistant(0)
    , m_model(0)
    , m_popupFrame(new Utils::FakeToolTip)
    , m_numberLabel(new QLabel)
    , m_hintLabel(new QLabel)
    , m_pager(new QWidget)
    , m_currentHint(-1)
    , m_totalHints(0)
    , m_currentArgument(-1)
    , m_escapePressed(false)
{
    m_hintLabel->setTextFormat(Qt::RichText);
}

// ------------------
// HintProposalWidget
// ------------------
FunctionHintProposalWidget::FunctionHintProposalWidget()
    : m_d(new FunctionHintProposalWidgetPrivate)
{
    QToolButton *downArrow = new QToolButton;
    downArrow->setArrowType(Qt::DownArrow);
    downArrow->setFixedSize(16, 16);
    downArrow->setAutoRaise(true);

    QToolButton *upArrow = new QToolButton;
    upArrow->setArrowType(Qt::UpArrow);
    upArrow->setFixedSize(16, 16);
    upArrow->setAutoRaise(true);

    QHBoxLayout *pagerLayout = new QHBoxLayout(m_d->m_pager);
    pagerLayout->setMargin(0);
    pagerLayout->setSpacing(0);
    pagerLayout->addWidget(upArrow);
    pagerLayout->addWidget(m_d->m_numberLabel);
    pagerLayout->addWidget(downArrow);

    QHBoxLayout *popupLayout = new QHBoxLayout(m_d->m_popupFrame);
    popupLayout->setMargin(0);
    popupLayout->setSpacing(0);
    popupLayout->addWidget(m_d->m_pager);
    popupLayout->addWidget(m_d->m_hintLabel);

    connect(upArrow, SIGNAL(clicked()), SLOT(previousPage()));
    connect(downArrow, SIGNAL(clicked()), SLOT(nextPage()));

    qApp->installEventFilter(this);

    setFocusPolicy(Qt::NoFocus);
}

FunctionHintProposalWidget::~FunctionHintProposalWidget()
{
    delete m_d->m_model;
}

void FunctionHintProposalWidget::setAssistant(CodeAssistant *assistant)
{
    m_d->m_assistant = assistant;
}

void FunctionHintProposalWidget::setReason(AssistReason)
{}

void FunctionHintProposalWidget::setKind(AssistKind)
{}

void FunctionHintProposalWidget::setUnderlyingWidget(const QWidget *underlyingWidget)
{
    m_d->m_underlyingWidget = underlyingWidget;
}

void FunctionHintProposalWidget::setModel(IAssistProposalModel *model)
{
    m_d->m_model = static_cast<IFunctionHintProposalModel *>(model);
}

void FunctionHintProposalWidget::setDisplayRect(const QRect &rect)
{
    m_d->m_displayRect = rect;
}

void FunctionHintProposalWidget::setIsSynchronized(bool)
{}

void FunctionHintProposalWidget::showProposal(const QString &prefix)
{
    m_d->m_totalHints = m_d->m_model->size();
    if (m_d->m_totalHints == 0) {
        abort();
        return;
    }
    m_d->m_pager->setVisible(m_d->m_totalHints > 1);
    m_d->m_currentHint = 0;
    if (!updateAndCheck(prefix)) {
        abort();
        return;
    }
    m_d->m_popupFrame->show();
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
    if (m_d->m_popupFrame->isVisible())
        m_d->m_popupFrame->close();
    deleteLater();
}

bool FunctionHintProposalWidget::eventFilter(QObject *obj, QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            m_d->m_escapePressed = true;
        }
        break;
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            m_d->m_escapePressed = true;
        }
        if (m_d->m_model->size() > 1) {
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
    case QEvent::KeyRelease:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape && m_d->m_escapePressed) {
            abort();
            return false;
        }
        m_d->m_assistant->notifyChange();
        break;
    case QEvent::WindowDeactivate:
    case QEvent::FocusOut:
        if (obj != m_d->m_underlyingWidget) {
            break;
        }
        abort();
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel: {
            QWidget *widget = qobject_cast<QWidget *>(obj);
            if (! (widget == this || isAncestorOf(widget))) {
                abort();
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
    m_d->m_currentHint = (m_d->m_currentHint + 1) % m_d->m_totalHints;
    updateContent();
}

void FunctionHintProposalWidget::previousPage()
{
    if (m_d->m_currentHint == 0)
        m_d->m_currentHint = m_d->m_totalHints - 1;
    else
        --m_d->m_currentHint;
    updateContent();
}

bool FunctionHintProposalWidget::updateAndCheck(const QString &prefix)
{
    const int activeArgument = m_d->m_model->activeArgument(prefix);
    if (activeArgument == -1) {
        abort();
        return false;
    } else if (activeArgument != m_d->m_currentArgument) {
        m_d->m_currentArgument = activeArgument;
        updateContent();
    }

    return true;
}

void FunctionHintProposalWidget::updateContent()
{
    m_d->m_hintLabel->setText(m_d->m_model->text(m_d->m_currentHint));
    m_d->m_numberLabel->setText(tr("%1 of %2").arg(m_d->m_currentHint + 1).arg(m_d->m_totalHints));
    updatePosition();
}

void FunctionHintProposalWidget::updatePosition()
{
    const QDesktopWidget *desktop = QApplication::desktop();
#ifdef Q_WS_MAC
    const QRect &screen = desktop->availableGeometry(desktop->screenNumber(m_d->m_underlyingWidget));
#else
    const QRect &screen = desktop->screenGeometry(desktop->screenNumber(m_d->m_underlyingWidget));
#endif

    m_d->m_pager->setFixedWidth(m_d->m_pager->minimumSizeHint().width());

    m_d->m_hintLabel->setWordWrap(false);
    const int maxDesiredWidth = screen.width() - 10;
    const QSize &minHint = m_d->m_popupFrame->minimumSizeHint();
    if (minHint.width() > maxDesiredWidth) {
        m_d->m_hintLabel->setWordWrap(true);
        m_d->m_popupFrame->setFixedWidth(maxDesiredWidth);
        const int extra = m_d->m_popupFrame->contentsMargins().bottom() +
            m_d->m_popupFrame->contentsMargins().top();
        m_d->m_popupFrame->setFixedHeight(
            m_d->m_hintLabel->heightForWidth(maxDesiredWidth - m_d->m_pager->width()) + extra);
    } else {
        m_d->m_popupFrame->setFixedSize(minHint);
    }

    const QSize &sz = m_d->m_popupFrame->size();
    QPoint pos = m_d->m_displayRect.topLeft();
    pos.setY(pos.y() - sz.height() - 1);
    if (pos.x() + sz.width() > screen.right())
        pos.setX(screen.right() - sz.width());
    m_d->m_popupFrame->move(pos);
}

} // TextEditor
