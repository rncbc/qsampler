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

using namespace QSampler;

namespace qsamplerUtilities {

static int _hexToNumber(char hex_digit) {
    switch (hex_digit) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;

        case 'a': return 10;
        case 'b': return 11;
        case 'c': return 12;
        case 'd': return 13;
        case 'e': return 14;
        case 'f': return 15;

        case 'A': return 10;
        case 'B': return 11;
        case 'C': return 12;
        case 'D': return 13;
        case 'E': return 14;
        case 'F': return 15;

        default:  return 0;
    }
}

static int _hexsToNumber(char hex_digit0, char hex_digit1) {
    return _hexToNumber(hex_digit1)*16 + _hexToNumber(hex_digit0);
}

// returns true if the connected LSCP server supports escape sequences
static bool _remoteSupportsEscapeSequences() {
    const lscpVersion_t version = getRemoteLscpVersion();
    // LSCP v1.2 or younger required
    return (version.major > 1 || (version.major == 1 && version.minor >= 2));
}

// converts the given file path into a path as expected by LSCP 1.2
QString lscpEscapePath ( const QString& sPath )
{
    if (!_remoteSupportsEscapeSequences()) return sPath;

    QString path(sPath);

    // replace POSIX path escape sequences (%HH) by LSCP escape sequences (\xHH)
    // TODO: missing code for other systems like Windows
    {
        QRegExp regexp("%[0-9a-fA-F][0-9a-fA-F]");
        for (int i = path.find(regexp); i >= 0; i = path.find(regexp, i + 4))
            path.replace(i, 1, "\\x");
    }
    // replace POSIX path escape sequence (%%) by its raw character
    for (int i = path.find("%%"); i >= 0; i = path.find("%%", ++i))
        path.remove(i, 1);

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
                ::snprintf(buf, sizeof(buf), "\\x%02x", static_cast<unsigned char>(c));
                path.replace(i, 1, buf);
                i += 3;
            }
        }
    }

    return path;
}

// converts a path returned by a LSCP command (and may contain escape
// sequences) into the appropriate POSIX path
QString lscpEscapedPathToPosix(QString path) {
    if (!_remoteSupportsEscapeSequences()) return path;

    // first escape all percent ('%') characters for POSIX
    for (int i = path.find('%'); i >= 0; i = path.find('%', i+2))
        path.replace(i, 1, "%%");

    // resolve LSCP hex escape sequences (\xHH)
    QRegExp regexp(QRegExp::escape("\\x") + "[0-9a-fA-F][0-9a-fA-F]");
    for (int i = path.find(regexp); i >= 0; i = path.find(regexp, i + 4)) {
        const QString sHex = path.mid(i+2, 2).lower();
        // the slash has to be escaped for POSIX as well
        if (sHex == "2f") {
            path.replace(i, 4, "%2f");
            continue;
        }
        // all other characters we simply decode
        char cAscii = _hexsToNumber(sHex.at(1).latin1(), sHex.at(0).latin1());
        path.replace(i, 4, cAscii);
    }

    return path;
}

// converts a text returned by a LSCP command and may contain escape
// sequences) into raw text, that is with all escape sequences decoded
QString lscpEscapedTextToRaw(QString txt) {
    if (!_remoteSupportsEscapeSequences()) return txt;

    // resolve LSCP hex escape sequences (\xHH)
    QRegExp regexp(QRegExp::escape("\\x") + "[0-9a-fA-F][0-9a-fA-F]");
    for (int i = txt.find(regexp); i >= 0; i = txt.find(regexp, i + 4)) {
        const QString sHex = txt.mid(i+2, 2).lower();
        // decode into raw ASCII character
        char cAscii = _hexsToNumber(sHex.at(1).latin1(), sHex.at(0).latin1());
        txt.replace(i, 4, cAscii);
    }

    return txt;
}

lscpVersion_t getRemoteLscpVersion (void)
{
    lscpVersion_t result = { 0, 0 };

    MainForm* pMainForm = MainForm::getInstance();
    if (pMainForm == NULL)
        return result;
    if (pMainForm->client() == NULL)
        return result;

    lscp_server_info_t* pServerInfo =
        ::lscp_get_server_info(pMainForm->client());
    if (pServerInfo && pServerInfo->protocol_version)
        ::sscanf(pServerInfo->protocol_version, "%d.%d",
            &result.major, &result.minor);

    return result;
}

} // namespace qsamplerUtilities
