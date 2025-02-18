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

#include "randomaccesssearch.h"
#include <assert.h>

namespace artemis {

RandomAccessSearch::RandomAccessSearch(TraceNodePtr tree, AbstractSelectorPtr selector, uint searchBudget) :
    mTree(tree),
    mSelector(selector),
    mBudget(searchBudget),
    mUnlimitedBudget(searchBudget == 0),
    mNotifiedFirstTrace(false)
{
}

bool RandomAccessSearch::chooseNextTarget()
{
    // If we have never notified the selector about the initial tree, do this now.
    if(!mNotifiedFirstTrace) {
        mSelector->newTraceAdded(TraceNodePtr(), false, mTree, mTree);
        mNotifiedFirstTrace = true;
    }

    // If the budget is exhausted, the search is over.
    if(!(mUnlimitedBudget || mBudget > 0)) {
        mTarget = ExplorationDescriptor();
        return false;
    }
    mBudget--;

    // Call analyseTree to get the set of possible explorations.
    analyseTree();

    // If there are none, then the search is over.
    if (mPossibleExplorations.empty()) {
        mTarget = ExplorationDescriptor();
        return false;
    }

    // Call chooseNext() to choose one of these to explore.
    ExplorationDescriptor choice = mSelector->nextTarget(mPossibleExplorations);

    // This must be a valid choice.
    assert(mPossibleExplorations.contains(choice));

    // Calculate the DOM constraints (PC is calculated from mTarget on-the-fly to account for branches marked as difficult).
    mTarget = choice;
    mTargetDomConstraints = calculateDomConstraints(mTarget);

    return true;
}

PathConditionPtr RandomAccessSearch::calculatePC(ExplorationDescriptor target)
{
    // Search upwards through the tree using mBranchParents to build the PathBranchList.

    QPair<TraceSymbolicBranchPtr, bool> current = QPair<TraceSymbolicBranchPtr, bool>(target.branch, target.branchDirection);
    PathBranchList branches;

    // Null parent marks the first symbolic branch on each trace.
    while (!current.first.isNull()) {
        qDebug() << "Adding" << current << "Difficult:" << current.first->isDifficult();

        // Add the current node's condition to the PC.
        // We prepend so the resulting PC is given in root to leaf order, which is required.
        branches.prepend(PathBranch(current.first.data(), current.second));

        // Move to the next node.
        assert(mBranchParents.contains(current.first));
        current = mBranchParents.value(current.first);
    }

    return PathCondition::createFromBranchList(branches);
}

QSet<SelectRestriction> RandomAccessSearch::calculateDomConstraints(ExplorationDescriptor target)
{
    // First find the initial marker above this branch.
    assert(mBranchParentMarkers.contains(target.branch));
    TraceMarkerPtr current = mBranchParentMarkers.value(target.branch);

    // Search upwards through the tree using mMarkerParents to build the DOM constraints.
    QSet<SelectRestriction> restrictions;

    // Null parent markes the first marker node on each trace.
    while (!current.isNull()) {
        // Add the current marker's condition.
        if (current->isSelectRestriction) {
            restrictions.insert(current->selectRestriction);
        }

        // Move to the next node.
        assert(mMarkerParents.contains(current));
        current = mMarkerParents.value(current);
    }

    return restrictions;
}




PathConditionPtr RandomAccessSearch::getTargetPC()
{
    return calculatePC(mTarget);
}

QSet<SelectRestriction> RandomAccessSearch::getTargetDomConstraints()
{
    return mTargetDomConstraints;
}

bool RandomAccessSearch::overUnexploredNode()
{
    // Only call after a successful call to chooseNextTarget().
    assert(!mTarget.branch.isNull());

    // Use mTarget to check if we explored the correct area.
    if (mTarget.branchDirection) {
        return isImmediatelyNotAttempted(mTarget.branch->getTrueBranch());
    } else {
        return isImmediatelyNotAttempted(mTarget.branch->getFalseBranch());
    }
}

void RandomAccessSearch::markExplorationIndex(uint index)
{
    // Only call after a successful call to chooseNextTarget().
    assert(!mTarget.branch.isNull());

    if (mTarget.branchDirection) {
        mTarget.branch->markExploration(index, true);
    } else {
        mTarget.branch->markExploration(index, false);
    }
}

void RandomAccessSearch::markNodeUnsat()
{
    // Only call after a successful call to chooseNextTarget().
    assert(!mTarget.branch.isNull());

    // Assuming overUnexploredNode(), mark this target as UNSAT.
    if (mTarget.branchDirection) {
        assert(isImmediatelyNotAttempted(mTarget.branch->getTrueBranch()));
        mTarget.branch->setTrueBranch(TraceUnexploredUnsat::getInstance());
    } else {
        assert(isImmediatelyNotAttempted(mTarget.branch->getFalseBranch()));
        mTarget.branch->setFalseBranch(TraceUnexploredUnsat::getInstance());
    }

    // Notify the selector
    mSelector->newUnsat(mTarget);
}

void RandomAccessSearch::markNodeUnsolvable()
{
    // Only call after a successful call to chooseNextTarget().
    assert(!mTarget.branch.isNull());

    // Assuming overUnexploredNode(), mark this target as Unsolvable.
    if (mTarget.branchDirection) {
        assert(isImmediatelyNotAttempted(mTarget.branch->getTrueBranch()));
        mTarget.branch->setTrueBranch(TraceUnexploredUnsolvable::getInstance());
    } else {
        assert(isImmediatelyNotAttempted(mTarget.branch->getFalseBranch()));
        mTarget.branch->setFalseBranch(TraceUnexploredUnsolvable::getInstance());
    }

    // Notify the selector
    mSelector->newUnsolvable(mTarget);
}

void RandomAccessSearch::markNodeMissed()
{
    // Only call after a successful call to chooseNextTarget().
    assert(!mTarget.branch.isNull());

    // Assuming overUnexploredNode(), mark this target as Missed.
    if (mTarget.branchDirection) {
        assert(isImmediatelyNotAttempted(mTarget.branch->getTrueBranch()));
        mTarget.branch->setTrueBranch(TraceUnexploredMissed::getInstance());
    } else {
        assert(isImmediatelyNotAttempted(mTarget.branch->getFalseBranch()));
        mTarget.branch->setFalseBranch(TraceUnexploredMissed::getInstance());
    }

    // Notify the selector
    mSelector->newMissed(mTarget);
}





// Analyse the tree and set the following:
// mPossibleExplorations, mBranchParents, mMarkerParents, mBranchParentMarkers
void RandomAccessSearch::analyseTree()
{
    mPossibleExplorations.clear();
    mBranchParents.clear();
    mMarkerParents.clear();
    mBranchParentMarkers.clear();

    mCurrentBranchParent = TraceSymbolicBranchPtr();
    // mCurrentBranchParentDirection
    mCurrentMarkerParent = TraceMarkerPtr();
    mCurrentSymbolicDepth = 0;

    // All calls to the visitor must go through analyseNode, so that mThisNode is always valid.
    analyseNode(mTree);
}

void RandomAccessSearch::analyseNode(TraceNodePtr node)
{
    mThisNode = node;
    node->accept(this);
}


// Called whenever a new trace (suffix) is added to the tree.
void RandomAccessSearch::slNewTraceAdded(TraceNodePtr parent, int direction, TraceNodePtr suffix, TraceNodePtr fullTrace)
{
    mSelector->newTraceAdded(parent, direction, suffix, fullTrace);
}





// The visitor part, used to analyse the full tree.

void RandomAccessSearch::visit(TraceNode *node)
{
    Log::fatal("Error: Reached a node of unknown type while searching the tree (RandomAccessSearch).");
    exit(1);
}

void RandomAccessSearch::visit(TraceConcreteBranch *node)
{
    // Just recurse on both children.

    // But reset the visitor state correctly for the second one.
    TraceSymbolicBranchPtr currentBranchParent = mCurrentBranchParent;
    bool currentBranchParentDirection = mCurrentBranchParentDirection;
    TraceMarkerPtr currentMarkerParent = mCurrentMarkerParent;
    unsigned int currentSymbolicDepth = mCurrentSymbolicDepth;

    analyseNode(node->getFalseBranch());

    mCurrentBranchParent = currentBranchParent;
    mCurrentBranchParentDirection = currentBranchParentDirection;
    mCurrentMarkerParent = currentMarkerParent;
    mCurrentSymbolicDepth = currentSymbolicDepth;

    analyseNode(node->getTrueBranch());
}

void RandomAccessSearch::visit(TraceSymbolicBranch *node)
{
    TraceSymbolicBranchPtr thisSymBranch = mThisNode.dynamicCast<TraceSymbolicBranch>();
    assert(!thisSymBranch.isNull());

    // Keep the current marker parent so we can reset it correctly for both branches.
    TraceMarkerPtr currentMarkerParent = mCurrentMarkerParent;
    unsigned int currentSymbolicDepth = mCurrentSymbolicDepth + 1;

    // Update the tables.
    QPair<TraceSymbolicBranchPtr, bool> parent = QPair<TraceSymbolicBranchPtr, bool>(mCurrentBranchParent, mCurrentBranchParentDirection);
    mBranchParents.insert(thisSymBranch, parent);
    mBranchParentMarkers.insert(thisSymBranch, mCurrentMarkerParent);

    // If either child is unexplored, this is a new exploration target.
    // Otherwise, analyse the children.
    if (isImmediatelyNotAttempted(node->getFalseBranch())) {
        ExplorationDescriptor explore;
        explore.branch = thisSymBranch;
        explore.branchDirection = false;
        explore.symbolicDepth = currentSymbolicDepth;

        // If this node is difficult, replace the unexplored child with CNS and ignore it.
        if (node->isDifficult()) {
            node->setFalseBranch(TraceUnexploredUnsolvable::getInstance());
            mSelector->newUnsolvable(explore);
        } else {
            // Otherwise, add it to the exploration list.
            mPossibleExplorations.append(explore);
        }
    } else {
        mCurrentBranchParent = thisSymBranch;
        mCurrentBranchParentDirection = false;
        mCurrentMarkerParent = currentMarkerParent;
        mCurrentSymbolicDepth = currentSymbolicDepth;
        analyseNode(node->getFalseBranch());
    }

    if (isImmediatelyNotAttempted(node->getTrueBranch())) {
        ExplorationDescriptor explore;
        explore.branch = thisSymBranch;
        explore.branchDirection = true;
        explore.symbolicDepth = currentSymbolicDepth;

        // If this node is difficult, replace the unexplored child with CNS and ignore it.
        if (node->isDifficult()) {
            node->setTrueBranch(TraceUnexploredUnsolvable::getInstance());
            mSelector->newUnsolvable(explore);
        } else {
            // Otherwise, add it to the exploration list.
            mPossibleExplorations.append(explore);
        }
    } else {
        mCurrentBranchParent = thisSymBranch;
        mCurrentBranchParentDirection = true;
        mCurrentMarkerParent = currentMarkerParent;
        mCurrentSymbolicDepth = currentSymbolicDepth;
        analyseNode(node->getTrueBranch());
    }

}

void RandomAccessSearch::visit(TraceConcreteSummarisation *node)
{
    // Just recurse on all children.

    // But reset the visitor state correctly for the second one.
    TraceSymbolicBranchPtr currentBranchParent = mCurrentBranchParent;
    bool currentBranchParentDirection = mCurrentBranchParentDirection;
    TraceMarkerPtr currentMarkerParent = mCurrentMarkerParent;
    unsigned int currentSymbolicDepth = mCurrentSymbolicDepth;

    foreach(TraceConcreteSummarisation::SingleExecution ex, node->executions) {
        mCurrentBranchParent = currentBranchParent;
        mCurrentBranchParentDirection = currentBranchParentDirection;
        mCurrentMarkerParent = currentMarkerParent;
        mCurrentSymbolicDepth = currentSymbolicDepth;

        analyseNode(ex.second);
    }
}

void RandomAccessSearch::visit(TraceMarker *node)
{
    TraceMarkerPtr thisMarker = mThisNode.dynamicCast<TraceMarker>();
    assert(!thisMarker.isNull());

    // Update the tables
    mMarkerParents.insert(thisMarker, mCurrentMarkerParent);

    // Continue
    mCurrentMarkerParent = thisMarker;

    analyseNode(node->next);
}

void RandomAccessSearch::visit(TraceUnexplored *node)
{
    // All types of unexplored node are ignored.
    // Possible explorations are aded to the list by their parent branch and are not visited.
}

void RandomAccessSearch::visit(TraceAnnotation *node)
{
    // Ignore and continue.
    analyseNode(node->next);
}

void RandomAccessSearch::visit(TraceEnd *node)
{
    // Ignore.
}





} // namespace artemis
