// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pasteview.h"

#include "columnindicatortextedit.h"
#include "cpastertr.h"
#include "protocol.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTextEdit>

using namespace Utils;

namespace CodePaster {

const char groupC[] = "CPaster";
const char heightKeyC[] = "PasteViewHeight";
const char widthKeyC[] = "PasteViewWidth";

PasteView::PasteView(const QList<Protocol *> &protocols,
                     const QString &mt,
                     QWidget *parent) :
    QDialog(parent),
    m_protocols(protocols),
    m_commentPlaceHolder(Tr::tr("<Comment>")),
    m_mimeType(mt)
{
    setObjectName("CodePaster.ViewDialog");
    resize(670, 678);
    setWindowTitle(Tr::tr("Send to Codepaster"));

    m_protocolBox = new QComboBox;
    m_protocolBox->setObjectName("protocolBox");
    for (const Protocol *p : protocols)
        m_protocolBox->addItem(p->name());

    m_expirySpinBox = new QSpinBox;
    m_expirySpinBox->setSuffix(Tr::tr(" Days"));
    m_expirySpinBox->setRange(1, 365);

    m_uiUsername = new QLineEdit(this);
    m_uiUsername->setPlaceholderText(Tr::tr("<Username>"));

    m_uiDescription = new QLineEdit(this);
    m_uiDescription->setObjectName("uiDescription");
    m_uiDescription->setPlaceholderText(Tr::tr("<Description>"));

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_uiComment = new QTextEdit(this);
    m_uiComment->setSizePolicy(sizePolicy);
    m_uiComment->setMaximumHeight(100);
    m_uiComment->setTabChangesFocus(true);

    m_uiPatchList = new QListWidget;
    sizePolicy.setVerticalStretch(1);
    m_uiPatchList->setSizePolicy(sizePolicy);
    m_uiPatchList->setUniformItemSizes(true);

    m_uiPatchView = new CodePaster::ColumnIndicatorTextEdit;
    sizePolicy.setVerticalStretch(3);
    m_uiPatchView->setSizePolicy(sizePolicy);

    QFont font;
    font.setFamilies({"Courier New"});
    m_uiPatchView->setFont(font);
    m_uiPatchView->setReadOnly(true);

    auto groupBox = new QGroupBox(Tr::tr("Parts to Send to Server"));
    groupBox->setFlat(true);

    m_plainTextEdit = new QPlainTextEdit;
    m_plainTextEdit->setObjectName("plainTextEdit");

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(groupBox);
    m_stackedWidget->addWidget(m_plainTextEdit);
    m_stackedWidget->setCurrentIndex(0);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    buttonBox->button(QDialogButtonBox::Ok)->setText(Tr::tr("Paste"));

    const bool __sortingEnabled = m_uiPatchList->isSortingEnabled();
    m_uiPatchList->setSortingEnabled(false);
    m_uiPatchList->setSortingEnabled(__sortingEnabled);

    using namespace Layouting;

    Column {
        m_uiPatchList,
        m_uiPatchView
    }.attachTo(groupBox);

    Column {
        spacing(2),
        Form {
            Tr::tr("Protocol:"), m_protocolBox, br,
            Tr::tr("&Expires after:"), m_expirySpinBox, br,
            Tr::tr("&Username:"), m_uiUsername, br,
            Tr::tr("&Description:"), m_uiDescription,
        },
        m_uiComment,
        m_stackedWidget,
        buttonBox
    }.attachTo(this);

    connect(m_uiPatchList, &QListWidget::itemChanged, this, &PasteView::contentChanged);

    connect(m_protocolBox, &QComboBox::currentIndexChanged, this, &PasteView::protocolChanged);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

PasteView::~PasteView() = default;

QString PasteView::user() const
{
    const QString username = m_uiUsername->text();
    if (username.isEmpty())
        return QLatin1String("Anonymous");
    return username;
}

QString PasteView::description() const
{
    return m_uiDescription->text();
}

QString PasteView::comment() const
{
    const QString comment = m_uiComment->toPlainText();
    if (comment == m_commentPlaceHolder)
        return QString();
    return comment;
}

QString PasteView::content() const
{
    if (m_mode == PlainTextMode)
        return m_plainTextEdit->toPlainText();

    QString newContent;
    for (int i = 0; i < m_uiPatchList->count(); ++i) {
        QListWidgetItem *item = m_uiPatchList->item(i);
        if (item->checkState() != Qt::Unchecked)
            newContent += m_parts.at(i).content;
    }
    return newContent;
}

int PasteView::protocol() const
{
    return m_protocolBox->currentIndex();
}

void PasteView::contentChanged()
{
    m_uiPatchView->setPlainText(content());
}

void PasteView::protocolChanged(int p)
{
    QTC_ASSERT(p >= 0 && p < m_protocols.size(), return);
    const unsigned caps = m_protocols.at(p)->capabilities();
    m_uiDescription->setEnabled(caps & Protocol::PostDescriptionCapability);
    m_uiUsername->setEnabled(caps & Protocol::PostUserNameCapability);
    m_uiComment->setEnabled(caps & Protocol::PostCommentCapability);
}

void PasteView::setupDialog(const QString &user, const QString &description, const QString &comment)
{
    m_uiUsername->setText(user);
    m_uiDescription->setText(description);
    m_uiComment->setPlainText(comment.isEmpty() ? m_commentPlaceHolder : comment);
}

int PasteView::showDialog()
{
    m_uiDescription->setFocus();
    m_uiDescription->selectAll();

    // (Re)store dialog size
    const QtcSettings *settings = Core::ICore::settings();
    const Key rootKey = Key(groupC) + '/';
    const int h = settings->value(rootKey + heightKeyC, height()).toInt();
    const int defaultWidth = m_uiPatchView->columnIndicator() + 50;
    const int w = settings->value(rootKey + widthKeyC, defaultWidth).toInt();

    resize(w, h);

    return QDialog::exec();
}

// Show up with checkable list of diff chunks.
int PasteView::show(
        const QString &user,
        const QString &description,
        const QString &comment,
        int expiryDays,
        const FileDataList &parts)
{
    setupDialog(user, description, comment);
    m_uiPatchList->clear();
    m_parts = parts;
    m_mode = DiffChunkMode;
    QString content;
    for (const FileData &part : parts) {
        auto itm = new QListWidgetItem(part.filename, m_uiPatchList);
        itm->setCheckState(Qt::Checked);
        itm->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        content += part.content;
    }
    m_stackedWidget->setCurrentIndex(0);
    m_uiPatchView->setPlainText(content);
    setExpiryDays(expiryDays);
    return showDialog();
}

// Show up with editable plain text.
int PasteView::show(const QString &user, const QString &description,
                    const QString &comment, int expiryDays, const QString &content)
{
    setupDialog(user, description, comment);
    m_mode = PlainTextMode;
    m_stackedWidget->setCurrentIndex(1);
    m_plainTextEdit->setPlainText(content);
    setExpiryDays(expiryDays);
    return showDialog();
}

void PasteView::setExpiryDays(int d)
{
    m_expirySpinBox->setValue(d);
}

int PasteView::expiryDays() const
{
    return m_expirySpinBox->value();
}

void PasteView::accept()
{
    const int index = m_protocolBox->currentIndex();
    if (index == -1)
        return;

    Protocol *protocol = m_protocols.at(index);

    if (!Protocol::ensureConfiguration(protocol, this))
        return;

    const QString data = content();
    if (data.isEmpty())
        return;

    const Protocol::ContentType ct = Protocol::contentType(m_mimeType);
    protocol->paste(data, ct, expiryDays(), user(), comment(), description());
    // Store settings and close
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(groupC);
    settings->setValue(heightKeyC, height());
    settings->setValue(widthKeyC, width());
    settings->endGroup();
    QDialog::accept();
}

void PasteView::setProtocol(const QString &protocol)
{
     const int index = m_protocolBox->findText(protocol);
     if (index < 0)
         return;
     m_protocolBox->setCurrentIndex(index);
     if (index == m_protocolBox->currentIndex())
         protocolChanged(index); // Force enabling
     else
         m_protocolBox->setCurrentIndex(index);
}

} // CodePaster
