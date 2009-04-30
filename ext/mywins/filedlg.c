/*
** $Id: filedlg.c 7570 2007-09-13 09:53:48Z xwyan $
** 
** filedlg.c: Open File Dialog.
** 
** Copyright (C) 2003 ~ 2007 Feynman Software.
** Copyright (C) 2001 ~ 2002 Wei Yongming and others.
**
** Current maintainer: Leon
**
** Create date: 2000.xx.xx
*/

/* ------------------------------------------------------ 
** This FileDialog is based on the one wrote by FrankXM
** and Modified greatly by leon
** -------------------------------------------------------
** main changes:
** -------------------------------------------------------
**         1, change the layout of dialog
**         2, add the 'wildchar' matching 
**            and change other parts accordingly
*/

/*
** TODO:
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"

#ifndef __NOUNIX__

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>

#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "control.h"
#include "filedlg.h"

#define IDC_UP               510
#define IDC_STATIC1          520
#define IDC_STATIC2          522
#define IDC_DIRCHOISE        530
#define IDC_FILECHOISE       540
#define IDC_FILENAME     550
#define IDC_PATH             600



static int fitquestion (char *ptn, char *target);

// filter begin  /////////////////////////////////////////////////////
// match after a '*'
static int fitstar (char *ptn, char *target)
{
    char a[50];
    char *str = a;
    int i, j, k, dismatch;

    for (i=0; (ptn[i] != '*' && ptn[i] != '?' && ptn[i] != '\0'); i++) 
        str[i] = ptn[i];

    if (i ==0) {
        //'\0' ok!
        if (ptn[0] == '\0')
            return 0;
        if (ptn[0] == '*')
            return fitstar (ptn +1, target);
        //'?..' 
        if (ptn[0] == '?') {
            if (target[0] == '\0')
                return 2;
            else
                return fitstar (ptn+1, target+1);
        }
    }
        
    //otherwise give it a 'end'
    str[i] ='\0';
    
    j = 0;
    while (target[j+i-1] != '\0') {
        dismatch = 0;
        for (k=0; k<i; k++)
            if (str[k] != target[j+k]) {
                dismatch = 1;
                break;
            }
        if (dismatch){
            j++;
            continue;
        }
        if (ptn[i] == '\0') {
            if (target[i+j] == '\0')
                return 0;
            else {
                j++;
                continue;
            }
        }
        if (ptn[i] == '*') {
            if (fitstar (ptn+i+1, target+j+k) == 0)
                return 0;
        }else if (ptn[i] == '?') {
            if (target[j+k] == '\0')
                return 2;
            else if (ptn[i+1] == '\0') {
                if (target[j+k+1] == '\0')
                    return 0;
                else 
                    //ptn shorter than target, and the forepart is fit
                    return 1;
            }else
                if (fitquestion (ptn+i+1, target+j+k+1) == 0)
                    return 0;
        }
        j++;
    }
    return 2;
}

// match after a '?'
static int fitquestion (char *ptn, char *target)
{
    char a[50];
    char *str = a;
    int i, j;

    for (i=0; (ptn[i] != '*' && ptn[i] != '?' && ptn[i] != '\0'); i++)
        str[i] = ptn[i];

    if (i ==0) {
        //'\0' ok!
        if (ptn[0] == '\0') {
            if (target[0] == '\0')
                return 0;
            else
                //ptn not long enough!
                return 1;
        }
        if (ptn[0] == '*') 
            return fitstar (ptn+1, target);
        if (ptn[0] == '?') {
            if (target[0] == '\0')
                return 2;
            else
                return fitquestion (ptn+1, target +1);
        }
    }
    str[i] ='\0';

    //must fit on the head
    for (j=0; j<i; j++){
        if (str[j] != target[j])
            return 2;
    }
    if (ptn[i] == '*')
        return fitstar (ptn+i+1, target+i);
    else {
        return fitquestion (ptn+i, target+i);
    }
}

/*//////
    return value:
        0: matches
        1: ptn fites the forepart of target, but not long enough
        2: unmatches
