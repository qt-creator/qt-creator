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

#include "ilocatorfilter.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>

using namespace Locator;

ILocatorFilter::ILocatorFilter(QObject *parent):
    QObject(parent),
    m_priority(Medium),
    m_includedByDefault(false),
    m_hidden(false),
    m_enabled(true),
    m_isConfigurable(true)
{
}

QString ILocatorFilter::shortcutString() const
{
    return m_shortcut;
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
    dialog.setWindowTitle(tr("Filter Configuration"));

    QVBoxLayout *vlayout = new QVBoxLayout(&dialog);
    QHBoxLayout *hlayout = new QHBoxLayout;
    QLineEdit *shortcutEdit = new QLineEdit(shortcutString());
    QCheckBox *limitCheck = new QCheckBox(tr("Limit to prefix"));
    limitCheck->setChecked(!isIncludedByDefault());

    hlayout->addWidget(new QLabel(tr("Prefix:")));
    hlayout->addWidget(shortcutEdit);
    hlayout->addWidget(limitCheck);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    vlayout->addLayout(hlayout);
    vlayout->addStretch();
    vlayout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        setShortcutString(shortcutEdit->text().trimmed());
        setIncludedByDefault(!limitCheck->isChecked());
        return true;
    }

    return false;
}

QString ILocatorFilter::trimWildcards(const QString &str)
{
    if (str.isEmpty())
        return str;
    int first = 0, last = str.size()-1;
    const QChar asterisk = QLatin1Char('*');
    const QChar question = QLatin1Char('?');
    while (first < str.size() && (str.at(first) == asterisk || str.at(first) == question))
        ++first;
    while (last >= 0 && (str.at(last) == asterisk || str.at(last) == question))
        --last;
    if (first > last)
        return QString();
    return str.mid(first, last-first+1);
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

Core::Id ILocatorFilter::id() const
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

void ILocatorFilter::setId(Core::Id id)
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
