// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "infobar.h"

#include "algorithm.h"
#include "qtcassert.h"
#include "qtcsettings.h"
#include "utilsicons.h"
#include "utilstr.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

static const char C_SUPPRESSED_WARNINGS[] = "SuppressedWarnings";

namespace Utils {

QSet<Id> InfoBar::globallySuppressed;
QtcSettings *InfoBar::m_settings = nullptr;

class InfoBarWidget : public QWidget
{
public:
    InfoBarWidget(Qt::Edge edge, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    const Qt::Edge m_edge;
};

InfoBarWidget::InfoBarWidget(Qt::Edge edge, QWidget *parent)
    : QWidget(parent)
    , m_edge(edge)
{
    const bool topEdge = m_edge == Qt::TopEdge;
    setContentsMargins(2, topEdge ? 0 : 1, 0, topEdge ? 1 : 0);
}

void InfoBarWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter p(this);
    p.fillRect(rect(), creatorTheme()->color(Theme::InfoBarBackground));
    const QRectF adjustedRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const bool topEdge = m_edge == Qt::TopEdge;
    p.setPen(creatorTheme()->color(Theme::FancyToolBarSeparatorColor));
    p.drawLine(QLineF(topEdge ? adjustedRect.bottomLeft() : adjustedRect.topLeft(),
                      topEdge ? adjustedRect.bottomRight() : adjustedRect.topRight()));
}

InfoBarEntry::InfoBarEntry(Id _id, const QString &_infoText, GlobalSuppression _globalSuppression)
    : m_id(_id)
    , m_infoText(_infoText)
    , m_globalSuppression(_globalSuppression)
{
}

Id InfoBarEntry::id() const
{
    return m_id;
}

QString InfoBarEntry::text() const
{
    return m_infoText;
}

void InfoBarEntry::addCustomButton(const QString &buttonText,
                                   CallBack callBack,
                                   const QString &tooltip)
{
    m_buttons.append({buttonText, callBack, tooltip});
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

void InfoBarEntry::setComboInfo(const QStringList &list,
                                ComboCallBack callBack,
                                const QString &tooltip,
                                int currentIndex)
{
    auto comboInfos = transform(list, [](const QString &string) {
        return ComboInfo{string, string};
    });
    setComboInfo(comboInfos, callBack, tooltip, currentIndex);
}

void InfoBarEntry::setComboInfo(const QList<ComboInfo> &list,
                                ComboCallBack callBack,
                                const QString &tooltip,
                                int currentIndex)
{
    m_combo.entries = list;
    m_combo.callback = callBack;
    m_combo.tooltip = tooltip;
    m_combo.currentIndex = currentIndex;
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
    Utils::erase(m_infoBarEntries, equal(&InfoBarEntry::m_id, id));
    if (size != m_infoBarEntries.size())
        emit changed();
}

bool InfoBar::containsInfo(Id id) const
{
    return anyOf(m_infoBarEntries, equal(&InfoBarEntry::m_id, id));
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

void InfoBar::initialize(QtcSettings *settings)
{
    m_settings = settings;

    if (QTC_GUARD(m_settings)) {
        const QStringList list = m_settings->value(C_SUPPRESSED_WARNINGS).toStringList();
        globallySuppressed = transform<QSet>(list, Id::fromString);
    }
}

QtcSettings *InfoBar::settings()
{
    return m_settings;
}

void InfoBar::clearGloballySuppressed()
{
    globallySuppressed.clear();
    if (m_settings)
        m_settings->remove(C_SUPPRESSED_WARNINGS);
}

bool InfoBar::anyGloballySuppressed()
{
    return !globallySuppressed.isEmpty();
}

void InfoBar::writeGloballySuppressedToSettings()
{
    if (!m_settings)
        return;
    const QStringList list = transform<QList>(globallySuppressed, &Id::toString);
    m_settings->setValueWithDefault(C_SUPPRESSED_WARNINGS, list);
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

void InfoBarDisplay::setEdge(Qt::Edge edge)
{
    m_edge = edge;
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
    for (QWidget *widget : std::as_const(m_infoWidgets)) {
        widget->disconnect(this); // We want no destroyed() signal now
        widget->hide(); // Late deletion can cause duplicate infos. Hide immediately to prevent it.
        widget->deleteLater();
    }
    m_infoWidgets.clear();

    if (!m_infoBar)
        return;

    for (const InfoBarEntry &info : std::as_const(m_infoBar->m_infoBarEntries)) {
        auto infoWidget = new InfoBarWidget(m_edge);

        auto hbox = new QHBoxLayout;
        hbox->setContentsMargins(2, 2, 2, 2);

        auto vbox = new QVBoxLayout(infoWidget);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->addLayout(hbox);

        QLabel *infoWidgetLabel = new QLabel(info.m_infoText);
        infoWidgetLabel->setWordWrap(true);
        infoWidgetLabel->setOpenExternalLinks(true);
        hbox->addWidget(infoWidgetLabel, 1);

        if (info.m_detailsWidgetCreator) {
            if (m_isShowingDetailsWidget) {
                QWidget *detailsWidget = info.m_detailsWidgetCreator();
                vbox->addWidget(detailsWidget);
            }

            auto showDetailsButton = new QToolButton;
            showDetailsButton->setCheckable(true);
            showDetailsButton->setChecked(m_isShowingDetailsWidget);
            showDetailsButton->setText(Tr::tr("&Show Details"));
            connect(showDetailsButton, &QToolButton::clicked, this, [this, vbox, info] (bool) {
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

        if (!info.m_combo.entries.isEmpty()) {
            auto cb = new QComboBox();
            cb->setToolTip(info.m_combo.tooltip);
            for (const InfoBarEntry::ComboInfo &comboInfo : std::as_const(info.m_combo.entries))
                cb->addItem(comboInfo.displayText, comboInfo.data);
            if (info.m_combo.currentIndex >= 0 && info.m_combo.currentIndex < cb->count())
                cb->setCurrentIndex(info.m_combo.currentIndex);
            connect(cb, &QComboBox::currentIndexChanged, this, [cb, info] {
                info.m_combo.callback({cb->currentText(), cb->currentData()});
            }, Qt::QueuedConnection);

            hbox->addWidget(cb);
        }

        for (const InfoBarEntry::Button &button : std::as_const(info.m_buttons)) {
            auto infoWidgetButton = new QToolButton;
            infoWidgetButton->setText(button.text);
            infoWidgetButton->setToolTip(button.tooltip);
            connect(infoWidgetButton, &QAbstractButton::clicked, [button] { button.callback(); });
            hbox->addWidget(infoWidgetButton);
        }

        const Id id = info.m_id;
        QToolButton *infoWidgetSuppressButton = nullptr;
        if (info.m_globalSuppression == InfoBarEntry::GlobalSuppression::Enabled) {
            infoWidgetSuppressButton = new QToolButton;
            infoWidgetSuppressButton->setText(Tr::tr("Do Not Show Again"));
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
                m_infoBar->removeInfo(id);
            });
        }

        if (infoWidgetCloseButton) {
            if (info.m_cancelButtonText.isEmpty()) {
                infoWidgetCloseButton->setAutoRaise(true);
                infoWidgetCloseButton->setIcon(Icons::CLOSE_FOREGROUND.icon());
                infoWidgetCloseButton->setToolTip(Tr::tr("Close"));
            } else {
                infoWidgetCloseButton->setText(info.m_cancelButtonText);
            }
        }

        if (infoWidgetSuppressButton)
            hbox->addWidget(infoWidgetSuppressButton);

        if (infoWidgetCloseButton)
            hbox->addWidget(infoWidgetCloseButton);

        connect(infoWidget, &QObject::destroyed, this, [this, infoWidget] {
            m_infoWidgets.removeOne(infoWidget);
        });
        m_boxLayout->insertWidget(m_boxIndex, infoWidget);
        m_infoWidgets << infoWidget;
    }
}

} // namespace Utils
