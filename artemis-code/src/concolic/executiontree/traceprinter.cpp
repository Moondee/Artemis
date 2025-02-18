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


#include "traceprinter.h"
#include "util/loggingutil.h"
#include "concolic/solver/expressionvalueprinter.h"

namespace artemis
{



void TerminalTracePrinter::printTraceTree(TraceNodePtr root)
{
    // Reset the current state.
    mCurrentTree.clear();
    mCompletedLeftTrees.clear();

    // Run the visitor.
    root->accept(this);

    // Now mCurrentTree should be set to the entire result.
    Log::info("");
    QString leftPadding = " ";
    Log::info((QString(mCurrentTree.connector - 2 + leftPadding.length(), ' ') + "START").toStdString());
    Log::info((QString(mCurrentTree.connector + leftPadding.length(), ' ') + "|").toStdString());
    foreach(QString line, mCurrentTree.lines){
        Log::info((leftPadding + line).toStdString());
    }
    Log::info("");

    // Check there is nothing left in the list of intermediate trees...
    if(!mCompletedLeftTrees.empty()){
        Log::info("ERROR: tree printing did not print the entire tree.");
    }

}


void TerminalTracePrinter::visit(TraceNode* node)
{
    // Should never be reached.
    // There is nowhere we can go from here as general nodes don't have a successor!
    Log::fatal("Trace printer visited a node of unknown type.");
    exit(1);
}

void TerminalTracePrinter::visit(TraceConcreteBranch *node)
{
    // First process the left tree.
    node->getFalseBranch()->accept(this);

    // Now mCurrentTree represents the left subtree, so copy it into mCompletedLeftSubtrees.
    mCompletedLeftTrees.push(mCurrentTree);

    // Now clear the current tree and process the right subtree.
    mCurrentTree.clear();
    node->getTrueBranch()->accept(this);

    // Now we have both trees, so join them.
    addBranch("Branch");
}

void TerminalTracePrinter::visit(TraceSymbolicBranch* node)
{
    // First process the left tree.
    node->getFalseBranch()->accept(this);

    // Now mCurrentTree represents the left subtree, so copy it into mCompletedLeftSubtrees.
    mCompletedLeftTrees.push(mCurrentTree);

    // Now clear the current tree and process the right subtree.
    mCurrentTree.clear();
    node->getTrueBranch()->accept(this);


    QList<QString> branch;
    branch.append("Branch");
    ExpressionValuePrinter exprPrinter;
    node->getSymbolicCondition()->accept(&exprPrinter);
    branch.append(QString(exprPrinter.getResult().c_str()));

    // Now we have both trees, so join them.
    addBranch(branch);
}

void TerminalTracePrinter::visit(TraceUnexplored* node)
{
    addSingleValue("???");
}

void TerminalTracePrinter::visit(TraceAlert* node)
{
    node->next->accept(this);
    addSingleValue("Alert");
}

void TerminalTracePrinter::visit(TraceDomModification* node)
{
    node->next->accept(this);
    addSingleValue("DOM");
}

void TerminalTracePrinter::visit(TracePageLoad* node)
{
    node->next->accept(this);
    addSingleValue("Load");
}

void TerminalTracePrinter::visit(TraceMarker *node)
{
    node->next->accept(this);
    addSingleValue(QString("Marker: %1").arg(node->label));
}

void TerminalTracePrinter::visit(TraceFunctionCall* node)
{
    node->next->accept(this);
    QList<QString> lines;
    lines.append("Function");
    lines.append(QString("\"%1\"").arg(node->name));
    addSingleValue(lines);
}

void TerminalTracePrinter::visit(TraceConcreteSummarisation *node)
{
    if(node->executions.length() > 0) {
        node->executions[0].second->accept(this);
        addSingleValue("Concrete Execution");

        if(node->executions.length() > 1) {
            addSingleValue("!!! Note: missing concrete executions here. !!!");
        }
    }
}

void TerminalTracePrinter::visit(TraceEndSuccess* node)
{
    // Nowhere to go from here.
    addSingleValue("End (S)");
}

void TerminalTracePrinter::visit(TraceEndFailure* node)
{
    // Nowhere to go from here.
    addSingleValue("End (F)");
}

void TerminalTracePrinter::visit(TraceEndUnknown* node)
{
    // Nowhere to go from here.
    addSingleValue("End (?)");
}


// Add a single "pass-through" node to the top of the current tree.
void TerminalTracePrinter::addSingleValue(QString nodeText)
{
    QList<QString> nodeLines;
    nodeLines.append(nodeText);
    addSingleValue(nodeLines);
}

// Add a single "pass-through" node to the top of the current tree.
void TerminalTracePrinter::addSingleValue(QList<QString> nodeText)
{
    // Makes sure all the lines are the same length
    nodeText = processNodeTextLines(nodeText);
    int nodeTextLength = nodeText.at(0).length();

    if(mCurrentTree.lines.empty()){
        // Then we start a new tree
        mCurrentTree.lines.append(nodeText);
        mCurrentTree.width = nodeTextLength;
        mCurrentTree.connector = mCurrentTree.width / 2;

    }else{
        // Then we add to an existing tree.
        if(nodeTextLength <= mCurrentTree.width){
            // Pad the node text (making sure it will be exactly mCurrentTree.width chars in all)
            QMutableListIterator<QString> it(nodeText);
            while(it.hasNext()){
                it.setValue(padToConnector(it.next(), mCurrentTree.connector, mCurrentTree.width));
            }

        }else{
            QString leftPad = QString((nodeTextLength - mCurrentTree.width) / 2, ' ');
            QString rightPad = QString(nodeTextLength - mCurrentTree.width - leftPad.length(), ' ');
            // Pad the current tree (every line must be exaclty nodeTextLength chars in all)
            QMutableListIterator<QString> it(mCurrentTree.lines);
            while(it.hasNext()){
                QString line = it.next();
                line.prepend(leftPad);
                line.append(rightPad);
                it.setValue(line);
            }
            // Reset the width and connector position.
            mCurrentTree.width = nodeTextLength;
            mCurrentTree.connector += leftPad.length();
        }

        // Insert the connector and then the node text itself.
        QString connectorLine = QString(mCurrentTree.width, ' ');
        connectorLine.replace(mCurrentTree.connector, 1, '|');
        mCurrentTree.lines.prepend(connectorLine);

        QMutableListIterator<QString> it(nodeText);
        it.toBack();
        while(it.hasPrevious()){
            mCurrentTree.lines.prepend(it.previous());
        }

        // Now mCurrentTree is properly formatted (an exact rectangle with all lines of equal length).
    }
}


// Merge two trees into a new tree.
void TerminalTracePrinter::addBranch(QString nodeText)
{
    QList<QString> nodeLines;
    nodeLines.append(nodeText);
    addBranch(nodeLines);
}

// Merge two trees into a new tree.
void TerminalTracePrinter::addBranch(QList<QString> nodeText)
{
    // We already have the left subtree in the stack and the right (current) subtree.
    // Now we need to join them and create the branch.
    PrintableTree leftTree = mCompletedLeftTrees.pop();
    PrintableTree rightTree = mCurrentTree;
    mCurrentTree.clear();

    // Makes sure all the lines are the same length
    nodeText = processNodeTextLines(nodeText);
    int nodeTextLength = nodeText.at(0).length();

    QString spacing = QString(2, ' ');

    if(nodeTextLength > leftTree.width + rightTree.width + spacing.length()){
        // Then we will add some extra spacing between the trees when joining.
        spacing += QString(nodeTextLength - leftTree.width - rightTree.width - spacing.length(), ' ');
        mCurrentTree.width = leftTree.width + spacing.length() + rightTree.width; // == nodeTextLength.
        mCurrentTree.connector = mCurrentTree.width / 2;

    }else{
        // Then pad the node text.
        // Put it as close as possible to the point halfway beteen the left and right connectors.
        // However we will prefer to keep it within the correct width at all costs.
        int idealCentre = (leftTree.connector + leftTree.width + spacing.length() + rightTree.connector) / 2;

        mCurrentTree.width = leftTree.width + spacing.length() + rightTree.width;
        mCurrentTree.connector = idealCentre;

        QMutableListIterator<QString> it(nodeText);
        while(it.hasNext()){
            it.setValue(padToConnector(it.next(), idealCentre, mCurrentTree.width));
        }

    }

    // Make the trees the same height.
    if(leftTree.lines.length() > rightTree.lines.length()){
        int numToAdd = leftTree.lines.length() - rightTree.lines.length();
        for(int i = 0; i < numToAdd; i++){
            rightTree.lines.append(QString(rightTree.width, ' '));
        }

    }else{
        int numToAdd = rightTree.lines.length() - leftTree.lines.length();
        for(int i = 0; i < numToAdd; i++){
            leftTree.lines.append(QString(leftTree.width, ' '));
        }
    }

    // Join the trees.
    for(int i = 0; i < leftTree.lines.length(); i++){
        mCurrentTree.lines.append(leftTree.lines.at(i) + spacing + rightTree.lines.at(i));
    }

    // Add the conectors, connecting line and new node.
    int leftConnector = leftTree.connector;
    int rightConnector = leftTree.width + spacing.length() + rightTree.connector;

    QString connectorLines = QString(mCurrentTree.width, ' ');
    connectorLines.replace(leftConnector, 1, '|');
    connectorLines.replace(rightConnector, 1, '|');

    QString connectorBar = QString(leftConnector, ' ') + "+" + QString(rightConnector - leftConnector - 1, '-') + "+" + QString(mCurrentTree.width - rightConnector, ' ');
    connectorBar.replace(mCurrentTree.connector, 1, '+');

    QString connectorTopLine = QString(mCurrentTree.width, ' ');
    connectorTopLine.replace(mCurrentTree.connector, 1, '|');

    mCurrentTree.lines.prepend(connectorLines);
    mCurrentTree.lines.prepend(connectorBar);
    mCurrentTree.lines.prepend(connectorTopLine);

    QMutableListIterator<QString> it(nodeText);
    it.toBack();
    while(it.hasPrevious()){
        mCurrentTree.lines.prepend(it.previous());
    }

}



QString TerminalTracePrinter::padToConnector(QString text, int connector, int width)
{
    // Pad the text to be eactly width ccharacters and as close as possible
    // to being centred over the connector within that.

    int idealStart = connector - (text.length() / 2);
    int idealEnd = idealStart + text.length();

    if(idealStart <= 0){
        // Pad only on the right.
        text.append(QString(width - text.length(), ' '));

    }else if(idealEnd >= width){
        // Pad only on the left.
        text.prepend(QString(width - text.length(), ' '));

    }else{
        // Pad on both sides.
        text.prepend(QString(idealStart, ' '));
        text.append(QString(width - idealEnd, ' '));
    }

    return text;
}



// Makes sure all lines are the same length and adds some decoration around the node.
QList<QString> TerminalTracePrinter::processNodeTextLines(QList<QString> nodeText)
{
    if(nodeText.empty()){
        Log::fatal("Processing a node with empty description.");
        exit(1);
    }

    // Find the max line length.
    int maxLen = 0;
    foreach(QString line, nodeText){
        maxLen = max(maxLen, line.length());
    }

    // Modify the lines.
    QMutableListIterator<QString> it(nodeText);
    while(it.hasNext()){
        QString line = it.next();
        line = line.leftJustified(maxLen);
        line.prepend("[ ");
        line.append(" ]");
        it.setValue(line);
    }

    return nodeText;
}


} // namespace artemis

