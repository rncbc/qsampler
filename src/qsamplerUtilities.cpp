// qsamplerUtilities.cpp
//
/****************************************************************************
   Copyright (C) 2004-2007, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qsamplerUtilities.h"

#include "qsamplerMainForm.h"

#include <stdio.h>
#include <qregexp.h>

// converts the given file path into a path as expected by LSCP 1.2
QString lscpEscapePath(QString path) {
    // check if remote side supports LSCP escape sequences
    const lscpVersion_t version = getRemoteLscpVersion();
    if (version.major < 1 || version.minor < 2)
        return path; // LSCP v1.2 or younger required

    // replace POSIX path escape sequences (%HH) by LSCP escape sequences (\xHH)
    // TODO: missing code for other systems like Windows
    {
        QRegExp regexp("%[0-9a-fA-F][0-9a-fA-F]");
        for (int i = path.find(regexp); i >= 0; i = path.find(regexp, i + 3))
            path.replace(i, 1, "\\x");
    }
    // replace all non-basic characters by LSCP escape sequences
    {
        const char pathSeparator = '/';
        QRegExp regexp(QRegExp::escape("\\x") + "[0-9a-fA-F][0-9a-fA-F]");
        for (int i = 0; i < int(path.length()); i++) {
            // first skip all previously added LSCP escape sequences
            if (path.find(regexp, i) == i) {
                i += 3;
                continue;
            }
            // now match all non-alphanumerics
            // (we could exclude much more characters here, but that way
            // we're sure it just works^TM)
            const char c = path.at(i).latin1();
            if (
                !(c >= '0' && c <= '9') &&
                !(c >= 'a' && c <= 'z') &&
                !(c >= 'A' && c <= 'Z') &&
                !(c == pathSeparator)
            ) {
                // convert the non-basic character into a LSCP escape sequence
                char buf[5];
                snprintf(buf, sizeof(buf), "\\x%2x", static_cast<unsigned char>(c));
                path.replace(i, 1, buf);
                i += 3;
            }
        }
    }
    return path;
}

lscpVersion_t getRemoteLscpVersion() {
    lscpVersion_t result = { 0, 0 };

    qsamplerMainForm *pMainForm = qsamplerMainForm::getInstance();
    if (!pMainForm || !pMainForm->client()) return result;

    lscp_server_info_t* pServerInfo =
        ::lscp_get_server_info(pMainForm->client());
    if (!pServerInfo) return result;

    sscanf(pServerInfo->protocol_version, "%d.%d", &result.major, &result.minor);
    return result;
}
