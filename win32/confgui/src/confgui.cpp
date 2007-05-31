
#include <windows.h>

int CreateTestPropSheet(HWND);

class WinClass
{
public:
  WinClass(WNDPROC winProc, char const * classname, HINSTANCE hInst);

  void Register()
  {
    ::RegisterClass(&_class);
    
  }
private:
  WNDCLASS _class;
};





WinClass::WinClass(WNDPROC winProc, char const * classname, HINSTANCE hInst)
{
  _class.style = 0;
  _class.lpfnWndProc = winProc;
  _class.cbClsExtra = 0;
  _class.cbWndExtra = 0;
  _class.hInstance = hInst;
  _class.hIcon = 0;
  _class.hCursor = ::LoadCursor(0, IDC_ARROW);
  _class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  _class.lpszMenuName = 0;
  _class.lpszClassName = classname;
}

class WinMaker
{
public:
  WinMaker():_hwnd(0) {}
  WinMaker(char const * caption,
	   char const * className,
	   HINSTANCE hInstance);
  void Show(int cmdShow)
  {



      CreateTestPropSheet(_hwnd);


    //::ShowWindow(_hwnd,cmdShow);
    //::UpdateWindow(_hwnd);
  }
protected:
  HWND _hwnd;
};

WinMaker::WinMaker(char const * caption,
		   char const * className,
		   HINSTANCE hInstance)
{
  _hwnd = ::CreateWindow(className,
			 caption,
			 WS_OVERLAPPEDWINDOW,
			 CW_USEDEFAULT,
			 CW_USEDEFAULT,
			 CW_USEDEFAULT,
			 CW_USEDEFAULT,
			 0,
			 0,
			 hInstance,
			 0);
}






LRESULT CALLBACK
WindowProcedure(HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lParam)
{
  switch (message)

    {  
      /* Need to send a signal back to the main window when this is 

       * quit or canceled to exit the main window.

       */

    case WM_CREATE:
      return 0;

      

    case WM_DESTROY:

      ::PostQuitMessage(0);

    return 0;

    }

  return ::DefWindowProc(hwnd, message, wparam, lParam);

  

}

int WINAPI
WinMain (HINSTANCE hInstance, HINSTANCE hprevInst,
	 char * cmdParam, int cmdShow)
{
  char className[] = "Win";

  WinClass winclass (WindowProcedure, className, hInstance);
  winclass.Register();
  WinMaker win ("windows", className, hInstance);
 
  /* Don't show to get only the modal dialog, but have to 

   * handle the command from the modal dialog to be able to 

   * exit the app properly.

   */

  win.Show(cmdShow);
  

  MSG msg;
  int status;

  while ( (status = ::GetMessage(&msg, 0, 0, 0)) != 0)
    {
      if (status == -1)
	return -1;
      ::DispatchMessage(&msg);
    }

  return msg.wParam;
}

