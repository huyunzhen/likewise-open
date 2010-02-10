/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Likewise Server Service (LWSRVSVC)
 *
 * Server Main
 *
 * Function Prototypes
 *
 */

// config.c

DWORD
SrvSvcReadConfigSettings(
    VOID
    );

DWORD
SrvSvcConfigGetLsaLpcSocketPath(
    PSTR* ppszPath
    );

// main.c

VOID
SrvSvcSetProcessShouldExit(
    BOOLEAN val
    );

// signalhandler.c

VOID
SrvSvcBlockSelectedSignals(
    VOID
    );

DWORD
SrvSvcHandleSignals(
    VOID
    );

// utils.c

DWORD
SrvSvcSrvAllocateWC16StringFromUnicodeStringEx(
    OUT PWSTR            *ppwszOut,
    IN  UnicodeStringEx  *pIn
    );

DWORD
SrvSvcSrvAllocateWC16String(
    OUT PWSTR  *ppwszOut,
    IN  PCWSTR  pwszIn
    );

DWORD
SrvSvcSrvAllocateWC16StringFromCString(
    OUT PWSTR  *ppwszOut,
    IN  PCSTR   pszIn
    );
