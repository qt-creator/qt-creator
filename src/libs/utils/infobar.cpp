/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "infobar.h"

#include "algorithm.h"
#include "qtcassert.h"
#include "theme/theme.h"
#include "utilsicons.h"

#include <QHBoxLayout>
#include <QSettings>
#include <QVBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QComboBox>

static const char C_SUPPRESSED_WARNINGS[] = "SuppressedWarnings";

namespace Utils {

QSet<Id> InfoBar::globallySuppressed;
QSettings *InfoBar::m_settings = nullptr;
Utils::Theme *InfoBar::m_theme = nullptr;

InfoBarEntry::InfoBarEntry(Id _id, const QString &_infoText, GlobalSuppression _globalSuppression)
    : m_id(_id)
    , m_infoText(_infoText)
    , m_globalSuppression(_globalSuppression)
{
}

void InfoBarEntry::setCustomButtonInfo(const QString &_buttonText, CallBack callBack)
{
    m_buttonText = _buttonText;
    m_buttonCallBack = callBack;
}

void InfoBarEntry::setCancelButtonInfo(CallBack callBack)
{
    m_useCancelButton = true;
    m_cancelButtonCallBack = callBack;
}

void InfoBarEntry::setCancelButtonInfo(const QString &_cancelButtonText, CallBack callBack)
{
    m_useCancelButton = true;
    m_cancelButtonText = _cancelButtonText;
    m_cancelButtonCallBack = callBack;
}

void InfoBarEntry::setComboInfo(const QStringList &list, InfoBarEntry::ComboCallBack callBack)
{
    m_comboCallBack = callBack;
    m_comboInfo = list;
}

void InfoBarEntry::removeCancelButton()
{
    m_useCancelButton = false;
    m_cancelButtonText.clear();
    m_cancelButtonCallBack = nullptr;
}

void InfoBarEntry::setDetailsWidgetCreator(const InfoBarEntry::DetailsWidgetCreator &creator)
{
    m_detailsWidgetCreator = creator;
}

void InfoBar::addInfo(const InfoBarEntry &info)
{
    m_infoBarEntries << info;
    emit changed();
}

void InfoBar::removeInfo(Id id)
{
    const int size = m_infoBarEntries.size();
    Utils::erase(m_infoBarEntries, Utils::equal(&InfoBarEntry::m_id, id));
    if (size != m_infoBarEntries.size())
        emit changed();
}

bool InfoBar::containsInfo(Id id) const
{
    return Utils::anyOf(m_infoBarEntries, Utils::equal(&InfoBarEntry::m_id, id));
}

// Remove and suppress id
void InfoBar::suppressInfo(Id id)
{
    removeInfo(id);
    m_suppressed << id;
}

// Info cannot be added more than once, or if it is suppressed
bool InfoBar::canInfoBeAdded(Id id) const
{
    return !containsInfo(id) && !m_suppressed.contains(id) && !globallySuppressed.contains(id);
}

void InfoBar::unsuppressInfo(Id id)
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
    writeGloballySuppressedToSettings();
}

void InfoBar::globallyUnsuppressInfo(Id id)
{
    globallySuppressed.remove(id);
    writeGloballySuppressedToSettings();
}

void InfoBar::initialize(QSettings *settings, Theme *theme)
{
    m_settings = settings;
    m_theme = theme;

    if (QTC_GUARD(m_settings)) {
        const QStringList list = m_settings->value(QLatin1String(C_SUPPRESSED_WARNINGS)).toStringList();
        globallySuppressed = Utils::transform<QSet>(list, Id::fromString);
    }
}

void InfoBar::clearGloballySuppressed()
{
    globallySuppressed.clear();
    if (m_settings)
        m_settings->setValue(QLatin1String(C_SUPPRESSED_WARNINGS), QStringList());
}

bool InfoBar::anyGloballySuppressed()
{
    return !globallySuppressed.isEmpty();
}

void InfoBar::writeGloballySuppressedToSettings()
{
    if (!m_settings)
        return;
    const QStringList list = Utils::transform<QList>(globallySuppressed, &Id::toString);
    m_settings->setValue(QLatin1String(C_SUPPRESSED_WARNINGS), list);
}


InfoBarDisplay::InfoBarDisplay(QObject *parent)
    : QObject(parent)
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
        connect(m_infoBar, &InfoBar::changed, this, &InfoBarDisplay::update);
        connect(m_infoBar, &QObject::destroyed, this, &InfoBarDisplay::infoBarDestroyed);
    }
    update();
}

void InfoBarDisplay::setStyle(QFrame::Shadow style)
{
    m_style = style;
    update();
}

InfoBar *InfoBarDisplay::infoBar() const
{
    return m_infoBar;
}

void InfoBarDisplay::infoBarDestroyed()
{
    m_infoBar = nullptr;
    // Calling update() here causes a complicated crash on shutdown.
    // So instead we rely on the view now being either destroyed (in which case it
    // will delete the widgets itself) or setInfoBar() being called explicitly.
}

