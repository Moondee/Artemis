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

#include "artemiswebview.h"

namespace artemis
{

ArtemisWebView::ArtemisWebView() :
    QWebView(NULL)
{

}

void ArtemisWebView::closeEvent(QCloseEvent* event)
{
    event->ignore();
    emit sigClose();
}

}
