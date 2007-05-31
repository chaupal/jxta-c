
#define STRICT

#include <stdio.h>

#include <windows.h>
#include <prsht.h>		// includes the property sheet functionality
#include <assert.h>
#include <commdlg.h>
#include <commctrl.h>
#include <htmlhelp.h>

#include "resource.h"
#include "filebytes.h"

#include "jxtaconf.h"


BOOL APIENTRY NetworkProc(HWND,UINT,WPARAM,LPARAM);
BOOL APIENTRY PeerNameProc(HWND,UINT,WPARAM,LPARAM);
BOOL APIENTRY RendezvousProc(HWND,UINT,WPARAM,LPARAM);
BOOL APIENTRY RouterProc(HWND,UINT,WPARAM,LPARAM);


Jxtaconf * jconf;

void FillInPropertyPage( PROPSHEETPAGE* , int, LPSTR, DLGPROC, UINT, LPARAM, HINSTANCE);


/* Need to send a signal back to the main window when this is 
* quit or canceled to exit the main window.
*/
int 
CreateTestPropSheet(HWND hwnd) //, Analysisdata * ad)
{
   INITCOMMONCONTROLSEX commctl;
   PROPSHEETPAGE psp[4];
   PROPSHEETHEADER psh;
   LPARAM lparam;



   //Analysisdata * adnew = NULL;// = cloneAnalysisData(ad->this);
   //int i;
   HINSTANCE hInst = (HINSTANCE)GetWindowLong(hwnd,GWL_HINSTANCE);
   //HWND handle;
   
   //for (i=0;i<sizeof(psp)/sizeof(PROPSHEETPAGE);i++)
   //   psp[i].hInstance = hInst;
   
   /*
   To use an IP address control, call InitCommonControlsEx 
   with the ICC_INTERNET_CLASSES flag set in the dwICC member of 
   the INITCOMMONCONTROLSEX structure. 
*/
   commctl.dwICC |= ICC_INTERNET_CLASSES;
   InitCommonControlsEx(&commctl);


   jconf = new Jxtaconf();
   
   /* Before doing a lot of work here, read in the config file into 
   * a byte buffer in case the prop sheet gets canceled oe the 
   * user quits for some reason.
   */
   
   
   lparam = (LPARAM)hwnd; //adnew;
   
   FillInPropertyPage( &psp[0], IDD_PEERNAME, TEXT("&Peer Name"), PeerNameProc, IDI_ICON1, lparam, hInst);
   FillInPropertyPage( &psp[1], IDD_NETWORK, TEXT("&Network"), NetworkProc, IDI_ICON3, lparam, hInst);
   FillInPropertyPage( &psp[2], IDD_RENDEZVOUS, TEXT("Rende&zvous"), RendezvousProc, IDI_ICON4, lparam, hInst);
   FillInPropertyPage( &psp[3], IDD_ROUTER, TEXT("&Routing"), RouterProc, IDI_ICON5, lparam, hInst);
   
   
   psh.dwSize = sizeof(PROPSHEETHEADER);
   psh.dwFlags = PSH_PROPSHEETPAGE   | 
                 PSH_HASHELP	       |
                 PSH_USEHICON;
   psh.hwndParent = hwnd;
   psh.hInstance = hInst;
   psh.pszCaption = (LPSTR) TEXT("JXTA Configuration");
   psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
   psh.nStartPage = 0;
   psh.ppsp = (LPCPROPSHEETPAGE) &psp;
   psh.hIcon = (HICON)LoadImage(hInst,	
      MAKEINTRESOURCE(IDI_ICON3), 
      IMAGE_ICON, 
      LR_DEFAULTSIZE,
      LR_DEFAULTSIZE,
      LR_DEFAULTCOLOR);
   
   PropertySheet(&psh);
   return TRUE;
   
} 



//
//  FUNCTION: FillInPropertyPage(PROPSHEETPAGE *, int, LPSTR, LPFN) 
//
//  PURPOSE: Fills in the given PROPSHEETPAGE structure 
//
//  COMMENTS:
// 	  This function fills in a PROPSHEETPAGE structure with the
// 	  information the system needs to create the page.
// 
void 
FillInPropertyPage( PROPSHEETPAGE* psp, 
                   int idDlg, 
                   LPSTR pszProc, 
                   DLGPROC pfnDlgProc,
                   UINT icon,
                   LPARAM lparam,
                   HINSTANCE hInst)
{
   psp->dwSize = sizeof(PROPSHEETPAGE);
   psp->dwFlags = PSP_USETITLE | PSP_USEHICON;
   psp->hInstance = hInst;
   psp->pszTemplate = MAKEINTRESOURCE(idDlg);
   psp->pszIcon = NULL;
   psp->pfnDlgProc = pfnDlgProc;
   //psp->pfnCallback = pfnDlgProc;
   psp->pszTitle = pszProc;
   psp->lParam = lparam;
   
   psp->hIcon = (HICON)LoadImage(hInst,	
      MAKEINTRESOURCE(icon), 
      IMAGE_ICON, 
      LR_DEFAULTSIZE,
      LR_DEFAULTSIZE,
      LR_DEFAULTCOLOR);
   
}	/* close FillPropertyPage() */



