// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal.cc 12 2005-04-18 13:52:09Z vhex $

#include "../common/common.h"

extern int fix_exports();

extern "C" BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID)
{
        if (reason == DLL_PROCESS_ATTACH) {
                WSADATA wsa;
		if (fix_exports() == 0)
			return FALSE;
                if (WSAStartup(0x0201, &wsa) != 0)
                        return FALSE;
        } else if (reason == DLL_PROCESS_DETACH) {
                WSACleanup();
        }

        return TRUE;
}