*///////
static int filter( char *ptn, char *target)
{
    if (ptn[0] == '*')
        return fitstar (ptn+1, target);
    else
        return fitquestion (ptn, target);

}

//filter   end///////////////////////////////////////////////////////////////////
static char* GetParentDir (char *dir)
{
    int i, nParent = 0;

    for (i = 0; i < strlen (dir) -1; i++) 
       if (dir [i] == '/') 
            nParent = i;
    dir [nParent + 1] = 0;
       
    return dir;
}

static void myWinFileListDirWithFilter ( HWND hWnd, int dirListID, int fileListID, char* path, char* filtstr)
{
    struct     dirent* pDirEnt;
    DIR*     dir;
    struct     stat ftype;
    char     fullpath [PATH_MAX + 1];
    int     i;

    SendDlgItemMessage (hWnd, IDC_DIRCHOISE, LB_RESETCONTENT, 0, (LPARAM)0);
    SendDlgItemMessage (hWnd, IDC_FILECHOISE, LB_RESETCONTENT, 0, (LPARAM)0);
    SetWindowText (GetDlgItem (hWnd, IDC_PATH), path);
    
    dir = opendir (path);
    while ( (pDirEnt = readdir ( dir )) != NULL ) {

        // Assemble full path name.
        strncpy (fullpath, path, PATH_MAX);
        strcat (fullpath, "/");
        strcat (fullpath, pDirEnt->d_name);
        
        if (stat (fullpath, &ftype) < 0 ){
           //Ping();
           continue;
        }
        if (S_ISDIR (ftype.st_mode))
            SendDlgItemMessage (hWnd, dirListID, LB_ADDSTRING,
                               0, (LPARAM)pDirEnt->d_name);
        else if (S_ISREG(ftype.st_mode)) {
            if (filtstr != NULL) {
                i = filter (filtstr, pDirEnt->d_name);
                if (i == 0 /*|| i == 1 // when responding to the EN_TAB message later*/)
                    SendDlgItemMessage (hWnd, fileListID, LB_ADDSTRING,
                                       0, (LPARAM)pDirEnt->d_name);

            } else
                SendDlgItemMessage (hWnd, fileListID, LB_ADDSTRING, 0, (LPARAM)pDirEnt->d_name);
        }            
    }

    closedir(dir);
}

//leon   end/////////////////////////////////////////////////////////////////

