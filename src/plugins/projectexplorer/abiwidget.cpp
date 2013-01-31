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

#include "abiwidget.h"
#include "abi.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>

/*!
    \class ProjectExplorer::AbiWidget

    \brief A widget to set an ABI.

    \sa ProjectExplorer::Abi
*/

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// AbiWidgetPrivate:
// --------------------------------------------------------------------------

class AbiWidgetPrivate
{
public:
    QComboBox *m_abi;

    QComboBox *m_architectureComboBox;
    QComboBox *m_osComboBox;
    QComboBox *m_osFlavorComboBox;
    QComboBox *m_binaryFormatComboBox;
    QComboBox *m_wordWidthComboBox;
};

} // namespace Internal

// --------------------------------------------------------------------------
// AbiWidget
// --------------------------------------------------------------------------

AbiWidget::AbiWidget(QWidget *parent) :
    QWidget(parent),
    d(new Internal::AbiWidgetPrivate)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(2);

    d->m_abi = new QComboBox(this);
    layout->addWidget(d->m_abi);
    connect(d->m_abi, SIGNAL(currentIndexChanged(int)), this, SLOT(modeChanged()));

    layout->addSpacing(10);

    d->m_architectureComboBox = new QComboBox(this);
    layout->addWidget(d->m_architectureComboBox);
    for (int i = 0; i <= static_cast<int>(Abi::UnknownArchitecture); ++i)
        d->m_architectureComboBox->addItem(Abi::toString(static_cast<Abi::Architecture>(i)), i);
    d->m_architectureComboBox->setCurrentIndex(static_cast<int>(Abi::UnknownArchitecture));
    connect(d->m_architectureComboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(abiChanged()));

    QLabel *separator1 = new QLabel(this);
    separator1->setText(QLatin1String("-"));
    layout->addWidget(separator1);

    d->m_osComboBox = new QComboBox(this);
    layout->addWidget(d->m_osComboBox);
    for (int i = 0; i <= static_cast<int>(Abi::UnknownOS); ++i)
        d->m_osComboBox->addItem(Abi::toString(static_cast<Abi::OS>(i)), i);
    d->m_osComboBox->setCurrentIndex(static_cast<int>(Abi::UnknownOS));
    connect(d->m_osComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(osChanged()));

    QLabel *separator2 = new QLabel(this);
    separator2->setText(QLatin1String("-"));
    layout->addWidget(separator2);

    d->m_osFlavorComboBox = new QComboBox(this);
    layout->addWidget(d->m_osFlavorComboBox);
    osChanged();
    connect(d->m_osFlavorComboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(abiChanged()));

    QLabel *separator3 = new QLabel(this);
    separator3->setText(QLatin1String("-"));
    layout->addWidget(separator3);

    d->m_binaryFormatComboBox = new QComboBox(this);
    layout->addWidget(d->m_binaryFormatComboBox);
    for (int i = 0; i <= static_cast<int>(Abi::UnknownFormat); ++i)
        d->m_binaryFormatComboBox->addItem(Abi::toString(static_cast<Abi::BinaryFormat>(i)), i);
    d->m_binaryFormatComboBox->setCurrentIndex(static_cast<int>(Abi::UnknownFormat));
    connect(d->m_binaryFormatComboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(abiChanged()));

    QLabel *separator4 = new QLabel(this);
    separator4->setText(QLatin1String("-"));
    layout->addWidget(separator4);

    d->m_wordWidthComboBox = new QComboBox(this);
    layout->addWidget(d->m_wordWidthComboBox);

    d->m_wordWidthComboBox->addItem(Abi::toString(32), 32);
    d->m_wordWidthComboBox->addItem(Abi::toString(64), 64);
    d->m_wordWidthComboBox->addItem(Abi::toString(0), 0);
    d->m_wordWidthComboBox->setCurrentIndex(2);
    connect(d->m_wordWidthComboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(abiChanged()));

    layout->setStretchFactor(d->m_abi, 1);

    setAbis(QList<Abi>(), Abi::hostAbi());
}

