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

#include "abiwidget.h"
#include "abi.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>

/*!
    \class ProjectExplorer::AbiWidget

    \brief The AbiWidget class is a widget to set an ABI.

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
    bool isCustom() const
    {
        return m_abi->currentIndex() == 0;
    }

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

AbiWidget::AbiWidget(QWidget *parent) : QWidget(parent),
    d(new Internal::AbiWidgetPrivate)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(2);

    d->m_abi = new QComboBox(this);
    d->m_abi->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    d->m_abi->setMinimumContentsLength(4);
    layout->addWidget(d->m_abi);
    connect(d->m_abi, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AbiWidget::modeChanged);

    d->m_architectureComboBox = new QComboBox(this);
    layout->addWidget(d->m_architectureComboBox);
    for (int i = 0; i <= static_cast<int>(Abi::UnknownArchitecture); ++i)
        d->m_architectureComboBox->addItem(Abi::toString(static_cast<Abi::Architecture>(i)), i);
    d->m_architectureComboBox->setCurrentIndex(static_cast<int>(Abi::UnknownArchitecture));
    connect(d->m_architectureComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AbiWidget::customAbiChanged);

    QLabel *separator1 = new QLabel(this);
    separator1->setText(QLatin1String("-"));
    separator1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(separator1);

    d->m_osComboBox = new QComboBox(this);
    layout->addWidget(d->m_osComboBox);
    for (int i = 0; i <= static_cast<int>(Abi::UnknownOS); ++i)
        d->m_osComboBox->addItem(Abi::toString(static_cast<Abi::OS>(i)), i);
    d->m_osComboBox->setCurrentIndex(static_cast<int>(Abi::UnknownOS));
    connect(d->m_osComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AbiWidget::osChanged);

    QLabel *separator2 = new QLabel(this);
    separator2->setText(QLatin1String("-"));
    separator2->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(separator2);

    d->m_osFlavorComboBox = new QComboBox(this);
    layout->addWidget(d->m_osFlavorComboBox);
    connect(d->m_osFlavorComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AbiWidget::customAbiChanged);

    QLabel *separator3 = new QLabel(this);
    separator3->setText(QLatin1String("-"));
    separator3->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(separator3);

    d->m_binaryFormatComboBox = new QComboBox(this);
    layout->addWidget(d->m_binaryFormatComboBox);
    for (int i = 0; i <= static_cast<int>(Abi::UnknownFormat); ++i)
        d->m_binaryFormatComboBox->addItem(Abi::toString(static_cast<Abi::BinaryFormat>(i)), i);
    d->m_binaryFormatComboBox->setCurrentIndex(static_cast<int>(Abi::UnknownFormat));
    connect(d->m_binaryFormatComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AbiWidget::customAbiChanged);

    QLabel *separator4 = new QLabel(this);
    separator4->setText(QLatin1String("-"));
    separator4->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(separator4);

    d->m_wordWidthComboBox = new QComboBox(this);
    layout->addWidget(d->m_wordWidthComboBox);

    d->m_wordWidthComboBox->addItem(Abi::toString(32), 32);
    d->m_wordWidthComboBox->addItem(Abi::toString(64), 64);
    d->m_wordWidthComboBox->addItem(Abi::toString(0), 0);
    d->m_wordWidthComboBox->setCurrentIndex(2);
    connect(d->m_wordWidthComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AbiWidget::customAbiChanged);

    layout->setStretchFactor(d->m_abi, 1);

    setAbis(QList<Abi>(), Abi::hostAbi());
}

AbiWidget::~AbiWidget()
{
    delete d;
}

void AbiWidget::setAbis(const QList<Abi> &abiList, const Abi &current)
{
    QSignalBlocker blocker(this);
    d->m_abi->clear();

    Abi defaultAbi = current;
    if (defaultAbi.isNull()) {
        if (!abiList.isEmpty())
            defaultAbi = abiList.at(0);
        else
            defaultAbi = Abi::hostAbi();
    }

    d->m_abi->addItem(tr("<custom>"), defaultAbi.toString());
    d->m_abi->setCurrentIndex(0);

    for (int i = 0; i < abiList.count(); ++i) {
        int index = i + 1;
        const QString abiString = abiList.at(i).toString();
        d->m_abi->insertItem(index, abiString, abiString);
        if (abiList.at(i) == current)
            d->m_abi->setCurrentIndex(index);
    }

    d->m_abi->setVisible(!abiList.isEmpty());
    if (d->isCustom()) {
        if (!current.isValid() && !abiList.isEmpty())
            d->m_abi->setCurrentIndex(1); // default to the first Abi if none is selected.
        else
            setCustomAbi(current);
    }
    modeChanged();
}

QList<Abi> AbiWidget::supportedAbis() const
{
    QList<Abi> result;
    result.reserve(d->m_abi->count());
    for (int i = 1; i < d->m_abi->count(); ++i)
        result << Abi(d->m_abi->itemData(i).toString());
    return result;
}

bool AbiWidget::isCustomAbi() const
{
    return d->isCustom();
}

Abi AbiWidget::currentAbi() const
{
    return Abi(d->m_abi->itemData(d->m_abi->currentIndex()).toString());
}

void AbiWidget::osChanged()
{
    {
        QSignalBlocker blocker(d->m_osFlavorComboBox);
        d->m_osFlavorComboBox->clear();
        Abi::OS os = static_cast<Abi::OS>(d->m_osComboBox->itemData(d->m_osComboBox->currentIndex()).toInt());
        QList<Abi::OSFlavor> flavors = Abi::flavorsForOs(os);
        foreach (Abi::OSFlavor f, flavors)
            d->m_osFlavorComboBox->addItem(Abi::toString(f), static_cast<int>(f));
        d->m_osFlavorComboBox->setCurrentIndex(0); // default to generic flavor
    }
    customAbiChanged();
}

void AbiWidget::modeChanged()
{
    const bool customMode = d->isCustom();
    d->m_architectureComboBox->setEnabled(customMode);
    d->m_osComboBox->setEnabled(customMode);
    d->m_osFlavorComboBox->setEnabled(customMode);
    d->m_binaryFormatComboBox->setEnabled(customMode);
    d->m_wordWidthComboBox->setEnabled(customMode);

    setCustomAbi(currentAbi());
}

void AbiWidget::customAbiChanged()
{
    if (signalsBlocked())
        return;

    Abi current(static_cast<Abi::Architecture>(d->m_architectureComboBox->currentIndex()),
                static_cast<Abi::OS>(d->m_osComboBox->currentIndex()),
                static_cast<Abi::OSFlavor>(d->m_osFlavorComboBox->itemData(d->m_osFlavorComboBox->currentIndex()).toInt()),
                static_cast<Abi::BinaryFormat>(d->m_binaryFormatComboBox->currentIndex()),
                d->m_wordWidthComboBox->itemData(d->m_wordWidthComboBox->currentIndex()).toInt());
    d->m_abi->setItemData(0, current.toString());

    emit abiChanged();
}

void AbiWidget::setCustomAbi(const Abi &current)
{
    {
        QSignalBlocker blocker(this);
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
        if (d->isCustom())
            d->m_abi->setItemData(0, current.toString());
    }

    emit abiChanged();
}

} // namespace ProjectExplorer
