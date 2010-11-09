/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        create.c
 *
 * Abstract:
 *
 *        SMB Client Redirector
 *
 *        Create Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kaya Bekiroglu (kaya@likewisesoftware.com)
 *          Brian Koropoff (bkoropoff@likewise.com)
 */

#include "rdr.h"

static
BOOLEAN
RdrFinishCreate(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrTransceiveCreate(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions
    );

static
BOOLEAN
RdrChaseCreateReferralComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
void
RdrCancelCreate(
    PIRP pIrp,
    PVOID _pContext
    )
{
    return;
}

static
NTSTATUS
RdrConstructCanonicalPath(
    PWSTR pwszShare,
    PWSTR pwszFilename,
    PWSTR* ppwszCanonical
    )
{
    if (pwszFilename[0] == '\\' &&
        pwszFilename[1] == '\0')
    {
        return LwRtlWC16StringDuplicate(ppwszCanonical, pwszShare);
    }
    else
    {
        return LwRtlWC16StringAllocatePrintfW(
            ppwszCanonical,
            L"%ws%ws",
            pwszShare,
            pwszFilename);
    }
}

static
BOOLEAN
RdrShareIsIpc(
    PWSTR pwszShare
    )
{
    static const WCHAR wszIpcDollar[] = {'I','P','C','$','\0'};
    ULONG ulLen = LwRtlWC16StringNumChars(pwszShare);

    return (ulLen >= 4 && LwRtlWC16StringIsEqual(pwszShare + ulLen - 4, wszIpcDollar, FALSE));
}

NTSTATUS
RdrCreateTreeConnect(
    PRDR_OP_CONTEXT pContext,
    PWSTR pwszFilename
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIRP pIrp = pContext->pIrp;
    PIO_CREDS pCreds = IoSecurityGetCredentials(pIrp->Args.Create.SecurityContext);
    PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcessInfo =
        IoSecurityGetProcessInfo(pIrp->Args.Create.SecurityContext);
    PRDR_TREE pTree = NULL;
    PWSTR pwszServer = NULL;
    PWSTR pwszShare = NULL;
    PWSTR pwszResolved = NULL;
    BOOLEAN bChaseReferrals = FALSE;
    BOOLEAN bIsRoot = FALSE;

    do
    {
        if (!pContext->State.Create.OrigStatus)
        {
            pContext->State.Create.OrigStatus = status;
        }

        status = RdrConvertPath(
            pwszFilename,
            &pwszServer,
            &pwszShare,
            &pContext->State.Create.pwszFilename);
        BAIL_ON_NT_STATUS(status);

        status = RdrConstructCanonicalPath(
            pwszShare,
            pContext->State.Create.pwszFilename,
            &pContext->State.Create.pwszCanonicalPath);
        BAIL_ON_NT_STATUS(status);

        if (!RdrShareIsIpc(pwszShare))
        {
            /* Resolve against DFS cache */
            status = RdrDfsResolvePath(pContext->State.Create.pwszCanonicalPath, pContext->usTry, &pwszResolved, &bIsRoot);
            switch (status)
            {
            case STATUS_DFS_UNAVAILABLE:
                status = pContext->State.Create.OrigStatus;
                BAIL_ON_NT_STATUS(status);
            case STATUS_NOT_FOUND:
                /* Proceed with current path and chase referrals */
                bChaseReferrals = TRUE;
                status = STATUS_SUCCESS;
                break;
            default:
                BAIL_ON_NT_STATUS(status);

                /*
                 * Since we got a hit in our referral cache,
                 * we don't need to chase referrals when tree connecting
                 */
                bChaseReferrals = FALSE;

                RTL_FREE(&pwszServer);
                RTL_FREE(&pwszShare);
                RTL_FREE(&pContext->State.Create.pwszFilename);

                status = RdrConvertPath(
                    pwszResolved,
                    &pwszServer,
                    &pwszShare,
                    &pContext->State.Create.pwszFilename);
                BAIL_ON_NT_STATUS(status);
            }
        }
        else
        {
            bChaseReferrals = FALSE;
        }

        status = RdrTreeConnect(
            pwszServer,
            pwszShare,
            pCreds,
            pProcessInfo->Uid,
            bChaseReferrals,
            pContext);
        pContext->usTry++;
    } while (RdrDfsStatusIsRetriable(status));
    BAIL_ON_NT_STATUS(status);

cleanup:

    RTL_FREE(&pwszServer);
    RTL_FREE(&pwszShare);
    RTL_FREE(&pwszResolved);

    return status;

error:

    if (status != STATUS_PENDING && pTree)
    {
        RdrTreeRelease(pTree);
        pTree = NULL;
    }

    goto cleanup;
}

static
BOOLEAN
RdrCreateTreeConnected(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_TREE pTree = NULL;
    PIRP pIrp = pContext->pIrp;
    ACCESS_MASK DesiredAccess = pIrp->Args.Create.DesiredAccess;
    LONG64 AllocationSize = pIrp->Args.Create.AllocationSize;
    FILE_SHARE_FLAGS ShareAccess = pIrp->Args.Create.ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition = pIrp->Args.Create.CreateDisposition;
    FILE_CREATE_OPTIONS CreateOptions = pIrp->Args.Create.CreateOptions;
    FILE_ATTRIBUTES FileAttributes =  pIrp->Args.Create.FileAttributes;
    PRDR_CCB pFile = NULL;

    if (pParam)
    {
        switch (RDR_OBJECT_PROTOCOL(pParam))
        {
        case SMB_PROTOCOL_VERSION_1:
            pTree = (PRDR_TREE) pParam;
            break;
        case SMB_PROTOCOL_VERSION_2:
            /* We ended up with an SMB2 tree, short circuit to create2.c */
            return RdrCreateTreeConnected2(pContext, status, pParam);
        default:
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(status);
        }
    }

    if (RdrDfsStatusIsRetriable(status))
    {
        if (!pContext->State.Create.OrigStatus)
        {
            pContext->State.Create.OrigStatus = status;
        }
        pContext->Continue = RdrCreateTreeConnected;
        status = RdrCreateTreeConnect(pContext, pIrp->Args.Create.FileName.FileName);
    }
    BAIL_ON_NT_STATUS(status);

    status = LwIoAllocateMemory(
        sizeof(RDR_CCB),
        (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(status);

    status = LwErrnoToNtStatus(pthread_mutex_init(&pFile->mutex, NULL));
    BAIL_ON_NT_STATUS(status);

    pFile->bMutexInitialized = TRUE;
    pFile->version = SMB_PROTOCOL_VERSION_1;
    pFile->pTree = pTree;
    pTree = NULL;
    pFile->Params.CreateOptions = CreateOptions;

    pFile->pwszPath = pContext->State.Create.pwszFilename;
    pContext->State.Create.pwszFilename = NULL;

    pFile->pwszCanonicalPath = pContext->State.Create.pwszCanonicalPath;
    pContext->State.Create.pwszCanonicalPath = NULL;

    pContext->Continue = RdrFinishCreate;

    pContext->State.Create.pFile = pFile;

    if (DesiredAccess == DELETE)
    {
        /* If the desired access is precisely DELETE, we can only peform
           move or delete operations.  Since these are path-based, there is
           no point in opening a fid */
        status = IoFileSetContext(pContext->pIrp->FileHandle, pFile);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        status = RdrTransceiveCreate(
            pContext,
            pFile,
            DesiredAccess,
            AllocationSize,
            FileAttributes,
            ShareAccess,
            CreateDisposition,
            CreateOptions);
        switch (status)
        {
        case STATUS_SUCCESS:
            break;
        default:
            if (RdrDfsStatusIsRetriable(status))
            {
                if (!pContext->State.Create.OrigStatus)
                {
                    pContext->State.Create.OrigStatus = status;
                }

                RdrReleaseFile(pFile);
                pContext->State.Create.pFile = pFile = NULL;

                pContext->Continue = RdrCreateTreeConnected;
                status = RdrCreateTreeConnect(pContext, pIrp->Args.Create.FileName.FileName);
            }
            BAIL_ON_NT_STATUS(status);
        }
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    if (status != STATUS_PENDING)
    {
        RTL_FREE(&pContext->State.Create.pwszFilename);
        RTL_FREE(&pContext->State.Create.pwszCanonicalPath);
        RdrFreeContext(pContext);
        pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pIrp);
    }

    return FALSE;

error:

    if (status != STATUS_PENDING && pFile)
    {
        RdrReleaseFile(pFile);
    }

    if (status != STATUS_PENDING && pTree)
    {
        RdrTreeRelease(pTree);
    }


    goto cleanup;
}

static
BOOLEAN
RdrFinishCreate(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PRDR_CCB pFile = pContext->State.Create.pFile;
    PSMB_PACKET pPacket = pParam;
    PCREATE_RESPONSE_HEADER pResponseHeader = NULL;

    if (status == STATUS_SUCCESS)
    {
        status = pPacket->pSMBHeader->error;
    }

    switch (status)
    {
    case STATUS_SUCCESS:
        break;
    case STATUS_PATH_NOT_COVERED:
        RdrReleaseFile(pFile);
        pContext->State.Create.pFile = pFile = NULL;
        pContext->usTry = 0;
        pContext->Continue = RdrChaseCreateReferralComplete;
        status = RdrDfsChaseReferral1(
            pFile->pTree->pSession,
            IoSecurityGetCredentials(pContext->pIrp->Args.Create.SecurityContext),
            pFile->pwszCanonicalPath,
            pContext);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        if (RdrDfsStatusIsRetriable(status))
        {
            if (!pContext->State.Create.OrigStatus)
            {
                pContext->State.Create.OrigStatus = status;
            }

            RdrReleaseFile(pFile);
            pContext->State.Create.pFile = pFile = NULL;

            pContext->Continue = RdrCreateTreeConnected;
            status = RdrCreateTreeConnect(pContext, pContext->pIrp->Args.Create.FileName.FileName);
        }
        BAIL_ON_NT_STATUS(status);
    }

    status = WireUnmarshallSMBResponseCreate(
                pPacket->pParams,
                pPacket->bufferLen - pPacket->bufferUsed,
                &pResponseHeader);
    BAIL_ON_NT_STATUS(status);

    pFile->fid = pResponseHeader->fid;
    pFile->usFileType = pResponseHeader->fileType;

    status = IoFileSetContext(pContext->pIrp->FileHandle, pFile);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RdrFreePacket(pPacket);

    if (status != STATUS_PENDING)
    {
        pContext->pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pContext->pIrp);
        RTL_FREE(&pContext->State.Create.pwszFilename);
        RTL_FREE(&pContext->State.Create.pwszCanonicalPath);
        RdrFreeContext(pContext);
    }

    return FALSE;

error:

    RdrReleaseFile(pFile);

    goto cleanup;
}

static
BOOLEAN
RdrChaseCreateReferralComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    BAIL_ON_NT_STATUS(status);

    pContext->Continue = RdrCreateTreeConnected;

    status = RdrCreateTreeConnect(pContext, pContext->pIrp->Args.Create.FileName.FileName);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (status != STATUS_PENDING)
    {
        pContext->pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pContext->pIrp);
        RTL_FREE(&pContext->State.Create.pwszFilename);
        RTL_FREE(&pContext->State.Create.pwszCanonicalPath);
        RdrFreeContext(pContext);
    }

    return FALSE;

error:

    goto cleanup;
}

