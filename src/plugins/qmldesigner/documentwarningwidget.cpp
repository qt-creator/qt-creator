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

#include "documentwarningwidget.h"

#include <qmldesignerplugin.h>


#include <utils/theme/theme.h>
#include <utils/stylehelper.h>

#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QBoxLayout>
#include <QEvent>

#include <QDebug>

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

    m_ignoreWarningsCheckBox->setText(tr("Ignore always these unsupported Qt Quick Designer warnings."));

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

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_headerLabel);
    QVBoxLayout *messageLayout = new QVBoxLayout;
    messageLayout->setMargin(20);
    messageLayout->setSpacing(5);
    messageLayout->addWidget(m_navigateLabel);
    messageLayout->addWidget(m_messageLabel);
    layout->addLayout(messageLayout);
    layout->addWidget(m_ignoreWarningsCheckBox);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
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
    DocumentMessage message = m_messages.at(m_currentMessage);
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
    if (m_messages.count() > 1) {
        if (m_currentMessage != 0)
            links << link.arg(QLatin1String("previous"), tr("Previous"));
        else
            links << tr("Previous");

        if (m_messages.count() - 1 > m_currentMessage)
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
    const QColor backgroundColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_BackgroundColor);
    QPalette pal = palette();
    QColor color = pal.color(QPalette::ToolTipBase);
    const QColor backgroundNoAlpha = Utils::StyleHelper::alphaBlendedColors(color, backgroundColor);
    color.setAlpha(255);
    pal.setColor(QPalette::ToolTipBase, backgroundNoAlpha);
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
    return DesignerSettings::getValue(DesignerSettingsKey::WARNING_FOR_FEATURES_IN_DESIGNER).toBool();
}

void DocumentWarningWidget::ignoreCheckBoxToggled(bool b)
{
    DesignerSettings::setValue(DesignerSettingsKey::WARNING_FOR_FEATURES_IN_DESIGNER, !b);
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
