// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debugviewwidget.h"


#include <qmldesignerplugin.h>
#include <designersettings.h>

namespace QmlDesigner {

namespace Internal {

DebugViewWidget::DebugViewWidget(QWidget *parent) : QWidget(parent)
{
    m_ui.setupUi(this);
    connect(m_ui.enabledCheckBox, &QAbstractButton::toggled,
            this, &DebugViewWidget::enabledCheckBoxToggled);
}

void DebugViewWidget::addLogMessage(const QString &topic, const QString &message, bool highlight)
{
    if (highlight) {
        m_ui.modelLog->appendHtml(QStringLiteral("<b><font color=\"blue\">")
                                  + topic
                                  + QStringLiteral("</b><br>")
                                  + message);

    } else {
        m_ui.modelLog->appendHtml(QStringLiteral("<b>")
                                  + topic
                                  + QStringLiteral("</b><br>")
                                  + message);
    }
}

void DebugViewWidget::addErrorMessage(const QString &topic, const QString &message)
{
    m_ui.instanceErrorLog->appendHtml(QStringLiteral("<b><font color=\"red\">")
                              + topic
                              + QStringLiteral("</font></b><br>")
                              + message
                                      );
}

void DebugViewWidget::addLogInstanceMessage(const QString &topic, const QString &message, bool highlight)
{
    if (highlight) {
        m_ui.instanceLog->appendHtml("<b><font color=\"blue\">"
                                     + topic
                                     + "</b><br>"
                                     + "<p>"
                                     + message
                                     + "</p>"
                                     + "<br>");

    } else {
        m_ui.instanceLog->appendHtml("<b>"
                                     + topic
                                     + "</b><br>"
                                     + "<p>"
                                     + message
                                     + "</p>"
                                     + "<br>");
    }
}

void DebugViewWidget::setPuppetStatus(const QString &text)
{
    m_ui.instanceStatus->setPlainText(text);
}

void DebugViewWidget::setDebugViewEnabled(bool b)
{
    if (m_ui.enabledCheckBox->isChecked() != b)
        m_ui.enabledCheckBox->setChecked(b);
}

void DebugViewWidget::enabledCheckBoxToggled(bool b)
{
    QmlDesignerPlugin::settings().insert(DesignerSettingsKey::WARNING_FOR_FEATURES_IN_DESIGNER, b);
}

} //namespace Internal

} //namespace QmlDesigner
