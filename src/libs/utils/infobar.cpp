// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "infobar.h"

#include "algorithm.h"
#include "infolabel.h"
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
    InfoBarWidget(Qt::Edge edge, InfoLabel::InfoType infoType = InfoLabel::None,
                  QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor backgroundColor() const;
    const Utils::Icon &icon() const;

    const Qt::Edge m_edge;
    const InfoLabel::InfoType m_infoType;
};

InfoBarWidget::InfoBarWidget(Qt::Edge edge, InfoLabel::InfoType infoType, QWidget *parent)
    : QWidget(parent)
    , m_edge(edge)
    , m_infoType(infoType)
{
    const bool topEdge = m_edge == Qt::TopEdge;
    const int leftMargin = m_infoType == InfoLabel::None ? 2 : 26;
    setContentsMargins(leftMargin, topEdge ? 0 : 1, 0, topEdge ? 1 : 0);
}

void InfoBarWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter p(this);
    p.fillRect(rect(), backgroundColor());
    if (m_infoType != InfoLabel::None) {
        const QPixmap pixmap = icon().pixmap();
        const int iconY = (height() - pixmap.deviceIndependentSize().height()) / 2;
        p.drawPixmap(2, iconY, pixmap);
    }
    const QRectF adjustedRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const bool topEdge = m_edge == Qt::TopEdge;
    p.setPen(creatorColor(Theme::FancyToolBarSeparatorColor));
    p.drawLine(QLineF(topEdge ? adjustedRect.bottomLeft() : adjustedRect.topLeft(),
                      topEdge ? adjustedRect.bottomRight() : adjustedRect.topRight()));
}

QColor InfoBarEntry::backgroundColor(InfoLabel::InfoType infoType)
{
    Theme::Color color = Theme::InfoBarBackground;
    switch (infoType) {
    case InfoLabel::Information:
        color = Theme::Token_Notification_Neutral_Subtle; break;
    case InfoLabel::Warning:
        color = Theme::Token_Notification_Alert_Subtle; break;
    case InfoLabel::Error:
    case InfoLabel::NotOk:
        color = Theme::Token_Notification_Danger_Subtle; break;
    case InfoLabel::Ok:
        color = Theme::Token_Notification_Success_Subtle; break;
    default:
        color = Theme::InfoBarBackground; break;
    }
    return creatorColor(color);
}

QColor InfoBarWidget::backgroundColor() const
{
    return InfoBarEntry::backgroundColor(m_infoType);
}

const Icon &InfoBarEntry::icon(InfoLabel::InfoType infoType)
{
    switch (infoType) {
    case InfoLabel::Information: {
        const static Utils::Icon icon(
            {{":/utils/images/infolarge.png", Theme::Token_Notification_Neutral_Default}},
            Icon::Tint);
        return icon;
    }
    case InfoLabel::Warning: {
        const static Utils::Icon icon(
            {{":/utils/images/warninglarge.png", Theme::Token_Notification_Alert_Default}},
            Icon::Tint);
        return icon;
    }
    case InfoLabel::Error:
    case InfoLabel::NotOk: {
        const static Utils::Icon icon(
            {{":/utils/images/errorlarge.png", Theme::Token_Notification_Danger_Default}},
            Icon::Tint);
        return icon;
    }
    case InfoLabel::Ok: {
        const static Utils::Icon icon(
            {{":/utils/images/oklarge.png", Theme::Token_Notification_Success_Default}},
            Icon::Tint);
        return icon;
    }
    default: {
        const static Utils::Icon icon;
        return icon;
    }
    }
}

