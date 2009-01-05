/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        state_store.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Caching for AD Provider Database Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#ifndef __STATE_STORE_H__
#define __STATE_STORE_H__

DWORD
ADState_OpenDb(
    ADSTATE_CONNECTION_HANDLE* phDb
    );

// Sets the handle to null after closing it. If a null handle is passed in,
// it is ignored.
void
ADState_SafeCloseDb(
    ADSTATE_CONNECTION_HANDLE* phDb
    );

DWORD
ADState_GetProviderData(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    OUT PAD_PROVIDER_DATA* ppProvider
    );

DWORD
ADState_StoreProviderData(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    IN PAD_PROVIDER_DATA pProvider
    );

DWORD
ADState_GetDomainTrustList(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    OUT PDLINKEDLIST* ppList
    );

DWORD
ADState_StoreDomainTrustList(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    IN PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo,
    IN DWORD dwDomainInfoCount
    );

VOID
ADState_FreeEnumDomainInfoList(
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    IN OUT PDLINKEDLIST pList
    );

#endif /* __STATE_STORE_H__ */
