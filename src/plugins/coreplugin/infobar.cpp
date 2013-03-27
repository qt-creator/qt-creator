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

#include "infobar.h"

#include "coreconstants.h"
#include "icore.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

static const char C_SUPPRESSED_WARNINGS[] = "SuppressedWarnings";

namespace Core {

QSet<Id> InfoBar::globallySuppressed;

InfoBarEntry::InfoBarEntry(Id _id, const QString &_infoText, GlobalSuppressionMode _globalSuppression)
    : id(_id)
    , infoText(_infoText)
    , object(0)
    , buttonPressMember(0)
    , cancelObject(0)
    , cancelButtonPressMember(0)
    , globalSuppression(_globalSuppression)
{
}

void InfoBarEntry::setCustomButtonInfo(const QString &_buttonText, QObject *_object, const char *_member)
{
    buttonText = _buttonText;
    object = _object;
    buttonPressMember = _member;
}

void InfoBarEntry::setCancelButtonInfo(QObject *_object, const char *_member)
{
    cancelObject = _object;
    cancelButtonPressMember = _member;
}

void InfoBarEntry::setCancelButtonInfo(const QString &_cancelButtonText, QObject *_object, const char *_member)
{
    cancelButtonText = _cancelButtonText;
    cancelObject = _object;
    cancelButtonPressMember = _member;
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
        connect(infoBar, SIGNAL(changed()), SLOT(update()));
        connect(infoBar, SIGNAL(destroyed()), SLOT(infoBarDestroyed()));
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

        QPalette pal = infoWidget->palette();
        pal.setColor(QPalette::Window, QColor(255, 255, 225));
        pal.setColor(QPalette::WindowText, Qt::black);

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
            connect(infoWidgetButton, SIGNAL(clicked()), info.object, info.buttonPressMember);

            hbox->addWidget(infoWidgetButton);
        }

        QToolButton *infoWidgetSuppressButton = 0;
        if (info.globalSuppression == InfoBarEntry::GlobalSuppressionEnabled) {
            infoWidgetSuppressButton = new QToolButton;
            infoWidgetSuppressButton->setProperty("infoId", info.id.uniqueIdentifier());
            infoWidgetSuppressButton->setText(tr("Do not show again"));
            connect(infoWidgetSuppressButton, SIGNAL(clicked()), SLOT(suppressButtonClicked()));
        }

        QToolButton *infoWidgetCloseButton = new QToolButton;
        infoWidgetCloseButton->setProperty("infoId", info.id.uniqueIdentifier());

        // need to connect to cancelObjectbefore connecting to cancelButtonClicked,
        // because the latter removes the button and with it any connect
        if (info.cancelObject)
            connect(infoWidgetCloseButton, SIGNAL(clicked()),
                    info.cancelObject, info.cancelButtonPressMember);
        connect(infoWidgetCloseButton, SIGNAL(clicked()), SLOT(cancelButtonClicked()));

        if (info.cancelButtonText.isEmpty()) {
            infoWidgetCloseButton->setAutoRaise(true);
            infoWidgetCloseButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_CLEAR)));
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

void InfoBarDisplay::cancelButtonClicked()
{
    m_infoBar->suppressInfo(Id::fromUniqueIdentifier(sender()->property("infoId").toInt()));
}

void InfoBarDisplay::suppressButtonClicked()
{
    Id id = Id::fromUniqueIdentifier(sender()->property("infoId").toInt());
    m_infoBar->removeInfo(id);
    InfoBar::globallySuppressInfo(id);
}

} // namespace Core
