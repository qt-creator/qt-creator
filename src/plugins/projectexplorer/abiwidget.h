// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "abi.h"

#include <QWidget>

#include <memory>

namespace ProjectExplorer {

namespace Internal { class AbiWidgetPrivate; }

// --------------------------------------------------------------------------
// AbiWidget:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT AbiWidget : public QWidget
{
    Q_OBJECT

public:
    AbiWidget(QWidget *parent = nullptr);
    ~AbiWidget() override;

    void setAbis(const ProjectExplorer::Abis &, const Abi &currentAbi);

    Abis supportedAbis() const;
    bool isCustomAbi() const;
    Abi currentAbi() const;

signals:
    void abiChanged();

private:
    void mainComboBoxChanged();
    void customOsComboBoxChanged();
    void customComboBoxesChanged();

    void setCustomAbiComboBoxes(const Abi &current);

    void emitAbiChanged(const Abi &current);

    const std::unique_ptr<Internal::AbiWidgetPrivate> d;
};

} // namespace ProjectExplorer
