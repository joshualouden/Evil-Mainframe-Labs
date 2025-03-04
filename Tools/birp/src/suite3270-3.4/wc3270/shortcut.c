/*
 * Copyright (c) 1996-2009, 2015 Paul Mattes.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Paul Mattes nor his contributors may be used
 *       to endorse or promote products derived from this software without
 *       specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY PAUL MATTES "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PAUL MATTES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *	shortcut.c
 *		A Windows console-based 3270 Terminal Emulator
 *		Shell link creation
 */

#if !defined(_WIN32) /*[*/
#error For Windows only.
#endif /*]*/

#include <stdio.h>
#include "wincmn.h"

#include "shortcutc.h"
#include "winvers.h"

/*
 * CreateLink - uses the shell's IShellLink and IPersistFile interfaces to
 * create and store a shortcut to the specified object.
 * Returns the result of calling the member functions of the interfaces.
 *  path_obj - address of a buffer containing the path of the object
 *  path_link - address of a buffer containing the path where the shell link
 *   is to be stored
 *  desc - address of a buffer containing the description of the shell link
 */
HRESULT
create_link(LPCSTR path_obj, LPSTR path_link, LPSTR desc, LPSTR args,
	LPSTR dir, int rows, int cols, wchar_t *font, int pointsize,
	int codepage)
{
    HRESULT		hres;
    int	 		initialized;
    IShellLink*		psl = NULL; 
    IShellLinkDataList*	psldl = NULL; 
    IPersistFile*	ppf = NULL;
    NT_CONSOLE_PROPS	p;
    WORD		wsz[MAX_PATH];

    hres = CoInitialize(NULL);
    if (!SUCCEEDED(hres)) {
	fprintf(stderr, "CoInitialize failed\n");
	goto out;
    }
    initialized = 1;

    /* Get a pointer to the IShellLink interface. */
    hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
	    &IID_IShellLink, (LPVOID *)&psl);

    if (!SUCCEEDED(hres)) {
	fprintf(stderr, "CoCreateInstance failed\n");
	goto out;
    }

    /* Set the path to the shortcut target, and add the description. */
    psl->lpVtbl->SetPath(psl, path_obj);
    if (desc) {
	psl->lpVtbl->SetDescription(psl, desc);
    }
    if (args) {
	psl->lpVtbl->SetArguments(psl, args);
    }
    if (dir) {
	psl->lpVtbl->SetWorkingDirectory(psl, dir);
    }

    /* Add the icon. */
    if (windows_major_version >= 5) {
	psl->lpVtbl->SetIconLocation(psl, path_obj, 0);
    }

    hres = psl->lpVtbl->QueryInterface(psl, &IID_IShellLinkDataList,
	    (void **)&psldl);
    if (!SUCCEEDED(hres)) {
	fprintf(stderr, "QueryInterface(DataList) failed\n");
	goto out;
    }

    memset(&p, '\0', sizeof(NT_CONSOLE_PROPS));
    p.cbSize = sizeof(p);
    p.dwSignature = NT_CONSOLE_PROPS_SIG;
    p.wFillAttribute = 7;		/* ? */
    p.wPopupFillAttribute = 245;	/* ? */
    p.dwScreenBufferSize.X = cols;
    p.dwScreenBufferSize.Y = 0x012c;
    p.dwWindowSize.X = cols;
    p.dwWindowSize.Y = rows;
    p.dwWindowOrigin.X = 0;
    p.dwWindowOrigin.Y = 0;
    p.nFont = 0;
    p.nInputBufferSize = 0;
    p.dwFontSize.X = 0;
    p.dwFontSize.Y = pointsize? pointsize: 12;
    p.uFontFamily = 0;			/* FF_DONTCARE */
    p.uFontWeight = 400;		/* FW_NORMAL */
    wcsncpy(p.FaceName, font, LF_FACESIZE - 1);
    p.FaceName[LF_FACESIZE - 1] = 0;
    p.uCursorSize = 100;
    p.bFullScreen = 0;
    p.bQuickEdit = 0;
    p.bInsertMode = 1;
    p.bAutoPosition = 1;
    p.uHistoryBufferSize = 0x32;
    p.uNumberOfHistoryBuffers = 4;
    p.bHistoryNoDup = 0;
    p.ColorTable[0] = 0;
    p.ColorTable[1] =  0x00800000;
    p.ColorTable[2] =  0x00008000;
    p.ColorTable[3] =  0x00808000;
    p.ColorTable[4] =  0x00000080;
    p.ColorTable[5] =  0x00800080;
    p.ColorTable[6] =  0x00008080;
    p.ColorTable[7] =  0x00c0c0c0;
    p.ColorTable[8] =  0x00808080;
    p.ColorTable[9] =  0x00ff8000;
    p.ColorTable[10] = 0x0000ff00;
    p.ColorTable[11] = 0x00ffff00;
    p.ColorTable[12] = 0x000a0adc;
    p.ColorTable[13] = 0x00ff00ff;
    p.ColorTable[14] = 0x0000ffff;
    p.ColorTable[15] = 0x00ffffff;

    hres = psldl->lpVtbl->AddDataBlock(psldl, &p);
    if (!SUCCEEDED(hres)) {
	fprintf(stderr, "AddDataBlock(1) failed\n");
	goto out;
    }

    if (codepage) {
	NT_FE_CONSOLE_PROPS pfe;

	memset(&pfe, '\0', sizeof(pfe));
	pfe.cbSize = sizeof(pfe);
	pfe.dwSignature = NT_FE_CONSOLE_PROPS_SIG;
	pfe.uCodePage = codepage;

	hres = psldl->lpVtbl->AddDataBlock(psldl, &pfe);
	if (!SUCCEEDED(hres)) {
	    fprintf(stderr, "AddDataBlock(2) failed\n");
	    goto out;
	}
    }

    /*
     * Query IShellLink for the IPersistFile interface for saving the
     * shortcut in persistent storage.
     */
    hres = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (void **)&ppf);

    if (!SUCCEEDED(hres)) {
	fprintf(stderr, "QueryInterface(Persist) failed\n");
	goto out;
    }

    /* Ensure that the string is ANSI. */
    MultiByteToWideChar(CP_ACP, 0, path_link, -1, wsz, MAX_PATH);

    /* Save the link by calling IPersistFile::Save. */
    hres = ppf->lpVtbl->Save(ppf, wsz, TRUE);
    if (!SUCCEEDED(hres)) {
	fprintf(stderr, "Save failed\n");
	goto out;
    }

out:
    if (ppf != NULL) {
	ppf->lpVtbl->Release(ppf);
    }
    if (psldl != NULL) {
	psldl->lpVtbl->Release(psldl);
    }
    if (psl != NULL) {
	psl->lpVtbl->Release(psl);
    }

    if (initialized) {
	CoUninitialize();
    }

    return hres;
} 
