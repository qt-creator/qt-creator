// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codecselector.h"

#include "../coreplugintr.h"
#include "../textdocument.h"

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/itemviews.h>

#include <QDebug>
#include <QTextCodec>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

namespace Core {

/* custom class to make sure the width is wide enough for the
 * contents. Should be easier with Qt. */
class CodecListWidget : public Utils::ListWidget
{
public:
    CodecListWidget(QWidget *parent) : Utils::ListWidget(parent){}
    QSize sizeHint() const override {
        return QListWidget::sizeHint().expandedTo(
            QSize(sizeHintForColumn(0) + verticalScrollBar()->sizeHint().width() + 4, 0));
    }
};

CodecSelector::CodecSelector(QWidget *parent, Core::BaseTextDocument *doc)
    : QDialog(parent)
{
    m_hasDecodingError = doc->hasDecodingError();
    m_isModified = doc->isModified();

    QByteArray buf;
    if (m_hasDecodingError)
        buf = doc->decodingErrorSample();

    setWindowTitle(Tr::tr("Text Encoding"));
    m_label = new QLabel(this);
    QString decodingErrorHint;
    if (m_hasDecodingError)
        decodingErrorHint = '\n' + Tr::tr("The following encodings are likely to fit:");
    m_label->setText(Tr::tr("Select encoding for \"%1\".%2")
                     .arg(doc->filePath().fileName())
                     .arg(decodingErrorHint));

    m_listWidget = new CodecListWidget(this);
    m_listWidget->setActivationMode(Utils::DoubleClickActivation);

    QStringList encodings;

    const QList<int> mibs = Utils::sorted(QTextCodec::availableMibs());
    QList<int> sortedMibs;
    for (const int mib : mibs)
        if (mib >= 0)
            sortedMibs += mib;
    for (const int mib : mibs)
        if (mib < 0)
            sortedMibs += mib;

    int currentIndex = -1;
    for (const int mib : std::as_const(sortedMibs)) {
        QTextCodec *c = QTextCodec::codecForMib(mib);
        if (!doc->supportsCodec(c))
            continue;
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
        const QList<QByteArray> aliases = c->aliases();
        for (const QByteArray &alias : aliases)
            names += QLatin1String(" / ") + QString::fromLatin1(alias);
        if (doc->codec() == c)
            currentIndex = encodings.count();
        encodings << names;
    }
    m_listWidget->addItems(encodings);
    if (currentIndex >= 0)
        m_listWidget->setCurrentRow(currentIndex);

    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &CodecSelector::updateButtons);

    m_dialogButtonBox = new QDialogButtonBox(this);
    m_reloadButton = m_dialogButtonBox->addButton(Tr::tr("Reload with Encoding"), QDialogButtonBox::DestructiveRole);
    m_saveButton =  m_dialogButtonBox->addButton(Tr::tr("Save with Encoding"), QDialogButtonBox::DestructiveRole);
    m_dialogButtonBox->addButton(QDialogButtonBox::Cancel);
    connect(m_dialogButtonBox, &QDialogButtonBox::clicked, this, &CodecSelector::buttonClicked);
    connect(m_listWidget, &QAbstractItemView::activated, m_reloadButton, &QAbstractButton::click);

    auto vbox = new QVBoxLayout(this);
    vbox->addWidget(m_label);
    vbox->addWidget(m_listWidget);
    vbox->addWidget(m_dialogButtonBox);

    updateButtons();
}

CodecSelector::~CodecSelector() = default;

void CodecSelector::updateButtons()
{
    bool hasCodec = (selectedCodec() != nullptr);
    m_reloadButton->setEnabled(!m_isModified && hasCodec);
    m_saveButton->setEnabled(!m_hasDecodingError && hasCodec);
}

QTextCodec *CodecSelector::selectedCodec() const
{
    if (QListWidgetItem *item = m_listWidget->currentItem()) {
        if (!item->isSelected())
            return nullptr;
        QString codecName = item->text();
        if (codecName.contains(QLatin1String(" / ")))
            codecName = codecName.left(codecName.indexOf(QLatin1String(" / ")));
        return QTextCodec::codecForName(codecName.toLatin1());
    }
    return nullptr;
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

} // namespace Core
