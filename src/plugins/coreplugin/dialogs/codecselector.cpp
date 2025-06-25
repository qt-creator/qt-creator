// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codecselector.h"

#include "../coreplugintr.h"
#include "../icore.h"
#include "../textdocument.h"

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/itemviews.h>

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

using namespace Utils;

namespace Core {
namespace Internal {

class CodecSelector : public QDialog
{
public:
    explicit CodecSelector(BaseTextDocument *doc);

    TextEncoding selectedEncoding() const;

private:
    void updateButtons();
    void buttonClicked(QAbstractButton *button);

    bool m_hasDecodingError;
    bool m_isModified;
    QLabel *m_label;
    Utils::ListWidget *m_listWidget;
    QDialogButtonBox *m_dialogButtonBox;
    QAbstractButton *m_reloadButton;
    QAbstractButton *m_saveButton;
};

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

CodecSelector::CodecSelector(BaseTextDocument *doc)
    : QDialog(ICore::dialogParent())
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

    std::set<QString> encodingNames;

    const QByteArrayList codecs = Utils::sorted(TextEncoding::availableCodecs());

    int currentIndex = -1;
    for (const QByteArray &codec : codecs) {
        const TextEncoding encoding(codec);
        if (!doc->supportsEncoding(encoding))
            continue;
        if (!buf.isEmpty()) {
            const QByteArray verifyBuf = encoding.encode(encoding.decode(buf));
            // the minSize trick lets us ignore unicode headers
            int minSize = qMin(verifyBuf.size(), buf.size());
            if (minSize < buf.size() - 4
                || memcmp(verifyBuf.constData() + verifyBuf.size() - minSize,
                          buf.constData() + buf.size() - minSize, minSize))
                continue;
        }
        if (doc->encoding() == encoding)
            currentIndex = encodingNames.size();
        encodingNames.insert(encoding.fullDisplayName());
    }
    m_listWidget->addItems(QStringList(encodingNames.begin(), encodingNames.end()));
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

void CodecSelector::updateButtons()
{
    bool hasCodec = selectedEncoding().isValid();
    m_reloadButton->setEnabled(!m_isModified && hasCodec);
    m_saveButton->setEnabled(!m_hasDecodingError && hasCodec);
}

TextEncoding CodecSelector::selectedEncoding() const
{
    if (QListWidgetItem *item = m_listWidget->currentItem()) {
        if (!item->isSelected())
            return {};
        QString codecName = item->text();
        if (codecName.contains(QLatin1String(" / ")))
            codecName = codecName.left(codecName.indexOf(QLatin1String(" / ")));
        return codecName.toLatin1();
    }
    return {};
}

void CodecSelector::buttonClicked(QAbstractButton *button)
{
    CodecSelectorResult::Action result = CodecSelectorResult::Cancel;
    if (button == m_reloadButton)
        result = CodecSelectorResult::Reload;
    if (button == m_saveButton)
        result = CodecSelectorResult::Save;
    done(result);
}

} // namespace Internal

CodecSelectorResult askForCodec(BaseTextDocument *doc)
{
    Internal::CodecSelector dialog(doc);
    const CodecSelectorResult::Action result = CodecSelectorResult::Action(dialog.exec());
    return {result, dialog.selectedEncoding()};
}

} // namespace Core
