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

#include <utils/guard.h>

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

    Utils::Guard m_ignoreChanges;

    Abi m_currentAbi;

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
    d(std::make_unique<Internal::AbiWidgetPrivate>())
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    d->m_abi = new QComboBox(this);
    d->m_abi->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    d->m_abi->setMinimumContentsLength(4);
    layout->addWidget(d->m_abi);
    connect(d->m_abi, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AbiWidget::mainComboBoxChanged);

    d->m_architectureComboBox = new QComboBox(this);
    layout->addWidget(d->m_architectureComboBox);
    for (int i = 0; i <= static_cast<int>(Abi::UnknownArchitecture); ++i)
        d->m_architectureComboBox->addItem(Abi::toString(static_cast<Abi::Architecture>(i)), i);
    d->m_architectureComboBox->setCurrentIndex(static_cast<int>(Abi::UnknownArchitecture));
    connect(d->m_architectureComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AbiWidget::customComboBoxesChanged);

    QLabel *separator1 = new QLabel(this);
    separator1->setText(QLatin1String("-"));
    separator1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(separator1);

    d->m_osComboBox = new QComboBox(this);
    layout->addWidget(d->m_osComboBox);
    for (int i = 0; i <= static_cast<int>(Abi::UnknownOS); ++i)
        d->m_osComboBox->addItem(Abi::toString(static_cast<Abi::OS>(i)), i);
    d->m_osComboBox->setCurrentIndex(static_cast<int>(Abi::UnknownOS));
    connect(d->m_osComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AbiWidget::customOsComboBoxChanged);

    QLabel *separator2 = new QLabel(this);
    separator2->setText(QLatin1String("-"));
    separator2->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(separator2);

    d->m_osFlavorComboBox = new QComboBox(this);
    layout->addWidget(d->m_osFlavorComboBox);
    connect(d->m_osFlavorComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AbiWidget::customComboBoxesChanged);

    QLabel *separator3 = new QLabel(this);
    separator3->setText(QLatin1String("-"));
    separator3->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(separator3);

    d->m_binaryFormatComboBox = new QComboBox(this);
    layout->addWidget(d->m_binaryFormatComboBox);
    for (int i = 0; i <= static_cast<int>(Abi::UnknownFormat); ++i)
        d->m_binaryFormatComboBox->addItem(Abi::toString(static_cast<Abi::BinaryFormat>(i)), i);
    d->m_binaryFormatComboBox->setCurrentIndex(static_cast<int>(Abi::UnknownFormat));
    connect(d->m_binaryFormatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AbiWidget::customComboBoxesChanged);

    QLabel *separator4 = new QLabel(this);
    separator4->setText(QLatin1String("-"));
    separator4->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(separator4);

    d->m_wordWidthComboBox = new QComboBox(this);
    layout->addWidget(d->m_wordWidthComboBox);

    d->m_wordWidthComboBox->addItem(Abi::toString(16), 16);
    d->m_wordWidthComboBox->addItem(Abi::toString(32), 32);
    d->m_wordWidthComboBox->addItem(Abi::toString(64), 64);
    d->m_wordWidthComboBox->addItem(Abi::toString(0), 0);
    // Setup current word width of 0 by default.
    d->m_wordWidthComboBox->setCurrentIndex(d->m_wordWidthComboBox->count() - 1);
    connect(d->m_wordWidthComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AbiWidget::customComboBoxesChanged);

    layout->setStretchFactor(d->m_abi, 1);

    setAbis(Abis(), Abi::hostAbi());
}

AbiWidget::~AbiWidget() = default;

static Abi selectAbi(const Abi &current, const Abis &abiList)
{
    if (!current.isNull())
        return current;
    if (!abiList.isEmpty())
        return abiList.at(0);
    return Abi::hostAbi();
}

void AbiWidget::setAbis(const Abis &abiList, const Abi &currentAbi)
{
    const Abi defaultAbi = selectAbi(currentAbi, abiList);
    {
        const Utils::GuardLocker locker(d->m_ignoreChanges);

        // Initial setup of ABI combobox:
        d->m_abi->clear();
        d->m_abi->addItem(tr("<custom>"), defaultAbi.toString());
        d->m_abi->setCurrentIndex(0);
        d->m_abi->setVisible(!abiList.isEmpty());

        // Add supported ABIs:
        for (const Abi &abi : abiList) {
            const QString abiString = abi.toString();

            d->m_abi->addItem(abiString, abiString);
            if (abi == defaultAbi)
                d->m_abi->setCurrentIndex(d->m_abi->count() - 1);
        }

        setCustomAbiComboBoxes(defaultAbi);
    }

    // Update disabled state according to new automatically selected item in main ABI combobox.
    // This will call emitAbiChanged with the actual selected ABI.
    mainComboBoxChanged();
}

