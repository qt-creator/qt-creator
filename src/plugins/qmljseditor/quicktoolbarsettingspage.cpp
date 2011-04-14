/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include <qmldesigner/qmldesignerconstants.h>
#include "quicktoolbarsettingspage.h"

#include <coreplugin/icore.h>

#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <QtGui/QCheckBox>


using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;

QuickToolBarSettings::QuickToolBarSettings()
    : enableContextPane(false),
    pinContextPane(false)
{}

void QuickToolBarSettings::set()
{
    if (get() != *this) {
        if (QSettings *settings = Core::ICore::instance()->settings())
            toSettings(settings);
    }
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
    Core::ICore *core = Core::ICore::instance();
    QuickToolBarSettings settings;
    settings.fromSettings(core->settings());
    return settings;
}

QuickToolBarSettingsPage::QuickToolBarSettingsPage() :
    m_widget(0)
{
}

QString QuickToolBarSettingsPage::id() const
{
    return QLatin1String("QmlToolbar");
}

QString QuickToolBarSettingsPage::displayName() const
{
    return tr("Qt Quick ToolBar");
}

QString QuickToolBarSettingsPage::category() const
{
    return QLatin1String("Qt Quick");
}

QString QuickToolBarSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("Qt Quick", "Qt Quick");
}

QIcon QuickToolBarSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(QmlDesigner::Constants::SETTINGS_CATEGORY_QML_ICON));
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
