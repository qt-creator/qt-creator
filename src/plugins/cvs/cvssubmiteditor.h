// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbasesubmiteditor.h>

#include <QPair>
#include <QStringList>

namespace Cvs::Internal {

class CvsSubmitEditor : public VcsBase::VcsBaseSubmitEditor
{
    Q_OBJECT

public:
    CvsSubmitEditor();

    enum State { LocallyAdded, LocallyModified, LocallyRemoved };
    // A list of state indicators and file names.
    typedef QPair<State, QString> StateFilePair;
    typedef QList<StateFilePair> StateFilePairs;

    void setStateList(const StateFilePairs &statusOutput);

private:
    QString stateName(State st) const;

    const QString m_msgAdded;
    const QString m_msgRemoved;
    const QString m_msgModified;
};

} // Cvs::Internal
