// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diffalgorithm.h"
#include "diffeditorconstants.h"
#include "diffeditortr.h"

#include <coreplugin/icore.h>

#include <utils/qtcsettings.h>

#include <QComboBox>

namespace DiffEditor::Internal {

Utils::Differ::DiffAlgorithm readDiffAlgorithm()
{
    const Utils::QtcSettings *settings = Core::ICore::settings();
    const QString algorithm = settings->value(Constants::DIFF_ALGORITHM_KEY).toString();
    if (algorithm == "myers")
        return Utils::Differ::Myers;
    if (algorithm == "default")
        return Utils::Differ::Default;
    if (algorithm == "patience")
        return Utils::Differ::Patience;
    if (algorithm == "histogram")
        return Utils::Differ::Histogram;

    Utils::Differ::DiffAlgorithm legacyAlgorithm = Utils::Differ::Histogram;
    if (settings->contains(Constants::PATIENCE_KEY))
        legacyAlgorithm = settings->value(Constants::PATIENCE_KEY).toBool()
                              ? Utils::Differ::Patience : Utils::Differ::Default;
    saveDiffAlgorithm(legacyAlgorithm);
    return legacyAlgorithm;
}

void saveDiffAlgorithm(Utils::Differ::DiffAlgorithm algorithm)
{
    QString name;
    switch (algorithm) {
    case Utils::Differ::Default: name = "default"; break;
    case Utils::Differ::Myers: name = "myers"; break;
    case Utils::Differ::Patience: name = "patience"; break;
    case Utils::Differ::Histogram: name = "histogram"; break;
    }
    Core::ICore::settings()->setValue(Constants::DIFF_ALGORITHM_KEY, name);
}

void populateDiffAlgorithms(QComboBox *comboBox)
{
    comboBox->addItem(Tr::tr("Default"), Utils::Differ::Default);
    comboBox->addItem(Tr::tr("Histogram"), Utils::Differ::Histogram);
    comboBox->addItem(Tr::tr("Patience"), Utils::Differ::Patience);
    comboBox->addItem(Tr::tr("Myers"), Utils::Differ::Myers);
    comboBox->setToolTip(Tr::tr("Diff algorithm"));
}

} // namespace DiffEditor::Internal
