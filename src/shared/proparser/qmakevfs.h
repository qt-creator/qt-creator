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

#ifndef QMAKEVFS_H
#define QMAKEVFS_H

#include "qmake_global.h"

# include <qiodevice.h>
#ifndef PROEVALUATOR_FULL
# include <qhash.h>
# include <qstring.h>
# ifdef PROEVALUATOR_THREAD_SAFE
#  include <qmutex.h>
# endif
#endif

QT_BEGIN_NAMESPACE

class QMAKE_EXPORT QMakeVfs
{
public:
    QMakeVfs();

    bool writeFile(const QString &fn, QIODevice::OpenMode mode, const QString &contents, QString *errStr);
    bool readFile(const QString &fn, QString *contents, QString *errStr);
    bool exists(const QString &fn);

#ifndef PROEVALUATOR_FULL
    void invalidateCache();
    void invalidateContents();
#endif

private:
#ifndef PROEVALUATOR_FULL
# ifdef PROEVALUATOR_THREAD_SAFE
    QMutex m_mutex;
# endif
    QHash<QString, QString> m_files;
    QString m_magicMissing;
    QString m_magicExisting;
#endif
};

QT_END_NAMESPACE

#endif // QMAKEVFS_H
