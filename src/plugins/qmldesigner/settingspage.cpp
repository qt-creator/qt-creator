/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "designersettings.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "settingspage.h"

#include <qmljseditor/qmljseditorconstants.h>

#include <QTextStream>
#include <QCheckBox>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

SettingsPageWidget::SettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
}

DesignerSettings SettingsPageWidget::settings() const
{
    DesignerSettings ds;
    ds.itemSpacing = m_ui.spinItemSpacing->value();
    ds.snapMargin = m_ui.spinSnapMargin->value();
    ds.canvasWidth = m_ui.spinCanvasWidth->value();
    ds.canvasHeight = m_ui.spinCanvasHeight->value();
    return ds;
}

void SettingsPageWidget::setSettings(const DesignerSettings &s)
{
    m_ui.spinItemSpacing->setValue(s.itemSpacing);
    m_ui.spinSnapMargin->setValue(s.snapMargin);
    m_ui.spinCanvasWidth->setValue(s.canvasWidth);
    m_ui.spinCanvasHeight->setValue(s.canvasHeight);
}

QString SettingsPageWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc)
            << ' ' << m_ui.snapMarginLabel->text()
            << ' ' << m_ui.itemSpacingLabel->text()
            << ' ' << m_ui.canvasWidthLabel->text()
            << ' ' << m_ui.canvasHeightLabel->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

SettingsPage::SettingsPage() :
    m_widget(0)
{
    setId(QLatin1String("B.QmlDesigner"));
    setDisplayName(tr("Qt Quick Designer"));
    setCategory(QLatin1String(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML));
    setDisplayCategory(QCoreApplication::translate("QmlJSEditor",
        QmlJSEditor::Constants::SETTINGS_TR_CATEGORY_QML));
    setCategoryIcon(QLatin1String(Constants::SETTINGS_CATEGORY_QML_ICON));
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    m_widget = new SettingsPageWidget(parent);
    m_widget->setSettings(BauhausPlugin::pluginInstance()->settings());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void SettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;
    BauhausPlugin::pluginInstance()->setSettings(m_widget->settings());
}

bool SettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
