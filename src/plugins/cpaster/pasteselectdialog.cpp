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

#include "pasteselectdialog.h"
#include "protocol.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QPushButton>

namespace CodePaster {

PasteSelectDialog::PasteSelectDialog(const QList<Protocol*> &protocols,
                                     QWidget *parent) :
    QDialog(parent),
    m_protocols(protocols)
{
    m_ui.setupUi(this);
    foreach (const Protocol *protocol, protocols) {
        m_ui.protocolBox->addItem(protocol->name());
        connect(protocol, &Protocol::listDone, this, &PasteSelectDialog::listDone);
    }
    connect(m_ui.protocolBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &PasteSelectDialog::protocolChanged);

    m_refreshButton = m_ui.buttons->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
    connect(m_refreshButton, &QPushButton::clicked, this, &PasteSelectDialog::list);

    m_ui.listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    if (!Utils::HostOsInfo::isMacHost())
        m_ui.listWidget->setFrameStyle(QFrame::NoFrame);
    // Proportional formatting of columns for CodePaster
    QFont listFont = m_ui.listWidget->font();
    listFont.setFamily(QLatin1String("Courier"));
    listFont.setStyleHint(QFont::TypeWriter);
    m_ui.listWidget->setFont(listFont);
}

PasteSelectDialog::~PasteSelectDialog()
{
}

QString PasteSelectDialog::pasteId() const
{
    QString id = m_ui.pasteEdit->text();
    const int blankPos = id.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        id.truncate(blankPos);
    return id;
}

void PasteSelectDialog::setProtocol(const QString &p)
{
    const int index = m_ui.protocolBox->findText(p);
    if (index >= 0) {
        if (index != m_ui.protocolBox->currentIndex()) {
            m_ui.protocolBox->setCurrentIndex(index);
        } else {
            // Trigger a refresh
            protocolChanged(index);
        }
    }
}

QString PasteSelectDialog::protocol() const
{
    return m_ui.protocolBox->currentText();
}

int PasteSelectDialog::protocolIndex() const
{
    return m_ui.protocolBox->currentIndex();
}

void PasteSelectDialog::listDone(const QString &name, const QStringList &items)
{
    // Set if the protocol is still current
    if (name == protocol()) {
        m_ui.listWidget->clear();
        m_ui.listWidget->addItems(items);
    }
}

void PasteSelectDialog::list()
{
    const int index = protocolIndex();

    Protocol *protocol = m_protocols[index];
    QTC_ASSERT((protocol->capabilities() & Protocol::ListCapability), return);

    m_ui.listWidget->clear();
    if (Protocol::ensureConfiguration(protocol, this)) {
        m_ui.listWidget->addItem(new QListWidgetItem(tr("Waiting for items")));
        protocol->list();
    }
}

void PasteSelectDialog::protocolChanged(int i)
{
    const bool canList = m_protocols.at(i)->capabilities() & Protocol::ListCapability;
    m_refreshButton->setEnabled(canList);
    if (canList) {
        list();
    } else {
        m_ui.listWidget->clear();
        m_ui.listWidget->addItem(new QListWidgetItem(tr("This protocol does not support listing")));
    }
}
} // namespace CodePaster
