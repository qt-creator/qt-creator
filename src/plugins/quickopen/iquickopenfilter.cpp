/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include "iquickopenfilter.h"

#include <QtGui/QBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

using namespace QuickOpen;

IQuickOpenFilter::IQuickOpenFilter(QObject *parent):
    QObject(parent)
{
}

QString IQuickOpenFilter::shortcutString() const
{
    return m_shortcut;
}

void IQuickOpenFilter::setShortcutString(const QString &shortcut)
{
    m_shortcut = shortcut;
}

QByteArray IQuickOpenFilter::saveState() const
{
    QByteArray value;
    QDataStream out(&value, QIODevice::WriteOnly);
    out << shortcutString();
    out << defaultActiveState();
    return value;
}

bool IQuickOpenFilter::restoreState(const QByteArray &state)
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

bool IQuickOpenFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh);

    QDialog dialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
    dialog.setWindowTitle(tr("Filter Configuration"));

    QVBoxLayout *vlayout = new QVBoxLayout(&dialog);
    QHBoxLayout *hlayout = new QHBoxLayout;
    QLineEdit *shortcutEdit = new QLineEdit(shortcutString());
    QCheckBox *limitCheck = new QCheckBox(tr("Limit to prefix"));
    limitCheck->setChecked(!defaultActiveState());

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

bool IQuickOpenFilter::isConfigurable() const
{
    return true;
}

bool IQuickOpenFilter::defaultActiveState() const
{
    return m_default;
}

void IQuickOpenFilter::setIncludedByDefault(bool includedByDefault)
{
    m_default = includedByDefault;
}
