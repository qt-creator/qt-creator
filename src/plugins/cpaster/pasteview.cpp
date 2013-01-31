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

#include "pasteview.h"
#include "protocol.h"

#include <coreplugin/icore.h>

#include <QFontMetrics>
#include <QPainter>
#include <QScrollBar>
#include <QPushButton>
#include <QSettings>
#include <QByteArray>

static const char groupC[] = "CPaster";
static const char heightKeyC[] = "PasteViewHeight";
static const char widthKeyC[] = "PasteViewWidth";

namespace CodePaster {
// -------------------------------------------------------------------------------------------------
PasteView::PasteView(const QList<Protocol *> protocols,
                     const QString &mt,
                     QWidget *parent) :
    QDialog(parent),
    m_protocols(protocols),
    m_commentPlaceHolder(tr("<Comment>")),
    m_mimeType(mt),
    m_mode(DiffChunkMode)
{
    m_ui.setupUi(this);

    m_ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Paste"));
    connect(m_ui.uiPatchList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(contentChanged()));

    foreach (const Protocol *p, protocols)
        m_ui.protocolBox->addItem(p->name());
    connect(m_ui.protocolBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(protocolChanged(int)));
}

PasteView::~PasteView()
{
}

QString PasteView::user() const
{
    const QString username = m_ui.uiUsername->text();
    if (username.isEmpty())
        return QLatin1String("Anonymous");
    return username;
}

QString PasteView::description() const
{
    return m_ui.uiDescription->text();
}

QString PasteView::comment() const
{
    const QString comment = m_ui.uiComment->toPlainText();
    if (comment == m_commentPlaceHolder)
        return QString();
    return comment;
}

QString PasteView::content() const
{
    if (m_mode == PlainTextMode)
        return m_ui.plainTextEdit->toPlainText();

    QString newContent;
    for (int i = 0; i < m_ui.uiPatchList->count(); ++i) {
        QListWidgetItem *item = m_ui.uiPatchList->item(i);
        if (item->checkState() != Qt::Unchecked)
            newContent += m_parts.at(i).content;
    }
    return newContent;
}

QString PasteView::protocol() const
{
    return m_ui.protocolBox->currentText();
}

void PasteView::contentChanged()
{
    m_ui.uiPatchView->setPlainText(content());
}

void PasteView::protocolChanged(int p)
{
    const unsigned caps = m_protocols.at(p)->capabilities();
    m_ui.uiDescription->setEnabled(caps & Protocol::PostDescriptionCapability);
    m_ui.uiComment->setEnabled(caps & Protocol::PostCommentCapability);
}

void PasteView::setupDialog(const QString &user, const QString &description, const QString &comment)
{
    m_ui.uiUsername->setText(user);
    m_ui.uiDescription->setText(description);
    m_ui.uiComment->setPlainText(comment.isEmpty() ? m_commentPlaceHolder : comment);
}

int PasteView::showDialog()
{
    m_ui.uiDescription->setFocus();
    m_ui.uiDescription->selectAll();

    // (Re)store dialog size
    const QSettings *settings = Core::ICore::settings();
    const QString rootKey = QLatin1String(groupC) + QLatin1Char('/');
    const int h = settings->value(rootKey + QLatin1String(heightKeyC), height()).toInt();
    const int defaultWidth = m_ui.uiPatchView->columnIndicator() + 50;
    const int w = settings->value(rootKey + QLatin1String(widthKeyC), defaultWidth).toInt();

    resize(w, h);

    const int ret = QDialog::exec();
    return ret;
}

// Show up with checkable list of diff chunks.
int PasteView::show(const QString &user, const QString &description,
                    const QString &comment, const FileDataList &parts)
{
    setupDialog(user, description, comment);
    m_ui.uiPatchList->clear();
    m_parts = parts;
    m_mode = DiffChunkMode;
    QString content;
    foreach (const FileData &part, parts) {
        QListWidgetItem *itm = new QListWidgetItem(part.filename, m_ui.uiPatchList);
        itm->setCheckState(Qt::Checked);
        itm->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        content += part.content;
    }
    m_ui.stackedWidget->setCurrentIndex(0);
    m_ui.uiPatchView->setPlainText(content);
    return showDialog();
}

// Show up with editable plain text.
int PasteView::show(const QString &user, const QString &description,
                    const QString &comment, const QString &content)
{
    setupDialog(user, description, comment);
    m_mode = PlainTextMode;
    m_ui.stackedWidget->setCurrentIndex(1);
    m_ui.plainTextEdit->setPlainText(content);
    return showDialog();
}

void PasteView::accept()
{
    const int index = m_ui.protocolBox->currentIndex();
    if (index == -1)
        return;

    Protocol *protocol = m_protocols.at(index);

    if (!Protocol::ensureConfiguration(protocol, this))
        return;

    const QString data = content();
    if (data.isEmpty())
        return;

    const Protocol::ContentType ct = Protocol::contentType(m_mimeType);
    protocol->paste(data, ct, user(), comment(), description());
    // Store settings and close
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(heightKeyC), height());
    settings->setValue(QLatin1String(widthKeyC), width());
    settings->endGroup();
    QDialog::accept();
}

void PasteView::setProtocol(const QString &protocol)
{
     const int index = m_ui.protocolBox->findText(protocol);
     m_ui.protocolBox->setCurrentIndex(index);
     if (index == m_ui.protocolBox->currentIndex())
         protocolChanged(index); // Force enabling
     else
         m_ui.protocolBox->setCurrentIndex(index);
}

} //namespace CodePaster
