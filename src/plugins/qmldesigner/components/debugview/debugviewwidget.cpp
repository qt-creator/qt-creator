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
        m_ui.modelLog->appendHtml(QLatin1String("<b><font color=\"blue\">")
                                  + topic
                                  + QLatin1String("</b><br>")
                                  + message);

    } else {
        m_ui.modelLog->appendHtml(QLatin1String("<b>")
                                  + topic
                                  + QLatin1String("</b><br>")
                                  + message);
    }
}

void DebugViewWidget::addErrorMessage(const QString &topic, const QString &message)
{
    m_ui.instanceErrorLog->appendHtml(QLatin1String("<b><font color=\"red\">")
                              + topic
                              + QLatin1String("</font></b><br>")
                              + message
                                      );
}

void DebugViewWidget::addLogInstanceMessage(const QString &topic, const QString &message, bool highlight)
{
    if (highlight) {
        m_ui.instanceLog->appendHtml(QLatin1String("<b><font color=\"blue\">")
                                  + topic
                                  + QLatin1String("</b><br>")
                                  + message);

    } else {
        m_ui.instanceLog->appendHtml(QLatin1String("<b>")
                                  + topic
                                  + QLatin1String("</b><br>")
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
