// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "pythonsettings_test.h"

#include "../pythonsettings.h"

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QTest>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

void PythonSettingsTest::testDropsADefaultThatIsGone()
{
    const QList<Interpreter> before = PythonSettings::interpreters();
    const QString defaultIdBefore = PythonSettings::defaultInterpreterId();

    // Not the interpreter from PATH, so it cannot be picked as the replacement default.
    const Interpreter extra("python-settings-test-id",
                            "Python Settings Test",
                            FilePath::fromUserInput("/does/not/exist/python3"));
    PythonSettings::setInterpreter(before + QList<Interpreter>{extra}, extra.id);
    QCOMPARE(PythonSettings::defaultInterpreterId(), extra.id);

    PythonSettings::setInterpreter(before, extra.id);
    const QString defaultId = PythonSettings::defaultInterpreterId();
    QVERIFY(defaultId.isEmpty()
            || Utils::contains(PythonSettings::interpreters(),
                               Utils::equal(&Interpreter::id, defaultId)));

    PythonSettings::setInterpreter(before, defaultIdBefore);
}

QObject *createPythonSettingsTest()
{
    return new PythonSettingsTest;
}

} // namespace Python::Internal

#endif // WITH_TESTS
