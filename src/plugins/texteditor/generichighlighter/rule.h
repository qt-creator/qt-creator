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

#ifndef RULE_H
#define RULE_H

#include <QString>
#include <QList>
#include <QSharedPointer>

namespace TextEditor {
namespace Internal {

class ProgressData;
class HighlightDefinition;

class Rule
{
public:
    Rule(bool consumesNonSpace = true);
    virtual ~Rule();

    void setContext(const QString &context);
    const QString &context() const;

    void setItemData(const QString &itemData);
    const QString &itemData() const;

    void setBeginRegion(const QString &begin);
    const QString &beginRegion() const;

    void setEndRegion(const QString &end);
    const QString &endRegion() const;

    void setLookAhead(const QString &lookAhead);
    bool isLookAhead() const;

    void setFirstNonSpace(const QString &firstNonSpace);
    bool isFirstNonSpace() const;

    void setColumn(const QString &column);
    int column() const;

    void addChild(const QSharedPointer<Rule> &rule);
    const QList<QSharedPointer<Rule> > &children() const;
    bool hasChildren() const;

    void setDefinition(const QSharedPointer<HighlightDefinition> &definition);
    const QSharedPointer<HighlightDefinition> &definition() const;

    bool matchSucceed(const QString &text, const int length, ProgressData *progress);

    Rule *clone() const;

    void progressFinished();

protected:
    bool charPredicateMatchSucceed(const QString &text,
                                   const int length,
                                   ProgressData *progress,
                                   bool (QChar::* predicate)() const) const;
    bool charPredicateMatchSucceed(const QString &text,
                                   const int length,
                                   ProgressData *progress,
                                   bool (*predicate)(const QChar &)) const;

    bool matchCharacter(const QString &text,
                        const int length,
                        ProgressData *progress,
                        const QChar &c,
                        bool saveRestoreOffset = true) const;
    bool matchEscapeSequence(const QString &text,
                             const int length,
                             ProgressData *progress,
                             bool saveRestoreOffset = true) const;
    bool matchOctalSequence(const QString &text,
                            const int length,
                            ProgressData *progress,
                            bool saveRestoreOffset = true) const;
    bool matchHexSequence(const QString &text,
                          const int length,
                          ProgressData *progress,
                          bool saveRestoreOffset = true) const;

    static const QLatin1Char kBackSlash;
    static const QLatin1Char kUnderscore;
    static const QLatin1Char kDot;
    static const QLatin1Char kPlus;
    static const QLatin1Char kMinus;
    static const QLatin1Char kZero;
    static const QLatin1Char kQuote;
    static const QLatin1Char kSingleQuote;
    static const QLatin1Char kQuestion;
    static const QLatin1Char kX;
    static const QLatin1Char kA;
    static const QLatin1Char kB;
    static const QLatin1Char kE;
    static const QLatin1Char kF;
    static const QLatin1Char kN;
    static const QLatin1Char kR;
    static const QLatin1Char kT;
    static const QLatin1Char kV;
    static const QLatin1Char kOpeningBrace;
    static const QLatin1Char kClosingBrace;

private:
    virtual bool doMatchSucceed(const QString &text, const int length, ProgressData *progress) = 0;

    virtual Rule *doClone() const = 0;

    virtual void doProgressFinished() {}

    template <class predicate_t>
    bool predicateMatchSucceed(const QString &text,
                               const int length,
                               ProgressData *progress,
                               const predicate_t &p) const;

    QString m_context;
    QString m_itemData;
    QString m_beginRegion;
    QString m_endRegion;
    bool m_lookAhead;
    bool m_firstNonSpace;
    int m_column;
    bool m_consumesNonSpace;

    QList<QSharedPointer<Rule> > m_children;

    // Rules are represented within contexts. However, they have their own definition because
    // of externally included rules.
    QSharedPointer<HighlightDefinition> m_definition;
};

} // namespace Internal
} // namespace TextEditor

#endif // RULE_H
