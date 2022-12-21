// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ifunctionhintproposalmodel.h"

#include <QString>

using namespace TextEditor;

IFunctionHintProposalModel::IFunctionHintProposalModel() = default;

IFunctionHintProposalModel::~IFunctionHintProposalModel() = default;

QString IFunctionHintProposalModel::id(int /*index*/) const
{
    return QString();
}
