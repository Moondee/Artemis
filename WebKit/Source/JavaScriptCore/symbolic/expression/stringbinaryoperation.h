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

 // AUTO GENERATED - DO NOT MODIFY

#ifndef SYMBOLIC_STRINGBINARYOPERATION_H
#define SYMBOLIC_STRINGBINARYOPERATION_H

#include <string>

#include <list>

#include "visitor.h"
#include "stringexpression.h"

#ifdef ARTEMIS

namespace Symbolic
{

typedef enum {
	CONCAT, STRING_EQ, STRING_NEQ, STRING_LT, STRING_LEQ, STRING_GT, STRING_GEQ, STRING_SEQ, STRING_SNEQ
} StringBinaryOp;

const char* opToString(StringBinaryOp op);
Type opGetType(StringBinaryOp op);


class StringBinaryOperation : public StringExpression
{
public:
    explicit StringBinaryOperation(StringExpression* lhs, StringBinaryOp op, StringExpression* rhs);
    void accept(Visitor* visitor);
    void accept(Visitor* visitor, void* arg);

	inline StringExpression* getLhs() {
		return m_lhs;
	}
	inline StringBinaryOp getOp() {
		return m_op;
	}
	inline StringExpression* getRhs() {
		return m_rhs;
	}

private:
	StringExpression* m_lhs;
	StringBinaryOp m_op;
	StringExpression* m_rhs;

};
}

#endif
#endif // SYMBOLIC_STRINGBINARYOPERATION_H