AbiWidget::~AbiWidget()
{
    delete d;
}

void AbiWidget::setAbis(const QList<Abi> &abiList, const Abi &current)
{
    blockSignals(true);
    d->m_abi->clear();

    d->m_abi->addItem(tr("<custom>"), QLatin1String("custom"));
    d->m_abi->setCurrentIndex(0);

    for (int i = 0; i < abiList.count(); ++i) {
        const QString abiString = abiList.at(i).toString();
        d->m_abi->addItem(abiString, abiString);
        if (abiList.at(i) == current)
            d->m_abi->setCurrentIndex(i + 1);
    }

    d->m_abi->setVisible(!abiList.isEmpty());
    if (d->m_abi->currentIndex() == 0) {
        if (!current.isValid() && !abiList.isEmpty())
            d->m_abi->setCurrentIndex(1); // default to the first Abi if none is selected.
        else
            setCustomAbi(current);
    }
    modeChanged();

    blockSignals(false);
}

Abi AbiWidget::currentAbi() const
{
    if (d->m_abi->currentIndex() > 0)
        return Abi(d->m_abi->itemData(d->m_abi->currentIndex()).toString());

    return Abi(static_cast<Abi::Architecture>(d->m_architectureComboBox->currentIndex()),
               static_cast<Abi::OS>(d->m_osComboBox->currentIndex()),
               static_cast<Abi::OSFlavor>(d->m_osFlavorComboBox->itemData(d->m_osFlavorComboBox->currentIndex()).toInt()),
               static_cast<Abi::BinaryFormat>(d->m_binaryFormatComboBox->currentIndex()),
               d->m_wordWidthComboBox->itemData(d->m_wordWidthComboBox->currentIndex()).toInt());
}

void AbiWidget::osChanged()
{
    d->m_osFlavorComboBox->blockSignals(true);
    d->m_osFlavorComboBox->clear();
    Abi::OS os = static_cast<Abi::OS>(d->m_osComboBox->itemData(d->m_osComboBox->currentIndex()).toInt());
    QList<Abi::OSFlavor> flavors = Abi::flavorsForOs(os);
    foreach (Abi::OSFlavor f, flavors)
        d->m_osFlavorComboBox->addItem(Abi::toString(f), static_cast<int>(f));
    d->m_osFlavorComboBox->setCurrentIndex(0); // default to generic flavor
    d->m_osFlavorComboBox->blockSignals(false);

    emit abiChanged();
}

void AbiWidget::modeChanged()
{
    const bool customMode = (d->m_abi->currentIndex() == 0);
    d->m_architectureComboBox->setEnabled(customMode);
    d->m_osComboBox->setEnabled(customMode);
    d->m_osFlavorComboBox->setEnabled(customMode);
    d->m_binaryFormatComboBox->setEnabled(customMode);
    d->m_wordWidthComboBox->setEnabled(customMode);

    if (!customMode) {
        Abi current(d->m_abi->itemData(d->m_abi->currentIndex()).toString());
        setCustomAbi(current);
    }
}

void AbiWidget::setCustomAbi(const Abi &current)
{
    d->m_architectureComboBox->setCurrentIndex(static_cast<int>(current.architecture()));
    d->m_osComboBox->setCurrentIndex(static_cast<int>(current.os()));
    osChanged();
    for (int i = 0; i < d->m_osFlavorComboBox->count(); ++i) {
        if (d->m_osFlavorComboBox->itemData(i).toInt() == current.osFlavor()) {
            d->m_osFlavorComboBox->setCurrentIndex(i);
            break;
        }
    }
    d->m_binaryFormatComboBox->setCurrentIndex(static_cast<int>(current.binaryFormat()));
    for (int i = 0; i < d->m_wordWidthComboBox->count(); ++i) {
        if (d->m_wordWidthComboBox->itemData(i).toInt() == current.wordWidth()) {
            d->m_wordWidthComboBox->setCurrentIndex(i);
            break;
        }
    }
}

} // namespace ProjectExplorer
