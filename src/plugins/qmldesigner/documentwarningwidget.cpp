// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "documentwarningwidget.h"

#include <qmldesignerplugin.h>

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QEvent>
#include <QLabel>
#include <QPushButton>

namespace QmlDesigner {

static QString errorToString(const DocumentMessage &error)
{
    return QString("Line: %1: %2").arg(error.line()).arg(error.description());
}

DocumentWarningWidget::DocumentWarningWidget(QWidget *parent)
    : Utils::FakeToolTip(parent)
    , m_headerLabel(new QLabel(this))
    , m_messageLabel(new QLabel(this))
    , m_navigateLabel(new QLabel(this))
    , m_ignoreWarningsCheckBox(new QCheckBox(this))
    , m_continueButton(new QPushButton(this))
{
    // this "tooltip" should behave like a widget with parent child relation to the designer mode
    setWindowFlags(Qt::Widget);

    QFont boldFont = font();
    boldFont.setBold(true);
    m_headerLabel->setFont(boldFont);
    m_messageLabel->setForegroundRole(QPalette::ToolTipText);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_ignoreWarningsCheckBox->setText(tr("Always ignore these warnings about features "
                                         "not supported by Qt Quick Designer."));

    connect(m_navigateLabel, &QLabel::linkActivated, this, [=](const QString &link) {
        if (link == QLatin1String("goToCode")) {
            emitGotoCodeClicked(m_messages.at(m_currentMessage));
        } else if (link == QLatin1String("previous")) {
            --m_currentMessage;
            refreshContent();
        } else if (link == QLatin1String("next")) {
            ++m_currentMessage;
            refreshContent();
        }
    });

    connect(m_continueButton, &QPushButton::clicked, this, [=]() {
        if (m_mode == ErrorMode)
            emitGotoCodeClicked(m_messages.at(m_currentMessage));
        else
            hide();
    });

    connect(m_ignoreWarningsCheckBox, &QCheckBox::toggled, this, &DocumentWarningWidget::ignoreCheckBoxToggled);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_headerLabel);
    auto messageLayout = new QVBoxLayout;
    messageLayout->setContentsMargins(20, 20, 20, 20);
    messageLayout->setSpacing(5);
    messageLayout->addWidget(m_navigateLabel);
    messageLayout->addWidget(m_messageLabel);
    layout->addLayout(messageLayout);
    layout->addWidget(m_ignoreWarningsCheckBox);
    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_continueButton);
    layout->addLayout(buttonLayout);

    parent->installEventFilter(this);
}

void DocumentWarningWidget::moveToParentCenter()
{
    move(parentWidget()->rect().center() - rect().center());
}

void DocumentWarningWidget::refreshContent()
{

    if (m_mode == ErrorMode) {
        m_headerLabel->setText(tr("Cannot open this QML document because of an error in the QML file:"));
        m_ignoreWarningsCheckBox->hide();
        m_continueButton->setText(tr("OK"));
    } else {
        m_headerLabel->setText(tr("This QML file contains features which are not supported by Qt Quick Designer at:"));
        {
            QSignalBlocker blocker(m_ignoreWarningsCheckBox);
            m_ignoreWarningsCheckBox->setChecked(!warningsEnabled());
        }
        m_ignoreWarningsCheckBox->show();
        m_continueButton->setText(tr("Ignore"));
    }

    QString messageString;
    DocumentMessage message = m_messages.value(m_currentMessage);
    if (message.type() == DocumentMessage::ParseError) {
        messageString += errorToString(message);
        m_navigateLabel->setText(generateNavigateLinks());
        m_navigateLabel->show();
    } else {
        messageString += message.toString();
        m_navigateLabel->hide();
    }

    m_messageLabel->setText(messageString);
    resize(layout()->totalSizeHint());
}

QString DocumentWarningWidget::generateNavigateLinks()
{
    static const QString link("<a href=\"%1\">%2</a>");
    QStringList links;
    if (m_messages.size() > 1) {
        if (m_currentMessage != 0)
            links << link.arg(QLatin1String("previous"), tr("Previous"));
        else
            links << tr("Previous");

        if (m_messages.size() - 1 > m_currentMessage)
            links << link.arg(QLatin1String("next"), tr("Next"));
        else
            links << tr("Next");
    }

    if (m_mode == ErrorMode)
        links << link.arg(QLatin1String("goToCode"), tr("Go to error"));
    else
        links << link.arg(QLatin1String("goToCode"), tr("Go to warning"));
    return links.join(QLatin1String(" | "));
}

bool DocumentWarningWidget::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        moveToParentCenter();
    }
    return QObject::eventFilter(object, event);
}

void DocumentWarningWidget::showEvent(QShowEvent *event)
{
    const QColor backgroundColor = Utils::creatorTheme()->color(Utils::Theme::DScontrolBackground);
    const QColor outlineColor = Utils::creatorTheme()->color(Utils::Theme::DScontrolOutline);
    QPalette pal = palette();
    pal.setColor(QPalette::ToolTipBase, backgroundColor);
    pal.setColor(QPalette::ToolTipText, outlineColor);
    setPalette(pal);
    m_gotoCodeWasClicked = false;
    moveToParentCenter();
    refreshContent();
    Utils::FakeToolTip::showEvent(event);
}

bool DocumentWarningWidget::gotoCodeWasClicked()
{
    return m_gotoCodeWasClicked;
}

void DocumentWarningWidget::emitGotoCodeClicked(const DocumentMessage &message)
{
    m_gotoCodeWasClicked = true;
    emit gotoCodeClicked(message.url().toLocalFile(), message.line(), message.column() - 1);
}

bool DocumentWarningWidget::warningsEnabled() const
{
    return QmlDesignerPlugin::settings().value(DesignerSettingsKey::WARNING_FOR_FEATURES_IN_DESIGNER).toBool();
}

void DocumentWarningWidget::ignoreCheckBoxToggled(bool b)
{
    QmlDesignerPlugin::settings().value(DesignerSettingsKey::WARNING_FOR_FEATURES_IN_DESIGNER, !b);
}

void DocumentWarningWidget::setErrors(const QList<DocumentMessage> &errors)
{
    Q_ASSERT(!errors.empty());
    m_mode = ErrorMode;
    setMessages(errors);
}

void DocumentWarningWidget::setWarnings(const QList<DocumentMessage> &warnings)
{
    Q_ASSERT(!warnings.empty());
    m_mode = WarningMode;
    setMessages(warnings);
}

void DocumentWarningWidget::setMessages(const QList<DocumentMessage> &messages)
{
    m_messages.clear();
    m_messages = messages;
    m_currentMessage = 0;
    refreshContent();
}

} // namespace Designer