void
handleNotify(HWND hparent, LPARAM lParam)
{
   
   switch (((NMHDR FAR *) lParam)->code) 
   {
   case PSN_SETACTIVE:
      //MessageBox(NULL,"SETACTIVE","SETACTIVE",MB_OK);
      break;

   case PSN_APPLY:
      {
         jconf->write_jxta_conf();
         PostMessage(hparent,WM_DESTROY,0,0);
      }
      break;

   case PSN_KILLACTIVE:
      //(NULL,"KILLACTIVE","KILLACTIVE",MB_OK);
      //PostMessage(hparent,WM_DESTROY,0,0);
      jconf->write_original_data();
      break;

   case PSN_RESET:
      //MessageBox(NULL,"RESET","RESET",MB_OK);
      PostMessage(hparent,WM_DESTROY,0,0);
      break;
   }
}



BOOL APIENTRY
PeerNameProc(HWND hDlg, UINT messageID, WPARAM wParam, LPARAM lParam)
{
   static HWND hparent;
   char temp[100];
   
   switch(messageID)
   {
   case WM_INITDIALOG:
      {
         PROPSHEETPAGE * psp = (PROPSHEETPAGE *)lParam;
         hparent = (HWND)psp->lParam;
         //assert((Analysisdata *)psp->lParam == adglobalfortest);
      }	break;
      
      
   case WM_COMMAND:
      {
         //HWND hwndCtl = (HWND) lParam;      
         switch (LOWORD(wParam))
         {
         case IDC_PEERNAME:
            GetDlgItemText(hDlg,IDC_PEERNAME, temp, 100);
            jconf->set_peer_name(temp);
            //MessageBox(NULL,"WM_COMMAND","WM_COMMAND",MB_OK);
            break;
         case IDC_EDIT2:
            //SendMessage(hwndCtl,WM_SETTEXT,(WPARAM)0,(LPARAM)"foo");
            GetDlgItemText(hDlg,IDC_EDIT2, temp, 100);
            jconf->set_host_name(temp);
            //MessageBox(NULL,"WM_COMMAND","WM_COMMAND",MB_OK);
            break;
         }
      }
      break;

      
   case WM_NOTIFY:
      /* Need to make a class for the containing dialog box. */
      handleNotify(hparent, lParam);
      break;
   }
   
   return 0;
}


BOOL APIENTRY
NetworkProc(HWND hDlg, UINT messageID, WPARAM wParam, LPARAM lParam)
{
   static HWND hparent;
   
   switch(messageID)
   {
   case WM_INITDIALOG:
      {
         PROPSHEETPAGE * psp = (PROPSHEETPAGE *)lParam;
         hparent = (HWND)psp->lParam;
         //assert((Analysisdata *)psp->lParam == adglobalfortest);
      }	break;
      
   case WM_COMMAND:
      {
         int idEditCtrl = (int) LOWORD(wParam); // identifier of edit control 
         int wNotifyCode = HIWORD(wParam);		// notification code 
      } break;
      
   case WM_NOTIFY:
      {
         handleNotify(hparent,lParam);
         //MessageBox(NULL,"Notify message", "Notify message",MB_OK);
      }
   }
   return 0;
}



BOOL APIENTRY
RendezvousProc(HWND hDlg, UINT messageID, WPARAM wParam, LPARAM lParam)
{
   static HWND hparent;
   char temp[100];
   
   switch(messageID)
   {
   case WM_INITDIALOG:
      {
         PROPSHEETPAGE * psp = (PROPSHEETPAGE *)lParam;
         hparent = (HWND)psp->lParam;
         //assert((Analysisdata *)psp->lParam == adglobalfortest);
      }	break;
      
      
   case WM_COMMAND:
      {
         //HWND hwndCtl = (HWND) lParam;      
         switch (LOWORD(wParam))
         {
         case IDC_EDIT2:
            //SendMessage(hwndCtl,WM_SETTEXT,(WPARAM)0,(LPARAM)"foo");
            GetDlgItemText(hDlg,IDC_EDIT2, temp, 100);
            jconf->set_host_name(temp);
            //MessageBox(NULL,"WM_COMMAND","WM_COMMAND",MB_OK);
            break;
         }
      }
      break;

      
   case WM_NOTIFY:
      handleNotify(hparent, lParam);
      break;
   }
   
   return 0;
}	



BOOL APIENTRY
RouterProc(HWND hDlg, UINT messageID, WPARAM wParam, LPARAM lParam)
{
   static HWND hparent;
   char temp[100];
   
   switch(messageID)
   {
   case WM_INITDIALOG:
      {
         PROPSHEETPAGE * psp = (PROPSHEETPAGE *)lParam;
         hparent = (HWND)psp->lParam;
         //assert((Analysisdata *)psp->lParam == adglobalfortest);
      }	break;
      
      
   case WM_COMMAND:
      {
         //HWND hwndCtl = (HWND) lParam;      
         switch (LOWORD(wParam))
         {
         case IDC_EDIT2:
            //SendMessage(hwndCtl,WM_SETTEXT,(WPARAM)0,(LPARAM)"foo");
            GetDlgItemText(hDlg,IDC_EDIT2, temp, 100);
            jconf->set_host_name(temp);
            //MessageBox(NULL,"WM_COMMAND","WM_COMMAND",MB_OK);
            break;
         }
      }
      break;

   case WM_NOTIFY:
      handleNotify(hparent, lParam);
      break;
   }
   
   return 0;
}
