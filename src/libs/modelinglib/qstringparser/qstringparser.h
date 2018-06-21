/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include <QString>
#include <QList>

class QStringParser
{
private:
    class Parser
    {
    private:
        class Node
        {
        public:
            virtual ~Node() { }
            virtual bool accept(Parser &visitor, int *index) = 0;
        };

        template<typename V>
        class RefNode : public Node
        {
        public:
            explicit RefNode(V &v) : m_v(v) { }
            bool accept(Parser &visitor, int *index) override { return visitor.visit(this, index); }
            V &ref() const { return m_v; }
        private:
            V &m_v;
        };

        template<class U, typename V>
        class SetterNode : public Node
        {
        public:
            SetterNode(U &u, void (U::*setter)(V)) : m_object(u), m_setter(setter) { }
            bool accept(Parser &visitor, int *index)  override{ return visitor.visit(this, index); }
            U &object() const { return m_object; }
            void (U::*setter() const)(V) { return m_setter; }
        private:
            U &m_object;
            void (U::*m_setter)(V) = nullptr;
        };

    public:
        Parser(const QString &source, const QString &pattern);
        ~Parser();

        template<typename V>
        Parser &arg(V &v)
        {
            m_nodes.append(new RefNode<V>(v));
            return *this;
        }

        template<class U, typename V>
        Parser &arg(U &u, void (U::*setter)(V))
        {
            m_nodes.append(new SetterNode<U, V>(u, setter));
            return *this;
        }

        bool failed();

    private:
        bool scan(int *i, int *index);
        bool scan(double *d, int *index);

        template<typename V>
        bool visit(RefNode<V> *node, int *index)
        {
            V v = nullptr;
            if (!scan(&v, index))
                return false;
            node->ref() = v;
            return true;
        }

        template<class U, typename V>
        bool visit(SetterNode<U, V> *node, int *index)
        {
            V v = 0;
            if (!scan(&v, index))
                return false;
            (node->object().*(node->setter()))(v);
            return true;
        }

        template<class U, typename V>
        bool visit(SetterNode<U, const V &> *node, int *index)
        {
            V v = 0;
            if (!scan(&v, index))
                return false;
            (node->object().*(node->setter()))(v);
            return true;
        }

        void evaluate();

        const QString m_source;
        const QString m_pattern;
        bool m_isEvaluated = false;
        bool m_evaluationFailed = false;
        QList<Node *> m_nodes;
    };

public:
    explicit QStringParser(const QString &source);
    ~QStringParser();

    Parser parse(const QString &pattern);

private:
    const QString m_source;
};
