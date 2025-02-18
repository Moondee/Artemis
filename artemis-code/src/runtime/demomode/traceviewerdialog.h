/*
 * Copyright 2012 Aarhus University
 *
 * Licensed under the GNU General Public License, Version 3 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *          http://www.gnu.org/licenses/gpl-3.0.html
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <QtGui>
#include "concolic/executiontree/tracenodes.h"
#include "concolic/executiontree/tracevisitor.h"


#ifndef TRACEVIEWERDIALOG_H
#define TRACEVIEWERDIALOG_H

namespace artemis
{



class TraceViewerDialog : public QDialog, public TraceVisitor
{
    Q_OBJECT

public:
    TraceViewerDialog(TraceNodePtr trace, QWidget* parent = 0);


    // Visitor part used to populate the GUI display of this trace.
    virtual void visit(TraceNode* node);
    virtual void visit(TraceConcreteBranch* node);
    virtual void visit(TraceSymbolicBranch* node);
    virtual void visit(TraceUnexplored* node);
    virtual void visit(TraceAlert* node);
    virtual void visit(TraceDomModification* node);
    virtual void visit(TracePageLoad* node);
    virtual void visit(TraceMarker* node);
    virtual void visit(TraceFunctionCall* node);
    virtual void visit(TraceConcreteSummarisation* node);
    virtual void visit(TraceEndSuccess* node);
    virtual void visit(TraceEndFailure* node);
    virtual void visit(TraceEndUnknown* node);

private:
    QListWidget* mNodeList;
};


} //namespace artemis

#endif // TRACEVIEWERDIALOG_H
