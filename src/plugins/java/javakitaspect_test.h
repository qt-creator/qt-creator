// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef WITH_TESTS

#include <QHash>
#include <QObject>
#include <QVariant>

namespace ProjectExplorer { class Kit; }

namespace Java::Internal {

QObject *createJavaKitAspectTest();

// The JDK every kit of the session names, and whether its field is theirs to
// edit. A test that changes the list of the JDKs of the machine has the
// factory bring every kit in line with the list it leaves behind, so a test
// that runs against the settings of whoever started it puts the kits back.
struct JdkOfAKit
{
    // Whether the key was there at all: a kit set to "None" holds it with
    // nothing in it, which is a choice of the user's, while a kit that has
    // none is one the default is written into.
    bool hasValue = false;
    QVariant value;
    // And whether the JDK it holds is one it was given rather than one the
    // user picked, which is what decides whether a plugin that comes to
    // speak for the kit takes it over.
    bool hasMark = false;
    QVariant mark;
    bool sticky = false;
    bool isMutable = false;
};

QHash<ProjectExplorer::Kit *, JdkOfAKit> theJdksOfTheKits();
void putTheJdksOfTheKitsBack(const QHash<ProjectExplorer::Kit *, JdkOfAKit> &jdks);

} // namespace Java::Internal

#endif // WITH_TESTS
