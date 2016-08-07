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

#include "ilocatorfilter.h"

#include <coreplugin/coreconstants.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>

using namespace Core;

ILocatorFilter::ILocatorFilter(QObject *parent):
    QObject(parent)
{
}

QString ILocatorFilter::shortcutString() const
{
    return m_shortcut;
}

void ILocatorFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
}

void ILocatorFilter::setShortcutString(const QString &shortcut)
{
    m_shortcut = shortcut;
}

QByteArray ILocatorFilter::saveState() const
{
    QByteArray value;
    QDataStream out(&value, QIODevice::WriteOnly);
    out << shortcutString();
    out << isIncludedByDefault();
    return value;
}

bool ILocatorFilter::restoreState(const QByteArray &state)
{
    QString shortcut;
    bool defaultFilter;

    QDataStream in(state);
    in >> shortcut;
    in >> defaultFilter;

    setShortcutString(shortcut);
    setIncludedByDefault(defaultFilter);
    return true;
}

bool ILocatorFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh)

    QDialog dialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
    dialog.setWindowTitle(msgConfigureDialogTitle());

    QVBoxLayout *vlayout = new QVBoxLayout(&dialog);
    QHBoxLayout *hlayout = new QHBoxLayout;
    QLineEdit *shortcutEdit = new QLineEdit(shortcutString());
    QCheckBox *includeByDefault = new QCheckBox(msgIncludeByDefault());
    includeByDefault->setToolTip(msgIncludeByDefaultToolTip());
    includeByDefault->setChecked(isIncludedByDefault());

    auto prefixLabel = new QLabel(msgPrefixLabel());
    prefixLabel->setToolTip(msgPrefixToolTip());
    hlayout->addWidget(prefixLabel);
    hlayout->addWidget(shortcutEdit);
    hlayout->addWidget(includeByDefault);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    vlayout->addLayout(hlayout);
    vlayout->addStretch();
    vlayout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        setShortcutString(shortcutEdit->text().trimmed());
        setIncludedByDefault(includeByDefault->isChecked());
        return true;
    }

    return false;
}

QString ILocatorFilter::trimWildcards(const QString &str)
{
    if (str.isEmpty())
        return str;
    int first = 0, last = str.size() - 1;
    const QChar asterisk = QLatin1Char('*');
    const QChar question = QLatin1Char('?');
    while (first < str.size() && (str.at(first) == asterisk || str.at(first) == question))
        ++first;
    while (last >= 0 && (str.at(last) == asterisk || str.at(last) == question))
        --last;
    if (first > last)
        return QString();
    return str.mid(first, last - first + 1);
}

Qt::CaseSensitivity ILocatorFilter::caseSensitivity(const QString &str)
{
    return str == str.toLower() ? Qt::CaseInsensitive : Qt::CaseSensitive;
}

QString ILocatorFilter::msgConfigureDialogTitle()
{
    return tr("Filter Configuration");
}

QString ILocatorFilter::msgPrefixLabel()
{
    return tr("Prefix:");
}

QString ILocatorFilter::msgPrefixToolTip()
{
    return tr("Type the prefix followed by a space and search term to restrict search to the filter.");
}

QString ILocatorFilter::msgIncludeByDefault()
{
    return tr("Include by default");
}

QString ILocatorFilter::msgIncludeByDefaultToolTip()
{
    return tr("Include the filter when not using a prefix for searches.");
}

bool ILocatorFilter::isConfigurable() const
{
    return m_isConfigurable;
}

bool ILocatorFilter::isIncludedByDefault() const
{
    return m_includedByDefault;
}

void ILocatorFilter::setIncludedByDefault(bool includedByDefault)
{
    m_includedByDefault = includedByDefault;
}

bool ILocatorFilter::isHidden() const
{
    return m_hidden;
}

void ILocatorFilter::setHidden(bool hidden)
{
    m_hidden = hidden;
}

bool ILocatorFilter::isEnabled() const
{
    return m_enabled;
}

Id ILocatorFilter::id() const
{
    return m_id;
}

QString ILocatorFilter::displayName() const
{
    return m_displayName;
}

ILocatorFilter::Priority ILocatorFilter::priority() const
{
    return m_priority;
}

void ILocatorFilter::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void ILocatorFilter::setId(Id id)
{
    m_id = id;
}

void ILocatorFilter::setPriority(Priority priority)
{
    m_priority = priority;
}

void ILocatorFilter::setDisplayName(const QString &displayString)
{
    m_displayName = displayString;
}

void ILocatorFilter::setConfigurable(bool configurable)
{
    m_isConfigurable = configurable;
}
