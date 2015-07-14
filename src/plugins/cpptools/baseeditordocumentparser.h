/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASEEDITORDOCUMENTPARSER_H
#define BASEEDITORDOCUMENTPARSER_H

#include "cppmodelmanager.h"
#include "cpptools_global.h"
#include "cppworkingcopy.h"

#include <QObject>

namespace CppTools {

class CPPTOOLS_EXPORT BaseEditorDocumentParser : public QObject
{
    Q_OBJECT

public:
    static BaseEditorDocumentParser *get(const QString &filePath);

    struct Configuration {
        bool stickToPreviousProjectPart = true;
        bool usePrecompiledHeaders = false;
        QByteArray editorDefines;
        ProjectPart::Ptr manuallySetProjectPart;
    };

public:
    BaseEditorDocumentParser(const QString &filePath);
    virtual ~BaseEditorDocumentParser();

    QString filePath() const;
    Configuration configuration() const;
    void setConfiguration(const Configuration &configuration);

    struct CPPTOOLS_EXPORT InMemoryInfo {
        InMemoryInfo(bool withModifiedFiles);

        WorkingCopy workingCopy;
        Utils::FileNameList modifiedFiles;
    };
    void update(const InMemoryInfo &info);

    ProjectPart::Ptr projectPart() const;

protected:
    struct State {
        QByteArray editorDefines;
        ProjectPart::Ptr projectPart;
    };
    State state() const;
    void setState(const State &state);

    static ProjectPart::Ptr determineProjectPart(const QString &filePath,
                                                 const Configuration &config,
                                                 const State &state);

    mutable QMutex m_stateAndConfigurationMutex;

private:
    virtual void updateHelper(const InMemoryInfo &inMemoryInfo) = 0;

    const QString m_filePath;
    Configuration m_configuration;
    State m_state;
    mutable QMutex m_updateIsRunning;
};

} // namespace CppTools

#endif // BASEEDITORDOCUMENTPARSER_H
