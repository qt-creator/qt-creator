// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abiwidget.h"

#include "abi.h"
#include "projectexplorertr.h"

#include <utils/algorithm.h>
#include <utils/guard.h>
#include <utils/qtcassert.h>

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

bool pairLessThan(const QPair<QString, int> &lhs, const QPair<QString, int> &rhs)
{
    if (lhs.first == "unknown")
        return false;
    if (rhs.first == "unknown")
        return true;
    return lhs.first < rhs.first;
}

template<typename E>
void insertSorted(QComboBox *comboBox, E last)
{
    QList<QPair<QString, int>> abis;
    for (int i = 0; i <= static_cast<int>(last); ++i)
        abis << qMakePair(Abi::toString(static_cast<E>(i)), i);

    Utils::sort(abis, &pairLessThan);

    for (const auto &abiPair : abis)
        comboBox->addItem(abiPair.first, abiPair.second);
}

static int findIndex(const QComboBox *combo, int data)
{
    const int result = combo->findData(data);
    QTC_ASSERT(result != -1, return combo->count() - 1);
    return result;
}

template<typename T>
static void setIndex(QComboBox *combo, T value)
{
    combo->setCurrentIndex(findIndex(combo, static_cast<int>(value)));
}

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
    connect(d->m_abi, &QComboBox::currentIndexChanged, this, &AbiWidget::mainComboBoxChanged);

    d->m_architectureComboBox = new QComboBox(this);
    layout->addWidget(d->m_architectureComboBox);
    insertSorted(d->m_architectureComboBox, Abi::UnknownArchitecture);
    setIndex(d->m_architectureComboBox, Abi::UnknownArchitecture);
    connect(d->m_architectureComboBox, &QComboBox::currentIndexChanged,
            this, &AbiWidget::customComboBoxesChanged);

    QLabel *separator1 = new QLabel(this);
    separator1->setText(QLatin1String("-"));
    separator1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(separator1);

    d->m_osComboBox = new QComboBox(this);
    layout->addWidget(d->m_osComboBox);
    insertSorted(d->m_osComboBox, Abi::UnknownOS);
    setIndex(d->m_osComboBox, Abi::UnknownOS);
    connect(d->m_osComboBox, &QComboBox::currentIndexChanged,
            this, &AbiWidget::customOsComboBoxChanged);

    QLabel *separator2 = new QLabel(this);
    separator2->setText(QLatin1String("-"));
    separator2->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(separator2);

    d->m_osFlavorComboBox = new QComboBox(this);
    layout->addWidget(d->m_osFlavorComboBox);
    connect(d->m_osFlavorComboBox, &QComboBox::currentIndexChanged,
            this, &AbiWidget::customComboBoxesChanged);

    QLabel *separator3 = new QLabel(this);
    separator3->setText(QLatin1String("-"));
    separator3->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(separator3);

    d->m_binaryFormatComboBox = new QComboBox(this);
    layout->addWidget(d->m_binaryFormatComboBox);
    insertSorted(d->m_binaryFormatComboBox, Abi::UnknownFormat);
    setIndex(d->m_binaryFormatComboBox, Abi::UnknownFormat);
    connect(d->m_binaryFormatComboBox, &QComboBox::currentIndexChanged,
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
    connect(d->m_wordWidthComboBox, &QComboBox::currentIndexChanged,
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
        d->m_abi->addItem(Tr::tr("<custom>"), defaultAbi.toString());
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

    QList<QPair<QString, int>> sortedFlavors = Utils::transform(flavors, [](Abi::OSFlavor flavor) {
        return QPair<QString, int>{Abi::toString(flavor), static_cast<int>(flavor)};
    });

    Utils::sort(sortedFlavors, pairLessThan);

    for (const auto &[str, idx] : sortedFlavors)
        combo->addItem(str, idx);
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

// Sets a custom ABI in the custom abi widgets.
void AbiWidget::setCustomAbiComboBoxes(const Abi &current)
{
    const Utils::GuardLocker locker(d->m_ignoreChanges);

    setIndex(d->m_architectureComboBox, current.architecture());
    setIndex(d->m_osComboBox, current.os());
    updateOsFlavorCombobox(d->m_osFlavorComboBox, current.os());
    setIndex(d->m_osFlavorComboBox, current.osFlavor());
    setIndex(d->m_binaryFormatComboBox, current.binaryFormat());
    setIndex(d->m_wordWidthComboBox, current.wordWidth());
}

void AbiWidget::emitAbiChanged(const Abi &current)
{
    if (current == d->m_currentAbi)
        return;

    d->m_currentAbi = current;
    emit abiChanged();
}

} // namespace ProjectExplorer
