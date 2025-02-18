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

#ifndef SYMBOLIC_STRINGCHARAT_H
#define SYMBOLIC_STRINGCHARAT_H

#include <string>

#include <list>

#include "visitor.h"
#include "stringexpression.h"

#ifdef ARTEMIS

namespace Symbolic
{

class StringCharAt : public StringExpression
{
public:
    explicit StringCharAt(StringExpression* source, unsigned int position);
    void accept(Visitor* visitor);
    void accept(Visitor* visitor, void* arg);

	inline StringExpression* getSource() {
		return m_source;
	}
	inline unsigned int getPosition() {
		return m_position;
	}

private:
	StringExpression* m_source;
	unsigned int m_position;

};
}

#endif
#endif // SYMBOLIC_STRINGCHARAT_H