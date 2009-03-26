/* $Header$ */
/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2009
 *
 * Pierangelo Masarati	<masarati@aero.polimi.it>
 * Paolo Mantegazza	<mantegazza@aero.polimi.it>
 *
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 *
 * Changing this copyright notice is forbidden.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 * 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Michele Attolico <attolico@aero.polimi.it>
 */

/*
 * (Portions)
 *
 * AUTHOR: Dr. Rudolf Jaeger <rudijaeger@yahoo.com>
 * Copyright (C) 2008 all rights reserved.
 *
 * The copyright of this patch is trasferred
 * to Pierangelo Masarati and Paolo Mantegazza
 * for use in the software MBDyn as described 
 * in the GNU Public License version 2.1
 */

#ifdef HAVE_CONFIG_H
#include <mbconfig.h>           /* This goes first in every *.c,*.cc file */
#endif /* HAVE_CONFIG_H */

#ifdef USE_SOCKET

#include "dataman.h"
#include "filedrv.h"
#include "streamdrive.h"
#include "sock.h"
#include "socketstreamdrive.h"

#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/un.h>
#include <arpa/inet.h>

#define DEFAULT_PORT	9012	/* intentionally unassigned by IANA */
#define DEFAULT_HOST	"127.0.0.1"

SocketStreamDrive::SocketStreamDrive(unsigned int uL,
		const DriveHandler* pDH,
		UseSocket *pUS, bool c,
		const std::string& sFileName,
		integer nd, unsigned int ie,
		int flags,
		const struct timeval& st)
: StreamDrive(uL, pDH, sFileName, nd, c),
InputEvery(ie), InputCounter(0), pUS(pUS), recv_flags(flags),
SocketTimeout(st)
{
	ASSERT(InputEvery > 0);
}

SocketStreamDrive::~SocketStreamDrive(void)
{
	if (pUS != 0) {
		SAFEDELETE(pUS);
	}
}

FileDrive::Type
SocketStreamDrive::GetFileDriveType(void) const
{
	return FileDrive::SOCKETSTREAM;
}

/* Scrive il contributo del DriveCaller al file di restart */   
std::ostream&
SocketStreamDrive::Restart(std::ostream& out) const
{
	out << "  file: " << uLabel << ", socket stream," 
		" stream drive name, \"" << sFileName << "\"";
	pUS->Restart(out);
	return out << ", " << iNumDrives << ";" << std::endl;
}
   
