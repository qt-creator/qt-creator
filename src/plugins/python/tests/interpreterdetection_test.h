// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Python::Internal {

class InterpreterDetectionTest final : public QObject
{
    Q_OBJECT

private slots:
    void testReportsAnEmptySearch();
    void testRegistersDetectedInterpreters();
    void testAssignsAKnownInterpreterToAKit();
    void testAssignsAnInterpreterRegisteredUnderASymlinkTarget();
    void testRegistersOnceWhenSearchPathsResolveToTheSameBinary();
    void testDetectionIgnoresUnappliedRowsOfThePage();
    void testDetectsForAKitWithoutAKitOfItsOwn();
    void testKeepsUnappliedEditsOnASettingsWrite();
    void testFollowsADefaultChangedElsewhere();
    void testRegistersOnceWhenDetectionsOverlap();
    void testKeepsTheDefaultWhenRemovingDetectedInterpreters();
};

QObject *createInterpreterDetectionTest();

} // namespace Python::Internal
