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

#include "concolic/traceeventdetectors.h"
#include "util/loggingutil.h"

#include "tracebuilder.h"

namespace artemis
{

TraceBuilder::TraceBuilder(QObject* parent, bool shouldSummarise) :
    QObject(parent),
    mRecording(false),
    mSummarise(shouldSummarise)
{
}

void TraceBuilder::addDetector(QSharedPointer<TraceEventDetector> detector)
{
    mDetectors.append(detector);
    detector->setTraceBuilder(this);
}

void TraceBuilder::beginRecording()
{
    if(mRecording){
        //Log::fatal("TraceRecorder: Began recording during an existing recording.");
        //exit(1);
        return;
        // TODO: this is to fix issue #63, which seems to be introduced in 1e89249e8c49a94668d94e912a5a1fa17b76910e.
        // It seems that commit has trace recording on every page load (see webkitexecutor in that commit),
        // presumably to have a consistent start point for multiple runs of traces. I need to ask Casper about this
        // before I know this fix is ok.
    }

    mRecording = true;

    // Reset the trace to be empty.
    mTrace = QSharedPointer<TraceNode>();
    mSuccessor = &mTrace;
    mCurrentSummary.clear();

}

void TraceBuilder::endRecording()
{
    if(!mRecording){
        return;
    }

    if(!mCurrentSummary.empty()){
        flushSummary(); // Updates mSuccessor.
    }

    mRecording = false;

    // Finish off the trace with an EndUnknown node.
    *mSuccessor = QSharedPointer<TraceNode>(new TraceEndUnknown());
    mSuccessor = NULL;

}

void TraceBuilder::newNode(QSharedPointer<TraceNode> node, QSharedPointer<TraceNode>* successor)
{
    // ignore the new node unless we are recording a trace.
    if(mRecording){
        // If we have an active summary node running then flush that, then add this new node as a successor.
        if(!mCurrentSummary.empty()){
            flushSummary(); // Updates mSuccessor.
        }

        // Add the new node to the current successor pointer.
        *mSuccessor = node;

        // Update the new successor pointer
        mSuccessor = successor;

        // Notify the GUI (or anyone else) of the new node)
        emit sigAddedNode();
    }
}

void TraceBuilder::newSummaryInfo(TraceConcreteSummarisation::EventType info)
{
    if(mRecording){
        mCurrentSummary.append(info);
    }
}

// Write out a new summary node and update mSuccessor.
void TraceBuilder::flushSummary()
{
    QPair<QList<TraceConcreteSummarisation::EventType>, TraceNodePtr> execution;
    execution.first = mCurrentSummary;

    QSharedPointer<TraceConcreteSummarisation> node(new TraceConcreteSummarisation);
    node->executions.append(execution);

    // Add the new node to the current successor pointer and update the successor pointer.
    *mSuccessor = node;
    mSuccessor = &(node->executions[0].second);

    mCurrentSummary.clear();
}

TraceNodePtr TraceBuilder::trace()
{
    if(mRecording){
        Log::fatal("TraceRecorder: Requested trace during recording.");
        exit(1);
    }

    return mTrace;
}


} // namespace artemis