const Icon &InfoBarWidget::icon() const
{
    return InfoBarEntry::icon(m_infoType);
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

QString InfoBarEntry::title() const
{
    return m_title;
}

/*!
    Sets the \a title that is used if the entry is shown in popup form.
*/
void InfoBarEntry::setTitle(const QString &title)
{
    m_title = title;
}

InfoBarEntry::GlobalSuppression InfoBarEntry::globalSuppression() const
{
    return m_globalSuppression;
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

InfoBarEntry::Combo InfoBarEntry::combo() const
{
    return m_combo;
}

void InfoBarEntry::removeCancelButton()
{
    m_useCancelButton = false;
    m_cancelButtonText.clear();
    m_cancelButtonCallBack = nullptr;
}

bool InfoBarEntry::hasCancelButton() const
{
    return m_useCancelButton;
}

QString InfoBarEntry::cancelButtonText() const
{
    return m_cancelButtonText;
}

InfoBarEntry::CallBack InfoBarEntry::cancelButtonCallback() const
{
    return m_cancelButtonCallBack;
}

QList<InfoBarEntry::Button> InfoBarEntry::buttons() const
{
    return m_buttons;
}

void InfoBarEntry::setDetailsWidgetCreator(const InfoBarEntry::DetailsWidgetCreator &creator)
{
    m_detailsWidgetCreator = creator;
}

InfoBarEntry::DetailsWidgetCreator InfoBarEntry::detailsWidgetCreator() const
{
    return m_detailsWidgetCreator;
}

void InfoBarEntry::setInfoType(InfoLabel::InfoType infoType)
{
    m_infoType = infoType;
}

InfoLabel::InfoType InfoBarEntry::infoType() const
{
    return m_infoType;
}

void InfoBar::addInfo(const InfoBarEntry &info)
{
    m_infoBarEntries << info;
    emit changed();
}

void InfoBar::removeInfo(Id id)
{
    const int size = m_infoBarEntries.size();
    Utils::erase(m_infoBarEntries, equal(&InfoBarEntry::id, id));
    if (size != m_infoBarEntries.size())
        emit changed();
}

bool InfoBar::containsInfo(Id id) const
{
    return anyOf(m_infoBarEntries, equal(&InfoBarEntry::id, id));
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

QList<InfoBarEntry> InfoBar::entries() const
{
    return m_infoBarEntries;
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

static void disconnectRecursively(QObject *obj)
{
    obj->disconnect();
    for (QObject *child : obj->children())
        disconnectRecursively(child);
}

void InfoBarDisplay::update()
{
    for (QWidget *widget : std::as_const(m_infoWidgets)) {
        // Make sure that we are no longer connect to anything (especially lambdas).
        // Otherwise a lambda might live longer than the owner of the lambda.
        disconnectRecursively(widget);

        widget->hide(); // Late deletion can cause duplicate infos. Hide immediately to prevent it.
        widget->deleteLater();
    }
    m_infoWidgets.clear();

    if (!m_infoBar)
        return;

    const QList<InfoBarEntry> entries = m_infoBar->entries();
    for (const InfoBarEntry &info : entries) {
        auto infoWidget = new InfoBarWidget(m_edge, info.infoType());

        auto hbox = new QHBoxLayout;
        hbox->setContentsMargins(2, 2, 2, 2);

        auto vbox = new QVBoxLayout(infoWidget);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->addLayout(hbox);

        QLabel *infoWidgetLabel = new QLabel(info.text());
        infoWidgetLabel->setWordWrap(true);
        infoWidgetLabel->setOpenExternalLinks(true);
        hbox->addWidget(infoWidgetLabel, 1);

        if (info.detailsWidgetCreator()) {
            if (m_isShowingDetailsWidget) {
                QWidget *detailsWidget = info.detailsWidgetCreator()();
                vbox->addWidget(detailsWidget);
            }

            auto showDetailsButton = new QToolButton;
            showDetailsButton->setCheckable(true);
            showDetailsButton->setChecked(m_isShowingDetailsWidget);
            showDetailsButton->setText(Tr::tr("&Show Details"));
            connect(showDetailsButton, &QToolButton::clicked, this, [this, vbox, info] (bool) {
                QWidget *detailsWidget = vbox->count() == 2 ? vbox->itemAt(1)->widget() : nullptr;
                if (!detailsWidget) {
                    detailsWidget = info.detailsWidgetCreator()();
                    vbox->addWidget(detailsWidget);
                }

                m_isShowingDetailsWidget = !m_isShowingDetailsWidget;
                detailsWidget->setVisible(m_isShowingDetailsWidget);
            });

            hbox->addWidget(showDetailsButton);
        } else {
            m_isShowingDetailsWidget = false;
        }

        const InfoBarEntry::Combo combo = info.combo();
        if (!combo.entries.isEmpty()) {
            auto cb = new QComboBox();
            cb->setToolTip(combo.tooltip);
            for (const InfoBarEntry::ComboInfo &comboInfo : std::as_const(combo.entries))
                cb->addItem(comboInfo.displayText, comboInfo.data);
            if (combo.currentIndex >= 0 && combo.currentIndex < cb->count())
                cb->setCurrentIndex(combo.currentIndex);
            connect(
                cb,
                &QComboBox::currentIndexChanged,
                this,
                [cb, combo] { combo.callback({cb->currentText(), cb->currentData()}); },
                Qt::QueuedConnection);

            hbox->addWidget(cb);
        }

        const QList<InfoBarEntry::Button> buttons = info.buttons();
        for (const InfoBarEntry::Button &button : buttons) {
            auto infoWidgetButton = new QToolButton;
            infoWidgetButton->setText(button.text);
            infoWidgetButton->setToolTip(button.tooltip);
            connect(infoWidgetButton, &QAbstractButton::clicked, [button] { button.callback(); });
            hbox->addWidget(infoWidgetButton);
        }

        const Id id = info.id();
        QToolButton *infoWidgetSuppressButton = nullptr;
        if (info.globalSuppression() == InfoBarEntry::GlobalSuppression::Enabled) {
            infoWidgetSuppressButton = new QToolButton;
            infoWidgetSuppressButton->setText(Tr::tr("Do Not Show Again"));
            connect(infoWidgetSuppressButton, &QAbstractButton::clicked, this, [this, id] {
                m_infoBar->removeInfo(id);
                InfoBar::globallySuppressInfo(id);
            });
        }

        QToolButton *infoWidgetCloseButton = nullptr;
        if (info.hasCancelButton()) {
            infoWidgetCloseButton = new QToolButton;
            // need to connect to cancelObjectbefore connecting to cancelButtonClicked,
            // because the latter removes the button and with it any connect
            if (info.cancelButtonCallback())
                connect(infoWidgetCloseButton, &QAbstractButton::clicked, info.cancelButtonCallback());
            connect(infoWidgetCloseButton, &QAbstractButton::clicked, this, [this, id] {
                m_infoBar->removeInfo(id);
            });
        }

        if (infoWidgetCloseButton) {
            if (info.cancelButtonText().isEmpty()) {
                infoWidgetCloseButton->setAutoRaise(true);
                infoWidgetCloseButton->setIcon(Icons::CLOSE_FOREGROUND.icon());
                infoWidgetCloseButton->setToolTip(Tr::tr("Close"));
            } else {
                infoWidgetCloseButton->setText(info.cancelButtonText());
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
