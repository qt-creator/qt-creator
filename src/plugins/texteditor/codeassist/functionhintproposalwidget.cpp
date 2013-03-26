/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "functionhintproposalwidget.h"
#include "ifunctionhintproposalmodel.h"
#include "codeassistant.h"

#include <utils/faketooltip.h>
#include <utils/hostosinfo.h>

#include <QDebug>
#include <QApplication>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QDesktopWidget>
#include <QKeyEvent>

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
    : d(new FunctionHintProposalWidgetPrivate)
{
    QToolButton *downArrow = new QToolButton;
    downArrow->setArrowType(Qt::DownArrow);
    downArrow->setFixedSize(16, 16);
    downArrow->setAutoRaise(true);

    QToolButton *upArrow = new QToolButton;
    upArrow->setArrowType(Qt::UpArrow);
    upArrow->setFixedSize(16, 16);
    upArrow->setAutoRaise(true);

    QHBoxLayout *pagerLayout = new QHBoxLayout(d->m_pager);
    pagerLayout->setMargin(0);
    pagerLayout->setSpacing(0);
    pagerLayout->addWidget(upArrow);
    pagerLayout->addWidget(d->m_numberLabel);
    pagerLayout->addWidget(downArrow);

    QHBoxLayout *popupLayout = new QHBoxLayout(d->m_popupFrame);
    popupLayout->setMargin(0);
    popupLayout->setSpacing(0);
    popupLayout->addWidget(d->m_pager);
    popupLayout->addWidget(d->m_hintLabel);

    connect(upArrow, SIGNAL(clicked()), SLOT(previousPage()));
    connect(downArrow, SIGNAL(clicked()), SLOT(nextPage()));

    qApp->installEventFilter(this);

    setFocusPolicy(Qt::NoFocus);
}

FunctionHintProposalWidget::~FunctionHintProposalWidget()
{
    delete d->m_model;
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

void FunctionHintProposalWidget::setModel(IAssistProposalModel *model)
{
    d->m_model = static_cast<IFunctionHintProposalModel *>(model);
}

void FunctionHintProposalWidget::setDisplayRect(const QRect &rect)
{
    d->m_displayRect = rect;
}

void FunctionHintProposalWidget::setIsSynchronized(bool)
{}

void FunctionHintProposalWidget::showProposal(const QString &prefix)
{
    d->m_totalHints = d->m_model->size();
    if (d->m_totalHints == 0) {
        abort();
        return;
    }
    d->m_pager->setVisible(d->m_totalHints > 1);
    d->m_currentHint = 0;
    if (!updateAndCheck(prefix)) {
        abort();
        return;
    }
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
    if (d->m_popupFrame->isVisible())
        d->m_popupFrame->close();
    deleteLater();
}

bool FunctionHintProposalWidget::eventFilter(QObject *obj, QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape)
            d->m_escapePressed = true;
        break;
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape)
            d->m_escapePressed = true;
        if (d->m_model->size() > 1) {
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
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape && d->m_escapePressed) {
            abort();
            return false;
        }
        d->m_assistant->notifyChange();
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
    case QEvent::Wheel: {
            QWidget *widget = qobject_cast<QWidget *>(obj);
            if (!d->m_popupFrame->isAncestorOf(widget)) {
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
    updateContent();
}

void FunctionHintProposalWidget::previousPage()
{
    if (d->m_currentHint == 0)
        d->m_currentHint = d->m_totalHints - 1;
    else
        --d->m_currentHint;
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
