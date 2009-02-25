/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "codecselector.h"
#include "basetextdocument.h"

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QTextCodec>
#include <QtGui/QPushButton>
#include <QtGui/QScrollBar>
#include <QtGui/QVBoxLayout>

using namespace TextEditor;
using namespace TextEditor::Internal;

namespace TextEditor {
namespace Internal {

/* custom class to make sure the width is wide enough for the
 * contents. Should be easier with Qt. */
class CodecListWidget : public QListWidget
{
public:
    CodecListWidget(QWidget *parent):QListWidget(parent){}
    QSize sizeHint() const {
        return QListWidget::sizeHint().expandedTo(
            QSize(sizeHintForColumn(0) + verticalScrollBar()->sizeHint().width() + 4, 0));
    }
};

} // namespace Internal
} // namespace TextEditor


CodecSelector::CodecSelector(QWidget *parent, BaseTextDocument *doc)
    : QDialog(parent)
{
    m_hasDecodingError = doc->hasDecodingError();
    m_isModified = doc->isModified();

    QByteArray buf;
    if (m_hasDecodingError)
        buf = doc->decodingErrorSample();

    setWindowTitle(tr("Text Encoding"));
    m_label = new QLabel(this);
    QString decodingErrorHint;
    if (m_hasDecodingError)
        decodingErrorHint = tr("\nThe following encodings are likely to fit:");
    m_label->setText(tr("Select encoding for \"%1\".%2").arg(QFileInfo(doc->fileName()).fileName()).arg(decodingErrorHint));

    m_listWidget = new CodecListWidget(this);

    QStringList encodings;

    QList<int> mibs = QTextCodec::availableMibs();
    qSort(mibs);
    QList<int> sortedMibs;
    foreach (int mib, mibs)
        if (mib >= 0)
            sortedMibs += mib;
    foreach (int mib, mibs)
        if (mib < 0)
            sortedMibs += mib;

    int currentIndex = -1;
    foreach (int mib, sortedMibs) {
        QTextCodec *c = QTextCodec::codecForMib(mib);
        if (!buf.isEmpty()) {

            // slow, should use a feature from QTextCodec or QTextDecoder (but those are broken currently)
            QByteArray verifyBuf = c->fromUnicode(c->toUnicode(buf));
            // the minSize trick lets us ignore unicode headers
            int minSize = qMin(verifyBuf.size(), buf.size());
            if (minSize < buf.size() - 4
                || memcmp(verifyBuf.constData() + verifyBuf.size() - minSize,
                          buf.constData() + buf.size() - minSize, minSize))
                continue;
        }
        QString names = QString::fromLatin1(c->name());
        foreach (QByteArray alias, c->aliases())
            names += QLatin1String(" / ") + QString::fromLatin1(alias);
        if (doc->codec() == c)
            currentIndex = encodings.count();
        encodings << names;
    }
    m_listWidget->addItems(encodings);
    if (currentIndex >= 0)
        m_listWidget->setCurrentRow(currentIndex);

    connect(m_listWidget, SIGNAL(itemSelectionChanged()), this, SLOT(updateButtons()));

    m_dialogButtonBox = new QDialogButtonBox(this);
    m_reloadButton = m_dialogButtonBox->addButton(tr("Reload with Encoding"), QDialogButtonBox::DestructiveRole);
    m_saveButton =  m_dialogButtonBox->addButton(tr("Save with Encoding"), QDialogButtonBox::DestructiveRole);
    m_dialogButtonBox->addButton(QDialogButtonBox::Cancel);
    connect(m_dialogButtonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->addWidget(m_label);
    vbox->addWidget(m_listWidget);
    vbox->addWidget(m_dialogButtonBox);

    updateButtons();
}

CodecSelector::~CodecSelector()
{
}

void CodecSelector::updateButtons()
{
    bool hasCodec = (selectedCodec() != 0);
    m_reloadButton->setEnabled(!m_isModified && hasCodec);
    m_saveButton->setEnabled(!m_hasDecodingError && hasCodec);
}

QTextCodec *CodecSelector::selectedCodec() const
{
    if (QListWidgetItem *item = m_listWidget->currentItem()) {
        if (!item->isSelected())
            return 0;
        QString codecName = item->text();
        if (codecName.contains(QLatin1String(" / ")))
            codecName = codecName.left(codecName.indexOf(QLatin1String(" / ")));
        return QTextCodec::codecForName(codecName.toLatin1());
    }
    return 0;
}

CodecSelector::Result CodecSelector::exec()
{
    return (Result) QDialog::exec();
}

void CodecSelector::buttonClicked(QAbstractButton *button)
{
    Result result =  Cancel;
    if (button == m_reloadButton)
        result = Reload;
    if (button == m_saveButton)
        result = Save;
    done(result);
}