NTSTATUS
RdrCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;
    PIO_CREDS pCreds = IoSecurityGetCredentials(pIrp->Args.Create.SecurityContext);

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelCreate, pContext);

    if (!pCreds)
    {
        status = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(status);
    }

    pContext->Continue = RdrCreateTreeConnected;

    status = RdrCreateTreeConnect(pContext, pIrp->Args.Create.FileName.FileName);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (status != STATUS_PENDING && pContext)
    {
        pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pIrp);
        RdrFreeContext(pContext);
        status = STATUS_PENDING;
    }

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveCreate(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32_t packetByteCount = 0;
    CREATE_REQUEST_HEADER *pHeader = NULL;
    PWSTR pwszPath = RDR_CCB_PATH(pFile);

    status = RdrAllocateContextPacket(
        pContext,
        1024*64);
    BAIL_ON_NT_STATUS(status);

    status = SMBPacketMarshallHeader(
                pContext->Packet.pRawBuffer,
                pContext->Packet.bufferLen,
                COM_NT_CREATE_ANDX,
                0,
                0,
                pFile->pTree->tid,
                gRdrRuntime.SysPid,
                pFile->pTree->pSession->uid,
                0,
                TRUE,
                &pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    if (RDR_CCB_IS_DFS(pFile))
    {
        pContext->Packet.pSMBHeader->flags2 |= FLAG2_DFS;
    }

    pContext->Packet.pData = pContext->Packet.pParams + sizeof(CREATE_REQUEST_HEADER);

    pContext->Packet.bufferUsed += sizeof(CREATE_REQUEST_HEADER);

    pContext->Packet.pSMBHeader->wordCount = 24;

    pHeader = (CREATE_REQUEST_HEADER *) pContext->Packet.pParams;

    pHeader->reserved = 0;
    pHeader->nameLength = (wc16slen(pwszPath) + 1) * sizeof(wchar16_t);
    pHeader->flags = 0;
    pHeader->rootDirectoryFid = 0;
    pHeader->desiredAccess = desiredAccess;
    pHeader->allocationSize = llAllocationSize;
    pHeader->extFileAttributes = fileAttributes;
    pHeader->shareAccess = shareAccess;
    pHeader->createDisposition = createDisposition;
    pHeader->createOptions = createOptions;
    pHeader->impersonationLevel = 0x2; /* FIXME: magic constant */

    status = WireMarshallCreateRequestData(
                pContext->Packet.pData,
                pContext->Packet.bufferLen - pContext->Packet.bufferUsed,
                (pContext->Packet.pData - (uint8_t *) pContext->Packet.pSMBHeader) % 2,
                &packetByteCount,
                pwszPath);
    BAIL_ON_NT_STATUS(status);

    assert(packetByteCount <= UINT16_MAX);
    pHeader->byteCount = (uint16_t) packetByteCount;
    pContext->Packet.bufferUsed += packetByteCount;

    // byte order conversions
    SMB_HTOL8_INPLACE(pHeader->reserved);
    SMB_HTOL16_INPLACE(pHeader->nameLength);
    SMB_HTOL32_INPLACE(pHeader->flags);
    SMB_HTOL32_INPLACE(pHeader->rootDirectoryFid);
    SMB_HTOL32_INPLACE(pHeader->desiredAccess);
    SMB_HTOL64_INPLACE(pHeader->allocationSize);
    SMB_HTOL32_INPLACE(pHeader->extFileAttributes);
    SMB_HTOL32_INPLACE(pHeader->shareAccess);
    SMB_HTOL32_INPLACE(pHeader->createDisposition);
    SMB_HTOL32_INPLACE(pHeader->createOptions);
    SMB_HTOL32_INPLACE(pHeader->impersonationLevel);
    SMB_HTOL8_INPLACE(pHeader->securityFlags);
    SMB_HTOL16_INPLACE(pHeader->byteCount);

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pFile->pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}
