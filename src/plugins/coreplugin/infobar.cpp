/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "infobar.h"

#include "coreicons.h"
#include "icore.h"

#include <utils/theme/theme.h>

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

static const char C_SUPPRESSED_WARNINGS[] = "SuppressedWarnings";

using namespace Utils;

namespace Core {

QSet<Id> InfoBar::globallySuppressed;

InfoBarEntry::InfoBarEntry(Id _id, const QString &_infoText, GlobalSuppressionMode _globalSuppression)
    : id(_id)
    , infoText(_infoText)
    , globalSuppression(_globalSuppression)
{
}

void InfoBarEntry::setCustomButtonInfo(const QString &_buttonText, CallBack callBack)
{
    buttonText = _buttonText;
    m_buttonCallBack = callBack;
}

void InfoBarEntry::setCancelButtonInfo(CallBack callBack)
{
    m_cancelButtonCallBack = callBack;
}

void InfoBarEntry::setCancelButtonInfo(const QString &_cancelButtonText, CallBack callBack)
{
    cancelButtonText = _cancelButtonText;
    m_cancelButtonCallBack = callBack;
}


void InfoBar::addInfo(const InfoBarEntry &info)
{
    m_infoBarEntries << info;
    emit changed();
}

void InfoBar::removeInfo(Id id)
{
    QMutableListIterator<InfoBarEntry> it(m_infoBarEntries);
    while (it.hasNext())
        if (it.next().id == id) {
            it.remove();
            emit changed();
            return;
        }
}

bool InfoBar::containsInfo(Id id) const
{
    QListIterator<InfoBarEntry> it(m_infoBarEntries);
    while (it.hasNext())
        if (it.next().id == id)
            return true;

    return false;
}

// Remove and suppress id
void InfoBar::suppressInfo(Id id)
{
    removeInfo(id);
    m_suppressed << id;
}

// Info can not be added more than once, or if it is suppressed
bool InfoBar::canInfoBeAdded(Id id) const
{
    return !containsInfo(id) && !m_suppressed.contains(id) && !globallySuppressed.contains(id);
}

void InfoBar::enableInfo(Id id)
{
    m_suppressed.remove(id);
}

void InfoBar::clear()
{
    if (!m_infoBarEntries.isEmpty()) {
        m_infoBarEntries.clear();
        emit changed();
    }
}

void InfoBar::globallySuppressInfo(Id id)
{
    globallySuppressed.insert(id);
    QStringList list;
    foreach (Id i, globallySuppressed)
        list << QLatin1String(i.name());
    ICore::settings()->setValue(QLatin1String(C_SUPPRESSED_WARNINGS), list);
}

void InfoBar::initializeGloballySuppressed()
{
    QStringList list = ICore::settings()->value(QLatin1String(C_SUPPRESSED_WARNINGS)).toStringList();
    foreach (const QString &id, list)
        globallySuppressed.insert(Id::fromString(id));
}

void InfoBar::clearGloballySuppressed()
{
    globallySuppressed.clear();
    ICore::settings()->setValue(QLatin1String(C_SUPPRESSED_WARNINGS), QStringList());
}

bool InfoBar::anyGloballySuppressed()
{
    return !globallySuppressed.isEmpty();
}


InfoBarDisplay::InfoBarDisplay(QObject *parent)
    : QObject(parent)
    , m_infoBar(0)
    , m_boxLayout(0)
    , m_boxIndex(0)
{
}

void InfoBarDisplay::setTarget(QBoxLayout *layout, int index)
{
    m_boxLayout = layout;
    m_boxIndex = index;
}

void InfoBarDisplay::setInfoBar(InfoBar *infoBar)
{
    if (m_infoBar == infoBar)
        return;

    if (m_infoBar)
        m_infoBar->disconnect(this);
    m_infoBar = infoBar;
    if (m_infoBar) {
        connect(m_infoBar, SIGNAL(changed()), SLOT(update()));
        connect(m_infoBar, SIGNAL(destroyed()), SLOT(infoBarDestroyed()));
    }
    update();
}

void InfoBarDisplay::infoBarDestroyed()
{
    m_infoBar = 0;
    // Calling update() here causes a complicated crash on shutdown.
    // So instead we rely on the view now being either destroyed (in which case it
    // will delete the widgets itself) or setInfoBar() being called explicitly.
}

void InfoBarDisplay::update()
{
    foreach (QWidget *widget, m_infoWidgets) {
        widget->disconnect(this); // We want no destroyed() signal now
        delete widget;
    }
    m_infoWidgets.clear();

    if (!m_infoBar)
        return;

    foreach (const InfoBarEntry &info, m_infoBar->m_infoBarEntries) {
        QFrame *infoWidget = new QFrame;

        QPalette pal;
        pal.setColor(QPalette::Window, creatorTheme()->color(Theme::InfoBarBackground));
        pal.setColor(QPalette::WindowText, creatorTheme()->color(Theme::InfoBarText));

        infoWidget->setPalette(pal);
        infoWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
        infoWidget->setLineWidth(1);
        infoWidget->setAutoFillBackground(true);

        QHBoxLayout *hbox = new QHBoxLayout(infoWidget);
        hbox->setMargin(2);

        QLabel *infoWidgetLabel = new QLabel(info.infoText);
        infoWidgetLabel->setWordWrap(true);
        hbox->addWidget(infoWidgetLabel);

        if (!info.buttonText.isEmpty()) {
            QToolButton *infoWidgetButton = new QToolButton;
            infoWidgetButton->setText(info.buttonText);
            connect(infoWidgetButton, &QAbstractButton::clicked, [info]() { info.m_buttonCallBack(); });

            hbox->addWidget(infoWidgetButton);
        }

        const Id id = info.id;
        QToolButton *infoWidgetSuppressButton = 0;
        if (info.globalSuppression == InfoBarEntry::GlobalSuppressionEnabled) {
            infoWidgetSuppressButton = new QToolButton;
            infoWidgetSuppressButton->setText(tr("Do Not Show Again"));
            connect(infoWidgetSuppressButton, &QAbstractButton::clicked, this, [this, id] {
                m_infoBar->removeInfo(id);
                InfoBar::globallySuppressInfo(id);
            });
        }

        QToolButton *infoWidgetCloseButton = new QToolButton;
        // need to connect to cancelObjectbefore connecting to cancelButtonClicked,
        // because the latter removes the button and with it any connect
        if (info.m_cancelButtonCallBack)
            connect(infoWidgetCloseButton, &QAbstractButton::clicked, info.m_cancelButtonCallBack);
        connect(infoWidgetCloseButton, &QAbstractButton::clicked, this, [this, id] {
            m_infoBar->suppressInfo(id);
        });

        if (info.cancelButtonText.isEmpty()) {
            infoWidgetCloseButton->setAutoRaise(true);
            infoWidgetCloseButton->setIcon(Icons::CLEAR.icon());
            infoWidgetCloseButton->setToolTip(tr("Close"));
            if (infoWidgetSuppressButton)
                hbox->addWidget(infoWidgetSuppressButton);
            hbox->addWidget(infoWidgetCloseButton);
        } else {
            infoWidgetCloseButton->setText(info.cancelButtonText);
            hbox->addWidget(infoWidgetCloseButton);
            if (infoWidgetSuppressButton)
                hbox->addWidget(infoWidgetSuppressButton);
        }

        connect(infoWidget, SIGNAL(destroyed()), SLOT(widgetDestroyed()));
        m_boxLayout->insertWidget(m_boxIndex, infoWidget);
        m_infoWidgets << infoWidget;
    }
}

void InfoBarDisplay::widgetDestroyed()
{
    m_infoWidgets.removeOne(static_cast<QWidget *>(sender()));
}

} // namespace Core