void InfoBarDisplay::update()
{
    for (QWidget *widget : m_infoWidgets) {
        widget->disconnect(this); // We want no destroyed() signal now
        delete widget;
    }
    m_infoWidgets.clear();

    if (!m_infoBar)
        return;

    for (const InfoBarEntry &info : m_infoBar->m_infoBarEntries) {
        QFrame *infoWidget = new QFrame;

        QPalette pal;
        if (QTC_GUARD(InfoBar::m_theme)) {
            pal.setColor(QPalette::Window, InfoBar::m_theme->color(Theme::InfoBarBackground));
            pal.setColor(QPalette::WindowText, InfoBar::m_theme->color(Theme::InfoBarText));
        }

        infoWidget->setPalette(pal);
        infoWidget->setFrameStyle(QFrame::Panel | m_style);
        infoWidget->setLineWidth(1);
        infoWidget->setAutoFillBackground(true);

        auto hbox = new QHBoxLayout;
        hbox->setContentsMargins(2, 2, 2, 2);

        auto vbox = new QVBoxLayout(infoWidget);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->addLayout(hbox);

        QLabel *infoWidgetLabel = new QLabel(info.m_infoText);
        infoWidgetLabel->setWordWrap(true);
        hbox->addWidget(infoWidgetLabel, 1);

        if (info.m_detailsWidgetCreator) {
            if (m_isShowingDetailsWidget) {
                QWidget *detailsWidget = info.m_detailsWidgetCreator();
                vbox->addWidget(detailsWidget);
            }

            auto showDetailsButton = new QToolButton;
            showDetailsButton->setCheckable(true);
            showDetailsButton->setChecked(m_isShowingDetailsWidget);
            showDetailsButton->setText(tr("&Show Details"));
            connect(showDetailsButton, &QToolButton::clicked, [this, vbox, info] (bool) {
                QWidget *detailsWidget = vbox->count() == 2 ? vbox->itemAt(1)->widget() : nullptr;
                if (!detailsWidget) {
                    detailsWidget = info.m_detailsWidgetCreator();
                    vbox->addWidget(detailsWidget);
                }

                m_isShowingDetailsWidget = !m_isShowingDetailsWidget;
                detailsWidget->setVisible(m_isShowingDetailsWidget);
            });

            hbox->addWidget(showDetailsButton);
        } else {
            m_isShowingDetailsWidget = false;
        }

        if (!info.m_comboInfo.isEmpty()) {
            auto cb = new QComboBox();
            cb->addItems(info.m_comboInfo);
            connect(cb, &QComboBox::currentTextChanged, [info](const QString &text) {
                info.m_comboCallBack(text);
            });

            hbox->addWidget(cb);
        }

        if (!info.m_buttonText.isEmpty()) {
            auto infoWidgetButton = new QToolButton;
            infoWidgetButton->setText(info.m_buttonText);
            connect(infoWidgetButton, &QAbstractButton::clicked, [info]() { info.m_buttonCallBack(); });

            hbox->addWidget(infoWidgetButton);
        }

        const Id id = info.m_id;
        QToolButton *infoWidgetSuppressButton = nullptr;
        if (info.m_globalSuppression == InfoBarEntry::GlobalSuppression::Enabled) {
            infoWidgetSuppressButton = new QToolButton;
            infoWidgetSuppressButton->setText(tr("Do Not Show Again"));
            connect(infoWidgetSuppressButton, &QAbstractButton::clicked, this, [this, id] {
                m_infoBar->removeInfo(id);
                InfoBar::globallySuppressInfo(id);
            });
        }

        QToolButton *infoWidgetCloseButton = nullptr;
        if (info.m_useCancelButton) {
            infoWidgetCloseButton = new QToolButton;
            // need to connect to cancelObjectbefore connecting to cancelButtonClicked,
            // because the latter removes the button and with it any connect
            if (info.m_cancelButtonCallBack)
                connect(infoWidgetCloseButton, &QAbstractButton::clicked, info.m_cancelButtonCallBack);
            connect(infoWidgetCloseButton, &QAbstractButton::clicked, this, [this, id] {
                m_infoBar->suppressInfo(id);
            });
        }

        if (info.m_cancelButtonText.isEmpty()) {
            if (infoWidgetCloseButton) {
                infoWidgetCloseButton->setAutoRaise(true);
                infoWidgetCloseButton->setIcon(Utils::Icons::CLOSE_FOREGROUND.icon());
                infoWidgetCloseButton->setToolTip(tr("Close"));
            }

            if (infoWidgetSuppressButton)
                hbox->addWidget(infoWidgetSuppressButton);

            if (infoWidgetCloseButton)
                hbox->addWidget(infoWidgetCloseButton);
        } else {
            infoWidgetCloseButton->setText(info.m_cancelButtonText);
            hbox->addWidget(infoWidgetCloseButton);
            if (infoWidgetSuppressButton)
                hbox->addWidget(infoWidgetSuppressButton);
        }

        connect(infoWidget, &QObject::destroyed, this, &InfoBarDisplay::widgetDestroyed);
        m_boxLayout->insertWidget(m_boxIndex, infoWidget);
        m_infoWidgets << infoWidget;
    }
}

void InfoBarDisplay::widgetDestroyed()
{
    m_infoWidgets.removeOne(static_cast<QWidget *>(sender()));
}

} // namespace Utils
