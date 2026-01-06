// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pasteselectdialog.h"

#include "cpastertr.h"
#include "protocol.h"
#include "settings.h"

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

namespace CodePaster {

class PasteSelectDialog : public QDialog
{
public:
    explicit PasteSelectDialog(const QList<Protocol *> &protocols);

    QString pasteId() const;
    int protocol() const;

private:
    void protocolChanged(int);
    void list();

    const QList<Protocol *> m_protocols;

    QComboBox *m_protocolBox = nullptr;
    QListWidget *m_listWidget = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QLineEdit *m_pasteEdit = nullptr;
    QSingleTaskTreeRunner m_taskTreeRunner;
};

PasteSelectDialog::PasteSelectDialog(const QList<Protocol *> &protocols)
    : QDialog(Core::ICore::dialogParent())
    , m_protocols(protocols)
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
        const QString name = protocol->name();
        m_protocolBox->addItem(name);
    }
    connect(m_protocolBox, &QComboBox::currentIndexChanged,
            this, &PasteSelectDialog::protocolChanged);
    connect(m_refreshButton, &QPushButton::clicked, this, &PasteSelectDialog::list);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    const int index = m_protocolBox->findText(settings().protocols.stringValue());
    if (index >= 0) {
        if (index != m_protocolBox->currentIndex())
            m_protocolBox->setCurrentIndex(index);
         else
            protocolChanged(index); // Trigger a refresh
    }
}

QString PasteSelectDialog::pasteId() const
{
    QString id = m_pasteEdit->text();
    const int blankPos = id.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        id.truncate(blankPos);
    return id;
}

int PasteSelectDialog::protocol() const
{
    return m_protocolBox->currentIndex();
}

void PasteSelectDialog::list()
{
    const int index = protocol();

    Protocol *protocol = m_protocols[index];
    QTC_ASSERT((protocol->capabilities() & Capability::List), return);

    m_listWidget->clear();
    if (Protocol::ensureConfiguration(protocol, this)) {
        m_listWidget->addItem(Tr::tr("Waiting for items..."));

        const auto listHandler = [this](const QStringList &results) {
            m_listWidget->clear();
            m_listWidget->addItems(results);
        };
        const auto errorHandler = [this] {
            m_listWidget->addItem(Tr::tr("Error while retrieving items."));
        };
        m_taskTreeRunner.start({protocol->listRecipe(listHandler)}, {},
                               errorHandler, QtTaskTree::CallDone::OnError);
    }
}

void PasteSelectDialog::protocolChanged(int i)
{
    m_taskTreeRunner.reset();
    const bool canList = m_protocols.at(i)->capabilities() & Capability::List;
    m_refreshButton->setEnabled(canList);
    if (canList) {
        list();
    } else {
        m_listWidget->clear();
        m_listWidget->addItem(new QListWidgetItem(Tr::tr("This protocol does not support listing")));
    }
}

QString executeFetchDialog(const QList<Protocol *> &protocols)
{
    PasteSelectDialog dialog(protocols);

    if (dialog.exec() != QDialog::Accepted)
        return {};
    // Save new protocol in case user changed it.
    if (settings().protocols() != dialog.protocol()) {
        settings().protocols.setValue(dialog.protocol());
        settings().writeSettings();
    }
    return dialog.pasteId();
}

} // CodePaster
