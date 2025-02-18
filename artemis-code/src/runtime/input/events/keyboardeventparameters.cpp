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

#include "keyboardeventparameters.h"
#include "artemisglobals.h"
#include "util/randomutil.h"

namespace artemis
{

KeyboardEventParameters::KeyboardEventParameters(QString eventType, bool canBubble, bool cancelable,
                                                QString keyIdentifier,  unsigned int keyLocation,
                                                bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, bool altGraphKey) :
    EventParameters()
{
    Q_ASSERT(!eventType.isEmpty());
    this->eventType = eventType;
    this->canBubble = canBubble;
    this->cancelable = cancelable;
    this->keyIdentifier = keyIdentifier;
    this->keyLocation = keyLocation;
    this->ctrlKey = ctrlKey;
    this->altKey = altKey;
    this->shiftKey = shiftKey;
    this->metaKey = metaKey;
    this->altGraphKey = altGraphKey;
}

QString KeyboardEventParameters::getJsString() const
{
    if (!cachedJsString.isEmpty()) {
        return cachedJsString;
    }

    QString randId = generateRandomJsId();
    QString res = "var " + randId + " = document.createEvent(\"KeyboardEvent\");";
    res = res + " " + randId + ".initKeyboardEvent(";
    res += eventType + ",";
    res += boolTostring(canBubble) + ",";
    res += boolTostring(cancelable) + ",";
    res += "null,";
    res += quoteString(keyIdentifier) + ",";
    res += keyLocation + ",";
    res += boolTostring(ctrlKey) + ",";
    res += boolTostring(altKey) + ",";
    res += boolTostring(shiftKey) + ",";
    res += boolTostring(metaKey) + ",";
    res += boolTostring(altGraphKey) + ");";

    res += "this.dispatchEvent(" + randId + ");";

    cachedJsString = res;
    return res;
}

EventType KeyboardEventParameters::getType() const
{
    return KEY_EVENT;
}

}
