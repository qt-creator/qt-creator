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

#ifndef TESTBAUHAUS_H
#define TESTBAUHAUS_H

#include <QObject>

#include <QtTest>


class TestBauhaus : public QObject
{
    Q_OBJECT
public:
    TestBauhaus();

private slots:
    void loadExamples_data();
    void loadExamples();
    void loadDemos_data();
    void loadDemos();
    void loadCreator_data();
    void loadCreator();
private:
    bool loadFile(const QString &file);

    QString m_executable;
    QString m_creatorDir;
    QString m_qtDir;
};

#endif // TESTBAUHAUS_H
