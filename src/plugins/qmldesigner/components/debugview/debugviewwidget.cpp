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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "debugviewwidget.h"


#include <qmldesignerplugin.h>
#include <designersettings.h>

namespace QmlDesigner {

namespace Internal {

DebugViewWidget::DebugViewWidget(QWidget *parent) : QWidget(parent)
{
    m_ui.setupUi(this);
    connect(m_ui.enabledCheckBox, SIGNAL(toggled(bool)), this, SLOT(enabledCheckBoxToggled(bool)));
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
        m_ui.instanceLog->appendHtml(QStringLiteral("<b><font color=\"blue\">")
                                  + topic
                                  + QStringLiteral("</b><br>")
                                  + message);

    } else {
        m_ui.instanceLog->appendHtml(QStringLiteral("<b>")
                                  + topic
                                  + QStringLiteral("</b><br>")
                                  + message);
    }
}

void DebugViewWidget::setDebugViewEnabled(bool b)
{
    if (m_ui.enabledCheckBox->isChecked() != b)
        m_ui.enabledCheckBox->setChecked(b);
}

void DebugViewWidget::enabledCheckBoxToggled(bool b)
{
    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
    settings.warningsInDesigner = b;
    QmlDesignerPlugin::instance()->setSettings(settings);
}

} //namespace Internal

} //namespace QmlDesigner
