/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef BAZAAREDITOR_H
#define BAZAAREDITOR_H

#include <vcsbase/vcsbaseeditor.h>

#include <QtCore/QRegExp>

namespace Bazaar {
namespace Internal {

class BazaarEditor : public VCSBase::VCSBaseEditorWidget
{
    Q_OBJECT
public:
    explicit BazaarEditor(const VCSBase::VCSBaseEditorParameters *type, QWidget *parent);

private:
    virtual QSet<QString> annotationChanges() const;
    virtual QString changeUnderCursor(const QTextCursor &cursor) const;
    virtual VCSBase::DiffHighlighter *createDiffHighlighter() const;
    virtual VCSBase::BaseAnnotationHighlighter *createAnnotationHighlighter(const QSet<QString> &changes) const;
    virtual QString fileNameFromDiffSpecification(const QTextBlock &diffFileSpec) const;

    const QRegExp m_exactChangesetId;
    const QRegExp m_diffFileId;
};

} // namespace Internal
} // namespace Bazaar
#endif // BAZAAREDITOR_H
