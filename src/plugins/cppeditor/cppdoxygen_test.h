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

#pragma once

#include "cppeditortestcase.h"

#include <texteditor/commentssettings.h>

#include <QObject>
#include <QScopedPointer>

namespace CppEditor {
namespace Internal {
namespace Tests {

/// Tests for inserting doxygen comments.
class DoxygenTest: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanTestCase();
    void cleanup();

    void testBasic_data();
    void testBasic();

    void testWithMacroFromHeaderBeforeFunction();

    void testNoLeadingAsterisks_data();
    void testNoLeadingAsterisks();

private:
    void verifyCleanState() const;
    void runTest(const QByteArray &original,
                 const QByteArray &expected,
                 TextEditor::CommentsSettings *settings = 0,
                 const TestDocuments &includedHeaderDocuments = TestDocuments());

    QScopedPointer<TextEditor::CommentsSettings> oldSettings;
};

} // namespace Tests
} // namespace Internal
} // namespace CppEditor
