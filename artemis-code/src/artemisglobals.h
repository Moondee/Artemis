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

#ifndef ARTEMISGLOBALS_H
#define ARTEMISGLOBALS_H

#include <cstdlib>
#include <iostream>
#include <QWebElement>

using namespace std;

namespace artemis
{

inline QString quoteString(const QString s)
{
    return "\"" + s + "\"";
}

inline QString boolTostring(const bool b)
{
    return (b ? "true" : "false");
}

inline QString intTostring(const int i)
{
    QString res = "";
    res.setNum(i);
    return res;
}

inline QUrl examplesIndexUrl()
{
    // The defualt URL will be artemis-code/tests/system/fixtures/concolic-examples-index.html
    char* artemisdir = std::getenv("ARTEMISDIR");
    if(!artemisdir){
        std::cerr << "Could not read ARTEMISDIR environment variable." << std::endl;
        exit(1);
    }

    return QUrl(QString("file://%1/artemis-code/tests/system/fixtures/concolic-examples-index.html").arg(artemisdir));
}

}

#endif // ARTEMISGLOBALS_H
