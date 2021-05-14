// qsamplerUtilities.cpp
//
/****************************************************************************
   Copyright (C) 2004-2020, rncbc aka Rui Nuno Capela. All rights reserved.
   Copyright (C) 2007, 2008 Christian Schoenebeck

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

#include "qsamplerOptions.h"
#include "qsamplerMainForm.h"

#include <QRegularExpression>


using namespace QSampler;

namespace qsamplerUtilities {

static int _hexToNumber ( char hex_digit )
{
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

static int _hexsToNumber ( char hex0, char hex1 )
{
	return _hexToNumber(hex1) * 16 + _hexToNumber(hex0);
}

static bool _isHex ( char hex_digit )
{
	return _hexToNumber ( hex_digit ) || hex_digit == '0';
}

// returns true if the connected LSCP server supports escape sequences
static bool _remoteSupportsEscapeSequences (void)
{
	const lscpVersion_t version = getRemoteLscpVersion();
	// LSCP v1.2 or younger required
	return (version.major > 1 || (version.major == 1 && version.minor >= 2));
}


// converts the given file path into a path as expected by LSCP 1.2
QByteArray lscpEscapePath ( const QString& sPath )
{
	QByteArray path = sPath.toUtf8();
	if (!_remoteSupportsEscapeSequences()) return path;

        const char pathSeparator = '/';
	int path_len = path.length();
	char buf[5];

	// Trying single pass to avoid redundant checks on extra run
	for ( int i = 0; i < path_len; i++ ) {
		// translate POSIX escape sequences
		if (path[i] == '%') {
			// replace POSIX path escape sequences (%HH) by LSCP escape sequences (\xHH)
			// TODO: missing code for other systems like Windows
			if (_isHex(path[i+1]) && _isHex(path[i+2])) {
				path.replace (i, 1, "\\x");
				path_len++;
				i += 3;
				continue;
			}
			// replace POSIX path escape sequence (%%) by its raw character
			if (path[i+1] == '%') {
				path.remove (i, 1);
				path_len--;
				continue;
			}
			continue;
		}
		// replace all non-basic characters by LSCP escape sequences
		//
		// match all non-alphanumerics
		// (we could exclude much more characters here, but that way
		// we're sure it just works^TM)
		const char c = path[i];
		if (
			!(c >= '0' && c <= '9') &&
			!(c >= 'a' && c <= 'z') &&
			!(c >= 'A' && c <= 'Z') &&
		#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
			!(c == ':') &&
		#endif
			!(c == pathSeparator)
		) {
			// convertion
			::snprintf(buf, sizeof(buf), "\\x%02x", static_cast<unsigned char>(c));
			path = path.replace(i, 1, buf);
			path_len += 3;
			i += 3;
		}
	}

	return path;
}


// converts a path returned by a LSCP command (and may contain escape
// sequences) into the appropriate POSIX path
QString lscpEscapedPathToPosix ( const char* sPath )
{
	if (!_remoteSupportsEscapeSequences()) return QString(sPath);

	QByteArray path(sPath);
	int path_len = path.length();

	char cAscii[2] = "\0";
	for ( int i = 0; i < path_len; i++) {
		// first escape all percent ('%') characters for POSIX
		if (path[i] == '%') {
			path.insert(i, '%');
			path_len++;
			i++;
			continue;
		}
		// resolve LSCP hex escape sequences (\xHH)
		if (path[i] == '\\' && path[i+1] == 'x' && _isHex(path[i+2]) && _isHex(path[i+3])) {
			const QByteArray sHex = path.mid(i + 2, 2).toLower();
			// the slash has to be escaped for POSIX as well
			if (sHex == "2f") {
				path.replace(i, 4, "%2f");
			// all other characters we simply decode
			} else {
				cAscii[0] = _hexsToNumber(sHex[1], sHex[0]);
				path.replace(i, 4, cAscii);
			}
			path_len -= 3;
			continue;
		}
	}

	return QString(path);
}


// converts the given text as expected by LSCP 1.2
// (that is by encoding special characters with LSCP escape sequences)
QByteArray lscpEscapeText ( const QString& sText )
{
	QByteArray text = sText.toUtf8();
	if (!_remoteSupportsEscapeSequences()) return text;

	int text_len = text.length();
	char buf[5];

	// replace all non-basic characters by LSCP escape sequences
	for (int i = 0; i < text_len; ++i) {
		// match all non-alphanumerics
		// (we could exclude much more characters here, but that way
		// we're sure it just works^TM)
		const char c = text[i];
		if (
			!(c >= '0' && c <= '9') &&
			!(c >= 'a' && c <= 'z') &&
			!(c >= 'A' && c <= 'Z')
		) {
			// convert the non-basic character into a LSCP escape sequence
			::snprintf(buf, sizeof(buf), "\\x%02x", static_cast<unsigned char>(c));
			text.replace(i, 1, buf);
			text_len += 3;
			i += 3;
		}
	}

	return text;
}


// converts a text returned by a LSCP command and may contain escape
// sequences) into raw text, that is with all escape sequences decoded
QString lscpEscapedTextToRaw ( const char* sText )
{
	if (!_remoteSupportsEscapeSequences()) return QString(sText);

	QByteArray text(sText);
	int text_len = text.length();
	char sHex[2], cAscii[2] = "\0";

	// resolve LSCP hex escape sequences (\xHH)
	for (int i = 0; i < text_len; i++) {
		if (text[i] != '\\' || text[i+1] != 'x') continue;

		sHex[0] = text[i+2], sHex[1] = text[i+3];
		if (_isHex(sHex[0]) && _isHex(sHex[1])) {
			cAscii[0] = _hexsToNumber(sHex[1], sHex[0]);
			text.replace(i, 4, cAscii);
			text_len -= 3;
		}
	}

	return QString(text);
}

lscpVersion_t getRemoteLscpVersion (void)
{
    lscpVersion_t result = { 0, 0 };

    MainForm* pMainForm = MainForm::getInstance();
    if (pMainForm == nullptr)
        return result;
    if (pMainForm->client() == nullptr)
        return result;

    lscp_server_info_t* pServerInfo =
        ::lscp_get_server_info(pMainForm->client());
    if (pServerInfo && pServerInfo->protocol_version)
        ::sscanf(pServerInfo->protocol_version, "%d.%d",
            &result.major, &result.minor);

    return result;
}

} // namespace qsamplerUtilities


// end of qsamplerUtilities.cpp