void
SocketStreamDrive::ServePending(const doublereal& t)
{
	
	// by now, an abandoned drive is not read any more;
	// should we retry or what?
	if (pUS->Abandoned()) {
		silent_cout("SocketStreamDrive(" << sFileName << "): "
			"abandoned"  << std::endl);
		return;
	}

	ASSERT(pUS->Connected());
	
	/* read only every InputEvery steps */
	InputCounter++;
	if (InputCounter != InputEvery) {
		return;
	}
	InputCounter = 0;
	
	int sock_nr = pUS->GetSock();
	int rc = -1;
	// Use socket timeout if set in input file; Default: 0
	if (SocketTimeout.tv_sec || SocketTimeout.tv_usec) {
		// Use Select() on the socket for automatic shutdown if
		// socket clients fail.
		fd_set readfds;

		// Clear the set
		FD_ZERO(&readfds);

		// Add descriptors to the set
		FD_SET(sock_nr, &readfds);

		// Copy timeout because select(2) may overwrite it
		struct timeval tv = SocketTimeout;

		// Call select
		std::cerr << sock_nr+1 << " " << &readfds << " " << &tv << std::endl;
		rc = select(sock_nr+1, &readfds, NULL, NULL, &tv);
		switch (rc) {
		case -1: {
			int save_errno = errno;
			char *err_msg = strerror(save_errno);

			silent_cout("SocketStreamDrive"
				"(" << sFileName << "): select failed"
				<< " (" << save_errno << ": " 
				<< err_msg << ")" << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
			}

		case 0:
			silent_cout("SocketStreamDrive"
				"(" << sFileName << "): select timed out"
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);

		default:
			if (!FD_ISSET(sock_nr, &readfds)) {
				silent_cout("SocketStreamDrive"
					"(" << sFileName << "): "
					"socket " << sock_nr << " reset"
					<< std::endl);
				throw ErrGeneric(MBDYN_EXCEPT_ARGS);
			}
		}
	}

	// Read data
	// NOTE: flags __SHOULD__ contain MSG_WAITALL;
	// however, it is not defined on some platforms (e.g. Cygwin)
	rc = recv(sock_nr, buf, size, recv_flags);

	/* FIXME: no receive at first step? */
	switch (rc) {
	case 0:
do_abandon:;
		silent_cout("SocketStreamDrive(" << sFileName << "): "
			<< "communication closed by host; abandoning..."
			<< std::endl);
		pUS->Abandon();
		break;

	case -1: {
		int save_errno = errno;

		/* some errno values may be legal */
		switch (save_errno) {
		case ECONNRESET:
			goto do_abandon;
		}

		char *err_msg = strerror(save_errno);

		silent_cout("SocketStreamDrive(" << sFileName << ") failed "
				"(" << save_errno << ": " << err_msg << ")"
				<< std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}

	default: {	
		doublereal *rbuf = (doublereal *)buf - 1;
		
		for (int i = 1; i <= iNumDrives; i++) {
			pdVal[i] = rbuf[i];
		}

		} break;
	}
}


/* legge i drivers tipo stream */

Drive*
ReadSocketStreamDrive(DataManager* pDM,
	MBDynParser& HP,
	unsigned int uLabel)
{
	bool create = false;
	unsigned short int port = -1; 
	std::string name;
	std::string host;
	std::string path;

	if (HP.IsKeyWord("name") || HP.IsKeyWord("stream" "drive" "name")) {
		const char *m = HP.GetStringWithDelims();
		if (m == 0) {
			silent_cerr("SocketStreamDrive(" << uLabel << "): "
				"unable to read stream drive name "
				"at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);

		} 

		name = m;

	} else {
		silent_cerr("SocketStreamDrive(" << uLabel << "):"
			"missing stream drive name "
			"at line " << HP.GetLineData()
			<< std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}

	if (HP.IsKeyWord("create")) {
		if (HP.IsKeyWord("yes")) {
			create = true;
		} else if (HP.IsKeyWord("no")) {
			create = false;
		} else {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"\"create\" must be either \"yes\" or \"no\" "
				"at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
	}
		
	if (HP.IsKeyWord("local") || HP.IsKeyWord("path")) {
		const char *m = HP.GetFileName();
		
		if (m == 0) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"unable to read local path"
				"at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		
		path = m;
	}
	
	if (HP.IsKeyWord("port")) {
		if (!path.empty()) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"cannot specify port "
				"for a local socket "
				"at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);		
		}

		int p = HP.GetInt();
		/* Da sistemare da qui */
#ifdef IPPORT_USERRESERVED
		if (p <= IPPORT_USERRESERVED) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"cannot listen on reserved port "
				<< port << ": less than "
				"IPPORT_USERRESERVED=" << IPPORT_USERRESERVED
				<< " at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		/* if #undef'd, don't bother checking;
		 * the OS will do it for us */
#endif /* IPPORT_USERRESERVED */

		port = p;
	}

	
	if (HP.IsKeyWord("host")) {
		if (!path.empty()) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"cannot specify host for a local socket "
				"at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);		
		}

		const char *h;
		
		h = HP.GetStringWithDelims();
		if (h == 0) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"unable to read host "
				"at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}

		host = h;

	} else if (path.empty() && !create) {
		silent_cerr("SocketStreamDrive"
			"(" << uLabel << ", \"" << name << "\"): "
			"host undefined, "
			"using default \"" << DEFAULT_HOST "\" "
			"at line " << HP.GetLineData()
			<< std::endl);
		host = DEFAULT_HOST;
	}

	// we want to block until the whole chunk is received
	int flags = 0;
#ifdef MSG_WAITALL
	flags |= MSG_WAITALL;
#endif // MSG_WAITALL

	while (HP.IsArg()) {
		if (HP.IsKeyWord("signal")) {
#ifdef MSG_NOSIGNAL
			flags &= ~MSG_NOSIGNAL;
#else // ! MSG_NOSIGNAL
			silent_cout("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"MSG_NOSIGNAL not defined (ignored) "
				"at line " << HP.GetLineData()
				<< std::endl);
#endif // ! MSG_NOSIGNAL

		// not honored by recv(2)
		} else if (HP.IsKeyWord("no" "signal")) {
#ifdef MSG_NOSIGNAL
			flags |= MSG_NOSIGNAL;
#else // ! MSG_NOSIGNAL
			silent_cout("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"MSG_NOSIGNAL not defined (ignored) "
				"at line " << HP.GetLineData()
				<< std::endl);
#endif // ! MSG_NOSIGNAL

		} else if (HP.IsKeyWord("blocking")) {
			// not honored by recv(2)?
			flags |= MSG_WAITALL;
			flags &= ~MSG_DONTWAIT;

		} else if (HP.IsKeyWord("non" "blocking")) {
			// not honored by recv(2)?
			flags &= ~MSG_WAITALL;
			flags |= MSG_DONTWAIT;

		} else {
			break;
		}
	}

	unsigned int InputEvery = 1;
	if (HP.IsKeyWord("input" "every")) {
		int i = HP.GetInt();
		if (i <= 0) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"invalid \"input every\" value " << i
				<< " at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		InputEvery = (unsigned int)i;
	}

	struct timeval SocketTimeout = { 0, 0 };
	if (HP.IsKeyWord("timeout")) {
		doublereal st = HP.GetReal();
		if (st < 0) {
			silent_cerr("SocketStreamDrive"
				"(" << uLabel << ", \"" << name << "\"): "
				"invalid socket timeout value " << st
				<< " at line " << HP.GetLineData()
				<< std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		SocketTimeout.tv_sec = long(st);
		SocketTimeout.tv_usec = long((st - SocketTimeout.tv_sec)*1000000);
	}

	pedantic_cout("SocketStreamDrive"
		"(" << uLabel << ", \"" << name << "\"): "
		"timeout: " << SocketTimeout.tv_sec << "s "
		<< SocketTimeout.tv_usec << "ns" << std::endl);

	int idrives = HP.GetInt();
	if (idrives <= 0) {
		silent_cerr("SocketStreamDrive"
			"(" << uLabel << ", \"" << name << "\"): "
			"illegal number of channels " << idrives
			<< "at line " << HP.GetLineData()
			<< std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}
	
	UseSocket *pUS = 0;
	if (path.empty()) {
		if (port == (unsigned short int)(-1)) {
			port = DEFAULT_PORT;
		}
		SAFENEWWITHCONSTRUCTOR(pUS, UseInetSocket, UseInetSocket(host, port, create));

	} else {
		SAFENEWWITHCONSTRUCTOR(pUS, UseLocalSocket, UseLocalSocket(path, create));
	}

	if (create) {
		pDM->RegisterSocketUser(pUS);

	} else {
		pUS->Connect();
	}

	Drive* pDr = 0;
	SAFENEWWITHCONSTRUCTOR(pDr, SocketStreamDrive,
		SocketStreamDrive(uLabel,
			pDM->pGetDrvHdl(), pUS, create,
			name, idrives, InputEvery,
			flags, SocketTimeout));

	return pDr;
} /* End of ReadStreamDrive */

#endif // USE_SOCKET