Abis AbiWidget::supportedAbis() const
{
    Abis result;
    result.reserve(d->m_abi->count());
    for (int i = 1; i < d->m_abi->count(); ++i)
        result << Abi::fromString(d->m_abi->itemData(i).toString());
    return result;
}

bool AbiWidget::isCustomAbi() const
{
    return d->isCustom();
}

Abi AbiWidget::currentAbi() const
{
    return d->m_currentAbi;
}

static void updateOsFlavorCombobox(QComboBox *combo, const Abi::OS os)
{
    const QList<Abi::OSFlavor> flavors = Abi::flavorsForOs(os);
    combo->clear();
    for (const Abi::OSFlavor &f : flavors)
        combo->addItem(Abi::toString(f), static_cast<int>(f));
    combo->setCurrentIndex(0);
}

void AbiWidget::customOsComboBoxChanged()
{
    if (d->m_ignoreChanges.isLocked())
        return;

    {
        const Utils::GuardLocker locker(d->m_ignoreChanges);
        d->m_osFlavorComboBox->clear();
        const Abi::OS os = static_cast<Abi::OS>(d->m_osComboBox->itemData(d->m_osComboBox->currentIndex()).toInt());
        updateOsFlavorCombobox(d->m_osFlavorComboBox, os);
    }

    customComboBoxesChanged();
}

void AbiWidget::mainComboBoxChanged()
{
    if (d->m_ignoreChanges.isLocked())
        return;

    const Abi newAbi = Abi::fromString(d->m_abi->currentData().toString());
    const bool customMode = d->isCustom();

    d->m_architectureComboBox->setEnabled(customMode);
    d->m_osComboBox->setEnabled(customMode);
    d->m_osFlavorComboBox->setEnabled(customMode);
    d->m_binaryFormatComboBox->setEnabled(customMode);
    d->m_wordWidthComboBox->setEnabled(customMode);

    setCustomAbiComboBoxes(newAbi);

    if (customMode)
        customComboBoxesChanged();
    else
        emitAbiChanged(Abi::fromString(d->m_abi->currentData().toString()));
}

void AbiWidget::customComboBoxesChanged()
{
    if (d->m_ignoreChanges.isLocked())
        return;

    const Abi current(static_cast<Abi::Architecture>(d->m_architectureComboBox->currentData().toInt()),
                      static_cast<Abi::OS>(d->m_osComboBox->currentData().toInt()),
                      static_cast<Abi::OSFlavor>(d->m_osFlavorComboBox->currentData().toInt()),
                      static_cast<Abi::BinaryFormat>(d->m_binaryFormatComboBox->currentData().toInt()),
                      static_cast<unsigned char>(d->m_wordWidthComboBox->currentData().toInt()));
    d->m_abi->setItemData(0, current.toString()); // Save custom Abi
    emitAbiChanged(current);
}

static int findIndex(const QComboBox *combo, int data)
{
    for (int i = 0; i < combo->count(); ++i) {
        if (combo->itemData(i).toInt() == data)
            return i;
    }
    return combo->count() >= 1 ? 0 : -1;
}

static void setIndex(QComboBox *combo, int data)
{
    combo->setCurrentIndex(findIndex(combo, data));
}

// Sets a custom ABI in the custom abi widgets.
void AbiWidget::setCustomAbiComboBoxes(const Abi &current)
{
    const Utils::GuardLocker locker(d->m_ignoreChanges);

    setIndex(d->m_architectureComboBox, static_cast<int>(current.architecture()));
    setIndex(d->m_osComboBox, static_cast<int>(current.os()));
    updateOsFlavorCombobox(d->m_osFlavorComboBox, current.os());
    setIndex(d->m_osFlavorComboBox, static_cast<int>(current.osFlavor()));
    setIndex(d->m_binaryFormatComboBox, static_cast<int>(current.binaryFormat()));
    setIndex(d->m_wordWidthComboBox, static_cast<int>(current.wordWidth()));
}

void AbiWidget::emitAbiChanged(const Abi &current)
{
    if (current == d->m_currentAbi)
        return;

    d->m_currentAbi = current;
    emit abiChanged();
}

} // namespace ProjectExplorer
