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
    m_includedByDefault(false),
    m_hidden(false),
    m_enabled(true)
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

bool ILocatorFilter::isConfigurable() const
{
    return true;
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

void ILocatorFilter::setEnabled(bool enabled)
{
    m_enabled = enabled;
}
