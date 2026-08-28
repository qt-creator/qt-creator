// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qdocconfig.h"

#include <utils/aspects.h>
#include <utils/filepath.h>

#include <QHash>
#include <QList>
#include <QString>

#include <optional>

namespace QDoc::Internal {

enum class RuleLevel { Off, Hint, Info, Warning, Error };

class Rule
{
public:
    QString id;
    RuleLevel level = RuleLevel::Warning;
    QString title;
};

// Every diagnostic carries a rule id, and each rule has a level the user can change,
// so one noisy check can be silenced without losing the rest. A default is chosen from
// how QDoc treats the problem and how often it fires in Qt.
const QList<Rule> &ruleTable();

class QDocSettings final : public Utils::AspectContainer
{
public:
    QDocSettings();

    RuleLevel levelOf(const QString &ruleId) const;
    void setLevelOf(const QString &ruleId, RuleLevel level);
    bool isRuleEnabled(const QString &ruleId) const;
    // The level a problem is reported at, or nothing when it is dropped. A warning
    // the .qdocconf silences through spurious is worth seeing, so by default it is
    // reported one step quieter rather than hidden.
    std::optional<RuleLevel> resolve(const QString &ruleId, bool suppressed) const;

    Utils::BoolAspect diagnosticsEnabled{this};
    Utils::SelectionAspect suppressedByConfig{this};

private:
    QHash<QString, Utils::SelectionAspect *> m_ruleLevels;
};

QDocSettings &settings();

// Publishes the problems of one document, and of the .qdocconf it was rendered
// against, in the Problems pane.
void updateDiagnostics(const Utils::FilePath &file,
                       const QList<Problem> &problems,
                       const DocContext &context);
void clearDiagnostics(const Utils::FilePath &file);

void setupQDocLinter();

#ifdef WITH_TESTS
QObject *createQDocLinterTest();
#endif

} // namespace QDoc::Internal