static int
WinFileProc (HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{

    switch (message) {
    case MSG_INITDIALOG:
    {
        PFILEDLGDATA pWinFileData = (PFILEDLGDATA)lParam;

        /* get current directory name */
        if (strcmp (pWinFileData->filepath, ".") == 0 ||
                strcmp (pWinFileData->filepath, "./") == 0)
            getcwd (pWinFileData->filepath, PATH_MAX);

        SetWindowAdditionalData (hDlg, (DWORD)lParam);

        myWinFileListDirWithFilter (hDlg, IDC_DIRCHOISE, IDC_FILECHOISE, pWinFileData->filepath, NULL);
        SetWindowText (GetDlgItem (hDlg, IDC_PATH), pWinFileData->filepath);
        return 1;
    }
    break;

    case MSG_MINIMIZE:
        SendMessage (hDlg, MSG_COMMAND, (WPARAM)IDOK, 0);
    break;

    case MSG_COMMAND:
    {
        PFILEDLGDATA pWinFileData = (PFILEDLGDATA) GetWindowAdditionalData (hDlg);
        int nSelect;    
        int code = HIWORD (wParam);
        int id   = LOWORD (wParam);
        char msg[129];
        
        switch (id) {
        case IDC_DIRCHOISE:
        {
            char dir [NAME_MAX + 1];
#if _DOUBLE_CLICK
            if (code == LBN_DBLCLK || code == LBN_ENTER) {
#else
            if (code == LBN_SELCHANGE || code == LBN_ENTER) {
#endif
                nSelect = SendDlgItemMessage (hDlg, IDC_DIRCHOISE, LB_GETCURSEL, 0, 0);
                 if (nSelect == -1) break;

                SendDlgItemMessage(hDlg, IDC_DIRCHOISE, LB_GETTEXT, nSelect, (LPARAM)dir);
                if (strcmp (dir, ".") == 0)
                    break;
                else if (strcmp(dir, "..") == 0) {
                    GetParentDir (pWinFileData->filepath);    
                    myWinFileListDirWithFilter (hDlg, IDC_DIRCHOISE, IDC_FILECHOISE, pWinFileData->filepath, NULL);
                }
                else {   
                    if (pWinFileData->filepath [strlen (pWinFileData->filepath) - 1] != '/')
                        strcat (pWinFileData->filepath, "/");
                    
                    strcat (pWinFileData->filepath, dir);
                    strcat (pWinFileData->filepath, "/");
                    if (access (pWinFileData->filepath, R_OK) == -1) {
                        sprintf (msg, "No read permission to %s! \n", pWinFileData->filepath);
                        MessageBox (hDlg, msg, "提示信息", MB_OK | MB_ICONSTOP| MB_BASEDONPARENT);
                        GetParentDir (pWinFileData->filepath);
                    }
                }
            
                GetWindowText (GetDlgItem (hDlg, IDC_FILENAME), dir, NAME_MAX);
                SetWindowText (GetDlgItem (hDlg, IDC_PATH), pWinFileData->filepath);
            
                if (dir[0] == '\0')
                    myWinFileListDirWithFilter (hDlg, IDC_DIRCHOISE, IDC_FILECHOISE, pWinFileData->filepath, NULL);
                   else
                    myWinFileListDirWithFilter (hDlg, IDC_DIRCHOISE, IDC_FILECHOISE, pWinFileData->filepath, dir);
            } 
            break;
        }

        case IDC_FILECHOISE:
        {
            if (code == LBN_SELCHANGE || code == LBN_DBLCLK || code == LBN_ENTER) {
                nSelect = SendDlgItemMessage (hDlg, IDC_FILECHOISE,
                                     LB_GETCURSEL, 0, 0);
                if (nSelect != -1) {
                    SendDlgItemMessage(hDlg, IDC_FILECHOISE, LB_GETTEXT, 
                       nSelect, (LPARAM)pWinFileData->filename);
                    SetWindowText (GetDlgItem(hDlg, IDC_FILENAME), 
                       pWinFileData->filename);
                }

                if (code == LBN_DBLCLK || code == LBN_ENTER) {
                    SendNotifyMessage (hDlg, MSG_COMMAND, IDOK, 0);
                }
            }
            break;
        }

#if 0
        case IDC_HOME:
        {
            char a1 [PATH_MAX + 1];
            char *filter = a1;
            
            strcpy (pWinFileData->filepath, getpwuid (getuid ())->pw_dir);
            SetWindowText (GetDlgItem (hDlg, IDC_PATH), pWinFileData->filepath);
            
            GetWindowText(GetDlgItem(hDlg, IDC_FILENAME), filter, NAME_MAX);
                
            if (filter[0] == '\0')
                myWinFileListDirWithFilter (hDlg, IDC_DIRCHOISE, IDC_FILECHOISE, 
                                   pWinFileData->filepath, NULL);
            else
                myWinFileListDirWithFilter (hDlg, IDC_DIRCHOISE, IDC_FILECHOISE, 
                                   pWinFileData->filepath, filter);
            
            break;
        }
#endif

        case IDC_UP:
        {
            char a1[NAME_MAX + 1];
            char *filter = a1;
            
            GetParentDir (pWinFileData->filepath);
            SetWindowText (GetDlgItem (hDlg, IDC_PATH), pWinFileData->filepath);
            
            GetWindowText(GetDlgItem(hDlg, IDC_FILENAME), filter, NAME_MAX);
                
            if (filter[0] == '\0')
                myWinFileListDirWithFilter (hDlg, IDC_DIRCHOISE, IDC_FILECHOISE, 
                                   pWinFileData->filepath, NULL);
            else
                myWinFileListDirWithFilter (hDlg, IDC_DIRCHOISE, IDC_FILECHOISE, 
                                   pWinFileData->filepath, filter);
            break;
        }

        case IDC_FILENAME:
        {
            char dir[PATH_MAX + 1];
            char fn[NAME_MAX + 1];
            char a[NAME_MAX + 1];
            char *filter = a;
            char msg[50];
            int i, nParent = 0;
            
            memset (dir, 0, PATH_MAX +1);
            if (code == EN_ENTER ) { // Add the response to EN_TAB later
        
                GetWindowText(GetDlgItem(hDlg, IDC_FILENAME), fn, NAME_MAX);
            
                for (i = strlen(fn)-1; i>=0; i--) {
                    if (fn [i] == '/') {
                        if (fn [1] != 0)
                            nParent = i;
                        else
                            nParent = 1;

                        strncpy (filter, fn+i+1 , NAME_MAX);
                        //just path only, no filter
                        if (i == strlen (fn)-1) {
                            filter = NULL;
                            SetWindowText (GetDlgItem (hDlg, IDC_FILENAME), "");
                        } else
                            SetWindowText (GetDlgItem (hDlg, IDC_FILENAME), filter);

                        fn[i+1] = 0;
                        break;
                    }
                }

                // filter in the current path
                if (nParent == 0) {
                    //file change
                    if (strlen(fn) == 0)
                        filter = NULL;
                    else
                        strncpy (filter, fn , NAME_MAX);
                    
                    myWinFileListDirWithFilter (hDlg, IDC_DIRCHOISE, IDC_FILECHOISE, 
                                   pWinFileData->filepath, filter);

                // have new path
                } else {
                    if (fn[0] == '/')
                        //absolute path
                        strncpy (dir, fn, PATH_MAX);
                    else {
                        //relative path
                        if (pWinFileData->filepath [strlen (pWinFileData->filepath)-1] != '/')
                            strcat(pWinFileData->filepath,"/");
                        
                        strncpy (dir, pWinFileData->filepath, PATH_MAX);
                        strcat (dir, fn);
                    }

                    if (access (dir, F_OK) == -1){
                        sprintf (msg, "对不起，未找到指定的目录：\n\n %s \n", fn);
                        MessageBox(hDlg , msg, "提示信息", MB_OK | MB_ICONSTOP| MB_BASEDONPARENT);
                    } else {
                        if (access (dir, R_OK) == -1) {
                            sprintf (msg, "不能读取 %s !\n", fn);
                            MessageBox(hDlg , msg, "提示信息", MB_OK | MB_ICONSTOP| MB_BASEDONPARENT);
                        } else if ((pWinFileData->IsSave) && (access (dir, W_OK) == -1)) {
                            sprintf (msg, "对 %s 没有写权限!\n", fn);
                            MessageBox(hDlg , msg, "提示信息", MB_OK | MB_ICONSTOP| MB_BASEDONPARENT);
                        } else {
                            strncpy (pWinFileData->filepath, dir, PATH_MAX);
                            myWinFileListDirWithFilter (hDlg, IDC_DIRCHOISE, IDC_FILECHOISE, 
                                                           pWinFileData->filepath, filter);
                        }
                    }
                }
            }
            break;
        }

        case IDOK:
        {
            char a1[NAME_MAX + 1];
            char a2[NAME_MAX + PATH_MAX + 1];
            char *filter = a1;
            char *fullname = a2;

            nSelect = SendDlgItemMessage(hDlg, IDC_FILECHOISE, LB_GETCURSEL, 0, 0);

            if (nSelect != LB_ERR) {
                GetWindowText(GetDlgItem(hDlg, IDC_FILENAME), filter, NAME_MAX);
                //make up full file name
                if (pWinFileData->filepath[strlen(pWinFileData->filepath)-1] != '/')
                    strcat(pWinFileData->filepath, "/");

                strncpy (pWinFileData->filefullname, pWinFileData->filepath, PATH_MAX);
            
                strncpy (fullname, pWinFileData->filefullname, PATH_MAX);
            
                if (pWinFileData->IsSave) {
                    if ((strchr(filter, '*') == NULL) 
                            && (strchr (filter, '?') == NULL)) {

                        strcat (fullname, pWinFileData->filename);

                        if (access (fullname, F_OK) != -1) {
                            sprintf (msg, "是否要覆盖文件 %s ?\n", fullname);
                            if (MessageBox(hDlg, msg, "提示信息", MB_YESNO | MB_ICONQUESTION | MB_BASEDONPARENT) == IDNO)
                                break;
                        }

                        if (strcmp (filter, pWinFileData->filename) == 0) {
                            if (access (fullname, W_OK) == -1) {
                                sprintf (msg, "对 %s 没有写权限!\n", fullname);
                                MessageBox(hDlg , msg, "提示信息", MB_OK | MB_ICONSTOP| MB_BASEDONPARENT);
                            } else if (access (fullname, R_OK) == -1) {
                                sprintf (msg, "不能读取 %s !\n", fullname);
                                MessageBox(hDlg , msg, "提示信息", MB_OK | MB_ICONSTOP| MB_BASEDONPARENT);
                            } else {
                                strncpy (pWinFileData->filename, filter, NAME_MAX);
                                strncpy (pWinFileData->filefullname, fullname, NAME_MAX + PATH_MAX + 1);
                                EndDialog(hDlg,IDOK);
                            }
                        } else {
                            strncpy (pWinFileData->filefullname, fullname, NAME_MAX + PATH_MAX + 1);
                            EndDialog(hDlg,IDOK);
                        }
                    }
                } else {    
                    strcat (pWinFileData->filefullname,pWinFileData->filename);
                    if (strcmp (filter, pWinFileData->filename) == 0)
                        EndDialog(hDlg,IDOK);
                }
            } else if (pWinFileData->IsSave) {
                // make up full file name
                if (pWinFileData->filepath[strlen(pWinFileData->filepath)-1] != '/')
                    strcat(pWinFileData->filepath,"/");
                strcpy (pWinFileData->filefullname,pWinFileData->filepath);
                GetWindowText(GetDlgItem(hDlg, IDC_FILENAME), filter, NAME_MAX);
                strcat (pWinFileData->filefullname, filter);
                EndDialog(hDlg,IDOK);
            }
            break;
        } 

        default:
            break;

        } /* switch (id) */
        break;

        }
        break;
    } /* switch (message) */

    return DefaultDialogProc (hDlg, message, wParam, lParam);
}

#define SPACE_WIDTH     4
#define SPACE_HEIGHT    4

int OpenFileDialogEx (HWND hWnd, int lx, int ty, int w, int h, PFILEDLGDATA pfdi)
{
    int totalW = w-3*SPACE_WIDTH;
    //int LstH = h -20- 6*SPACE_HEIGHT - GetSysCharHeight() - 20 - 20 - 20;
    //       20: caption                  20: edit 20:button 20:up button
    int LstH = h - 6*SPACE_HEIGHT - GetSysCharHeight()*5 
            - 8*(BTN_WIDTH_BORDER  + 1);
    int LstTY = 2*SPACE_HEIGHT + GetSysCharHeight() + 2*(BTN_WIDTH_BORDER + 1);
    
    CTRLDATA WinFileCtrl [] =
    { 
/*      { "static", WS_VISIBLE | SS_LEFT,
            SPACE_WIDTH, SPACE_WIDTH, 80, GetSysCharHeight(), IDC_STATIC, 
            "选择目录", 0 },
        { "static", WS_VISIBLE | SS_LEFT,
            SPACE_WIDTH+totalW*2/5, SPACE_WIDTH, 80, GetSysCharHeight(), 
            IDC_STATIC, "选择文件", 0 },
        { "button", WS_VISIBLE | WS_TABSTOP ,
            SPACE_WIDTH+totalW*2/5+ 60, SPACE_WIDTH, 22, GetSysCharHeight(), 
            IDC_HOME, "Hm", 0 },
*/
        { "button", WS_VISIBLE | WS_TABSTOP ,
            2*SPACE_WIDTH+totalW*2/5 , SPACE_HEIGHT - 1, 85, 
            GetSysCharHeight()+ 2*(BTN_WIDTH_BORDER + 1),//23,
               IDC_UP, "上一级目录", 0 },

        { "listbox", WS_VISIBLE | WS_TABSTOP | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_SORT,
            SPACE_WIDTH, LstTY, totalW*2/5, LstH, IDC_DIRCHOISE, NULL, 0 },

        { "listbox",WS_VISIBLE | WS_TABSTOP | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_SORT,
            2*SPACE_WIDTH+totalW*2/5, LstTY, totalW*3/5, LstH, 
            IDC_FILECHOISE, NULL, 0 },
            
        { "static", WS_VISIBLE | SS_LEFT,
            SPACE_WIDTH, LstH+LstTY+SPACE_HEIGHT, 4*GetSysCCharWidth(), 
            GetSysCharHeight() + SPACE_HEIGHT, IDC_STATIC1, "当前路径", 0 },

        { "static", WS_VISIBLE | SS_LEFT,
            2*SPACE_WIDTH+4*GetSysCCharWidth(), LstH+LstTY+SPACE_HEIGHT, 
            totalW-4*GetSysCCharWidth()-3*SPACE_WIDTH, GetSysCharHeight(), 
            IDC_PATH, "", 0 },

        { "static", WS_VISIBLE | SS_LEFT,
            SPACE_WIDTH, LstH+LstTY+2*SPACE_HEIGHT+GetSysCharHeight()+1, 
            2*GetSysCCharWidth(), GetSysCharHeight() + SPACE_HEIGHT, 
            IDC_STATIC2, "文件", 0 },
        
        { "sledit",WS_VISIBLE | WS_TABSTOP | WS_BORDER,
            2*SPACE_WIDTH+2*GetSysCCharWidth(), 
            LstH+LstTY+2*SPACE_HEIGHT+GetSysCharHeight(), 
            totalW-2*GetSysCCharWidth()-3*SPACE_WIDTH, 18, 
            IDC_FILENAME, NULL, 0 },
        
        { "button", WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | WS_GROUP,
            (totalW-2*40)*2/5+SPACE_WIDTH, 
            LstH+LstTY+3*SPACE_HEIGHT+GetSysCharHeight()+16, 40, 
            GetSysCharHeight()+ 2*(BTN_WIDTH_BORDER + 1), IDOK, GetSysText(IDS_MGST_OK), 0 },

        { "button", WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            (totalW-2*40)*3/5+2*SPACE_WIDTH+40, 
            LstH+LstTY+3*SPACE_HEIGHT+GetSysCharHeight()+16, 
            40, GetSysCharHeight()+ 2*(BTN_WIDTH_BORDER + 1), IDCANCEL, 
            GetSysText(IDS_MGST_CANCEL), 0 }
    };

    DLGTEMPLATE WinFileDlg = {
#ifdef _FLAT_WINDOW_STYLE
        WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX, WS_EX_NONE,
#else
        WS_BORDER | WS_CAPTION, WS_EX_NONE,
#endif
        lx, ty, w, h, NULL, 0, 0, 9, NULL };

    if (access (pfdi->filepath, F_OK) == -1) 
        return FILE_ERROR_PATHNOTEXIST;             
        
    WinFileCtrl[0].caption = GetSysText(IDS_MGST_UP);
    WinFileCtrl[3].caption = GetSysText(IDS_MGST_CURRPATH);
    WinFileCtrl[5].caption = GetSysText(IDS_MGST_FILE);
    WinFileCtrl[7].caption = GetSysText(IDS_MGST_OK);
    WinFileCtrl[8].caption = GetSysText(IDS_MGST_CANCEL);

    WinFileDlg.caption   = (pfdi->IsSave)?GetSysText(IDS_MGST_SAVEFILE):GetSysText(IDS_MGST_OPENFILE);
    WinFileDlg.controls  = WinFileCtrl;

    return DialogBoxIndirectParam (&WinFileDlg, hWnd,
            WinFileProc, (LPARAM)(pfdi));
}

#endif /* __NOUNIX__ */

