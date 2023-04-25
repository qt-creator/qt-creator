// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pasteselectdialog.h"

#include "cpastertr.h"
#include "protocol.h"

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

namespace CodePaster {

PasteSelectDialog::PasteSelectDialog(const QList<Protocol*> &protocols, QWidget *parent) :
    QDialog(parent),
    m_protocols(protocols)
{
    setObjectName("CodePaster.PasteSelectDialog");
    resize(550, 350);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_protocolBox = new QComboBox(this);
    m_protocolBox->setObjectName("protocolBox");

    m_pasteEdit = new QLineEdit(this);
    m_pasteEdit->setObjectName("pasteEdit");
    m_pasteEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_listWidget = new QListWidget(this);
    m_listWidget->setObjectName("listWidget");
    m_listWidget->setAlternatingRowColors(true);

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    m_refreshButton = buttons->addButton(Tr::tr("Refresh"), QDialogButtonBox::ActionRole);

    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    if (!Utils::HostOsInfo::isMacHost())
        m_listWidget->setFrameStyle(QFrame::NoFrame);

    // Proportional formatting of columns for CodePaster
    QFont listFont = m_listWidget->font();
    listFont.setFamily(QLatin1String("Courier"));
    listFont.setStyleHint(QFont::TypeWriter);
    m_listWidget->setFont(listFont);

    using namespace Layouting;
    Column {
        Form {
            Tr::tr("Protocol:"), m_protocolBox, br,
            Tr::tr("Paste:"), m_pasteEdit
        },
        m_listWidget,
        buttons
    }.attachTo(this);

    connect(m_listWidget, &QListWidget::currentTextChanged, m_pasteEdit, &QLineEdit::setText);
    connect(m_listWidget, &QListWidget::doubleClicked, this, &QDialog::accept);

    for (const Protocol *protocol : protocols) {
        m_protocolBox->addItem(protocol->name());
        connect(protocol, &Protocol::listDone, this, &PasteSelectDialog::listDone);
    }
    connect(m_protocolBox, &QComboBox::currentIndexChanged,
            this, &PasteSelectDialog::protocolChanged);
    connect(m_refreshButton, &QPushButton::clicked, this, &PasteSelectDialog::list);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

PasteSelectDialog::~PasteSelectDialog() = default;

QString PasteSelectDialog::pasteId() const
{
    QString id = m_pasteEdit->text();
    const int blankPos = id.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        id.truncate(blankPos);
    return id;
}

void PasteSelectDialog::setProtocol(const QString &p)
{
    const int index = m_protocolBox->findText(p);
    if (index >= 0) {
        if (index != m_protocolBox->currentIndex()) {
            m_protocolBox->setCurrentIndex(index);
        } else {
            // Trigger a refresh
            protocolChanged(index);
        }
    }
}

int PasteSelectDialog::protocol() const
{
    return m_protocolBox->currentIndex();
}

QString PasteSelectDialog::protocolName() const
{
    return m_protocolBox->currentText();
}

void PasteSelectDialog::listDone(const QString &name, const QStringList &items)
{
    // Set if the protocol is still current
    if (name == protocolName()) {
        m_listWidget->clear();
        m_listWidget->addItems(items);
    }
}

void PasteSelectDialog::list()
{
    const int index = protocol();

    Protocol *protocol = m_protocols[index];
    QTC_ASSERT((protocol->capabilities() & Protocol::ListCapability), return);

    m_listWidget->clear();
    if (Protocol::ensureConfiguration(protocol, this)) {
        m_listWidget->addItem(new QListWidgetItem(Tr::tr("Waiting for items")));
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
        m_listWidget->clear();
        m_listWidget->addItem(new QListWidgetItem(Tr::tr("This protocol does not support listing")));
    }
}

} // CodePaster
