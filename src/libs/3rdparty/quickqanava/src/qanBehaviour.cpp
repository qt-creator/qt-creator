/*
 Copyright (c) 2008-2024, Benoit AUTHEMAN All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author or Destrat.io nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the QuickQanava software library.
//
// \file	qanBehaviour.cpp
// \author	benoit@destrat.io
// \date	2016 04 04
//-----------------------------------------------------------------------------

// QuickQanava headers
#include "./qanBehaviour.h"
#include "./qanNode.h"
#include "./qanEdge.h"

namespace qan { // ::qan

/* NodeBehaviour Management *///-----------------------------------------------
NodeBehaviour::NodeBehaviour(const std::string& name, QObject* parent) :
    QObject{parent}
{
    gtpo::node_observer<qan::Node, qan::Edge>::setName(name);
}
//-----------------------------------------------------------------------------

/* Behaviour Host Management *///----------------------------------------------
void    NodeBehaviour::setHost(qan::Node* host)
{
    if ( _host != host ) {
        _host = host;
        emit hostChanged();
    }
}
//-----------------------------------------------------------------------------

/* Notification Interface *///-------------------------------------------------
void    NodeBehaviour::on_in_node_inserted(qan::Node& target, qan::Node& inNode, const qan::Edge& edge) noexcept
{
    Q_UNUSED(target);
    inNodeInserted(inNode, const_cast<qan::Edge&>(edge));
}

void    NodeBehaviour::on_in_node_removed(qan::Node& target, qan::Node& inNode, const qan::Edge& edge) noexcept
{
    Q_UNUSED(target);
    inNodeRemoved(inNode, const_cast<qan::Edge&>(edge));
}

void    NodeBehaviour::on_out_node_inserted(qan::Node& target, qan::Node& outNode, const qan::Edge& edge) noexcept
{
    Q_UNUSED(target);
    outNodeInserted(outNode, const_cast<qan::Edge&>(edge));
}

void    NodeBehaviour::on_out_node_removed(qan::Node& target, qan::Node& outNode, const qan::Edge& edge) noexcept
{
    Q_UNUSED(target);
    outNodeRemoved(outNode, const_cast<qan::Edge&>(edge));
}
//-----------------------------------------------------------------------------

} // ::qan
