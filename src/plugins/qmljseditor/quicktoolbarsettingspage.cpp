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

#include "quicktoolbarsettingspage.h"
#include "qmljseditorconstants.h"

#include <qmldesigner/qmldesignerconstants.h>
#include <coreplugin/icore.h>

#include <QSettings>
#include <QTextStream>
#include <QCheckBox>


using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;

QuickToolBarSettings::QuickToolBarSettings()
    : enableContextPane(false),
    pinContextPane(false)
{}

void QuickToolBarSettings::set()
{
    if (get() != *this)
        toSettings(Core::ICore::settings());
}

void QuickToolBarSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(QmlDesigner::Constants::QML_SETTINGS_GROUP));
    settings->beginGroup(QLatin1String(QmlDesigner::Constants::QML_DESIGNER_SETTINGS_GROUP));
    enableContextPane = settings->value(
            QLatin1String(QmlDesigner::Constants::QML_CONTEXTPANE_KEY), QVariant(false)).toBool();
    pinContextPane = settings->value(
                QLatin1String(QmlDesigner::Constants::QML_CONTEXTPANEPIN_KEY), QVariant(false)).toBool();
    settings->endGroup();
    settings->endGroup();
}

void QuickToolBarSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(QmlDesigner::Constants::QML_SETTINGS_GROUP));
    settings->beginGroup(QLatin1String(QmlDesigner::Constants::QML_DESIGNER_SETTINGS_GROUP));
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_CONTEXTPANE_KEY), enableContextPane);
    settings->setValue(QLatin1String(QmlDesigner::Constants::QML_CONTEXTPANEPIN_KEY), pinContextPane);
    settings->endGroup();
    settings->endGroup();
}

bool QuickToolBarSettings::equals(const QuickToolBarSettings &other) const
{
    return  enableContextPane == other.enableContextPane
            && pinContextPane == other.pinContextPane;
}


QuickToolBarSettingsPageWidget::QuickToolBarSettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
}

QuickToolBarSettings QuickToolBarSettingsPageWidget::settings() const
{
    QuickToolBarSettings ds;
    ds.enableContextPane = m_ui.textEditHelperCheckBox->isChecked();
    ds.pinContextPane = m_ui.textEditHelperCheckBoxPin->isChecked();
    return ds;
}

void QuickToolBarSettingsPageWidget::setSettings(const QuickToolBarSettings &s)
{
    m_ui.textEditHelperCheckBox->setChecked(s.enableContextPane);
    m_ui.textEditHelperCheckBoxPin->setChecked(s.pinContextPane);
}

QString QuickToolBarSettingsPageWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc)
            << ' ' << m_ui.textEditHelperCheckBox->text()
            << ' ' << m_ui.textEditHelperCheckBoxPin->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

QuickToolBarSettings QuickToolBarSettings::get()
{
    QuickToolBarSettings settings;
    settings.fromSettings(Core::ICore::settings());
    return settings;
}

QuickToolBarSettingsPage::QuickToolBarSettingsPage() :
    m_widget(0)
{
    setId("C.QmlToolbar");
    setDisplayName(tr("Qt Quick ToolBar"));
    setCategory(Constants::SETTINGS_CATEGORY_QML);
    setDisplayCategory(QCoreApplication::translate("QmlJSEditor",
        QmlJSEditor::Constants::SETTINGS_TR_CATEGORY_QML));
    setCategoryIcon(QLatin1String(QmlDesigner::Constants::SETTINGS_CATEGORY_QML_ICON));
}

QWidget *QuickToolBarSettingsPage::createPage(QWidget *parent)
{
    m_widget = new QuickToolBarSettingsPageWidget(parent);
    m_widget->setSettings(QuickToolBarSettings::get());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void QuickToolBarSettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;
    m_widget->settings().set();
}

bool QuickToolBarSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
