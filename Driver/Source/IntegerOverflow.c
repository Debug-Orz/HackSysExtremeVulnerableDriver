/*++

          ##     ## ######## ##     ## ########  
          ##     ## ##       ##     ## ##     ## 
          ##     ## ##       ##     ## ##     ## 
          ######### ######   ##     ## ##     ## 
          ##     ## ##        ##   ##  ##     ## 
          ##     ## ##         ## ##   ##     ## 
          ##     ## ########    ###    ########  

            HackSys Extreme Vulnerable Driver

Author : Ashfaq Ansari
Contact: ashfaq[at]payatu[dot]com
Website: http://www.payatu.com/

Copyright (C) 2011-2015 Payatu Technologies. All rights reserved.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

See the file 'LICENSE' for complete copying permission.

Module Name:
    IntegerOverflow.c

Abstract:
    This module implements the functions to demonstrate
    Integer Overflow (Arithmetic Overflow) vulnerability.

--*/

#include "IntegerOverflow.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, TriggerIntegerOverflow)
    #pragma alloc_text(PAGE, IntegerOverflowIoctlHandler)
#endif // ALLOC_PRAGMA

#pragma auto_inline(off)

/// <summary>
/// Trigger the Integer Overflow Vulnerability
/// </summary>
/// <param name="pUserModeBuffer">The pointer to user mode buffer</param>
/// <param name="userModeBufferSize">Size of the user mode buffer</param>
/// <returns>NTSTATUS</returns>
NTSTATUS TriggerIntegerOverflow(IN PVOID pUserModeBuffer, IN SIZE_T userModeBufferSize) {
    ULONG arrayCount = 0;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG bufferTerminator = 0xBAD0B0B0;
    ULONG kernelBuffer[BUFFER_SIZE] = {0};
    SIZE_T bufferTerminatorSize = sizeof(bufferTerminator);

    PAGED_CODE();

    __try {
        DbgPrint("[+] pUserModeBuffer: 0x%p\n", pUserModeBuffer);
        DbgPrint("[+] userModeBufferSize: 0x%X\n", userModeBufferSize);

        // Verify if the buffer resides in User Mode
        ProbeForRead(pUserModeBuffer, sizeof(kernelBuffer), (ULONG)__alignof(kernelBuffer));

        DbgPrint("[+] kernelBuffer: 0x%p\n", &kernelBuffer);
        DbgPrint("[+] kernelBuffer Size: 0x%X\n", sizeof(kernelBuffer));

        #ifdef SECURE
            // Secure Note: This is secure because the developer is not doing any arithmetic 
            // on the user supplied value. Instead, the developer is subtracting the size of 
            // ULONG i.e. 4 on x86 from kernel buffer size. Hence, integer overflow will not 
            // occur and this check will not fail
            if (userModeBufferSize > (sizeof(kernelBuffer) - bufferTerminatorSize)) {
                DbgPrint("[-] Invalid Buffer Size: 0x%X\n", userModeBufferSize);

                status = STATUS_INVALID_BUFFER_SIZE;
                return status;
            }
        #else
            DbgPrint("[+] Triggering Integer Overflow\n");

            // Vulnerability Note: This is a vanilla Integer Overflow vulnerability because if 
            // 'userModeBufferSize' is 0xFFFFFFFF and we do an addition with size of ULONG i.e. 
            // 4 on x86, the integer will wrap down and this will finally cause this check to fail
            if ((userModeBufferSize + bufferTerminatorSize) > sizeof(kernelBuffer)) {
                DbgPrint("[-] Invalid Buffer Size: 0x%X\n", userModeBufferSize);

                status = STATUS_INVALID_BUFFER_SIZE;
                return status;
            }
        #endif

        // Perform the copy operation
        while (arrayCount < (userModeBufferSize / sizeof(ULONG))) {
            if (*(PULONG)pUserModeBuffer != bufferTerminator) {
                kernelBuffer[arrayCount] = *(PULONG)pUserModeBuffer;
                pUserModeBuffer = (PULONG)pUserModeBuffer + 1;
                arrayCount++;
            }
            else {
                break;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        DbgPrint("[-] Exception Code: 0x%X\n", status);
    }

    return status;
}

/// <summary>
/// Integer Overflow Ioctl Handler
/// </summary>
/// <param name="pIrp">The pointer to IRP</param>
/// <param name="pIoStackIrp">The pointer to IO_STACK_LOCATION structure</param>
/// <returns>NTSTATUS</returns>
NTSTATUS IntegerOverflowIoctlHandler(IN PIRP pIrp, IN PIO_STACK_LOCATION pIoStackIrp) {
    PVOID pUserModeBuffer = NULL;
    SIZE_T userModeBufferSize = 0;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    UNREFERENCED_PARAMETER(pIrp);
    PAGED_CODE();

    pUserModeBuffer = pIoStackIrp->Parameters.DeviceIoControl.Type3InputBuffer;
    userModeBufferSize = pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength;

    if (pUserModeBuffer) {
        status = TriggerIntegerOverflow(pUserModeBuffer, userModeBufferSize);
    }

    return status;
}

#pragma auto_inline()
