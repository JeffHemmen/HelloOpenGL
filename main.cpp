#include <windows.h>                                                             //Header file for Windows
#include <gl/gl.h>                                                              //Header for OpenGL32 lib
#include <gl/glu.h>                                                             //Header f. the GLu32 lib
#include <gl/glaux.h>                                                           //Header f. the GLaux lib

HGLRC     hRC  = NULL;                                                          //Permanent rendering Context
HDC       hDC  = NULL;                                                          //Private GDI Device Context
HWND      hWnd = NULL;                                                          //Windlow Handle
HINSTANCE hInstance;                                                            //holds instance of application

bool keys[256];                                                                 //array used for keyboard routine
bool active = true;                                                             //is windows active? def: y
bool fullscreen = true;                                                         //fullscreen? def: y

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);                           //prototype for WinProc

GLvoid ReSizeGLScene(GLsizei x, GLsizei y) {
       if(y==0)
            y = 1;                                                              //prevent div. by zero
       glViewport(0, 0, x, y);                                                  //reset current viewport
       
       glMatrixMode(GL_PROJECTION);                                             //select projection matrix
       glLoadIdentity();                                                           //reset projection
       
       //Calculate Aspect Ratio of the Window
       gluPerspective(45.0f, (GLfloat)x/(GLfloat)y, 0.1f, 100.0f);
       
       glMatrixMode(GL_MODELVIEW);                                               //select modelview matrix
       glLoadIdentity();                                                        //reset modelview matrix
}

int InitGL(GLvoid) {                                                            //all setup for OpenGL goes here!

    glShadeModel(GL_SMOOTH);                                                    //smooth shading
    glClearColor(0.0f, 0.3f, 0.1f, 0.0f);                                       //background colour
    
    glClearDepth(1.0f);                                                         //depth buffer stup
    glEnable(GL_DEPTH_TEST);                                                    //enabled depth testing
    glDepthFunc(GL_LEQUAL);                                                     //type of depth test to do
    
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);                          //really nice perspective calculations
    
    return true;
}

int DrawGLScene(GLvoid) {                                                       //all drawing here!
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);                         //clear screen and depth buffer
    glLoadIdentity();                                                           //reset current modelview matrix
    return true;                                                                //everything went OK
}

GLvoid KillGLWindow(GLvoid) {                                                   //properly kill window

       if(fullscreen) {                                                         //if in fullscreen
            ChangeDisplaySettings(NULL, 0);                                     //switch back to desktop
            ShowCursor(true);                                                   //show cursor
       }
       
       if(hRC) {                                                                //if we have a rendering context
            if(!wglMakeCurrent(NULL, NULL)) {                                   //if release DC and RC failed
                 MessageBox(NULL, "Release of DC and RC failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONERROR);
            }                                                                   //show error-messagebox
            
            if(!wglDeleteContext(hRC)) {                                        //if delete RC failed
                 MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONERROR);
            }
            
            hRC = NULL;                                                         //set RC to null
       }
       
       if(hDC && !ReleaseDC(hWnd, hDC)) {                                       //if release DC failed
            MessageBox(NULL, "Release Device Context Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONERROR);
            hDC = NULL;                                                         //show error-messagebox
       }                                                                        //hDC now NULL
       
       if(hWnd && !DestroyWindow(hWnd)) {                                       //if destroy window failed
            MessageBox(NULL, "Could not release hWnd.", "SHUTDOWN ERROR", MB_OK | MB_ICONERROR);
            hWnd = NULL;                                                        //set hWnd to NULL
       }
       
       if(!UnregisterClass("OpenGL", hInstance)) {
            MessageBox(NULL, "Could not Unregister Class.", "SHUTDOWN ERROR", MB_OK | MB_ICONERROR);
            hInstance = NULL;                                                   //set hInstance to NULL
       }
}

bool CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag) {
     
     GLuint PixelFormat;                                                        //holds result after searching for match
     
     WNDCLASS wc;                                                               //window class structure
     
     DWORD dwExStyle;                                                           //window extended style
     DWORD dwStyle;                                                             //window style
     
     RECT Windowrect;                                                           //hilds rect. upper left & lower right values
     Windowrect.left   = (long) 0;                                              //set left to 0
     Windowrect.right  = (long) width;                                          //set right to requested width
     Windowrect.top    = (long) 0;                                              //set top to 0
     Windowrect.bottom = (long) height;                                         //set botton to rewuested height
     
     fullscreen = fullscreenflag;                                               //set *global* fullscreen flag
     
     hInstance        = GetModuleHandle(NULL);                                  //get instance for oour window
     wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;                     //redraw on move and own DC for window
     wc.lpfnWndProc   = (WNDPROC) WndProc;                                      //WndProc handles messages
     wc.cbClsExtra    = 0;                                                      //no extra wnd data
     wc.cbWndExtra    = 0;                                                      //no extra wnd data
     wc.hInstance     = hInstance;                                              //set instance
     wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);                            //load def. icon
     wc.hCursor       = LoadCursor(NULL, IDC_ARROW);                            //load arrow pointer
     wc.hbrBackground = NULL;                                                   //no background required for GL
     wc.lpszMenuName  = NULL;                                                   //no menu
     wc.lpszClassName = "OpenGL";                                               //set class name
     
     if(!RegisterClass(&wc)) {                                                  //if register class fails
          MessageBox(NULL, "Failed to register the Window Class.", "ERROR", MB_OK | MB_ICONERROR);
          return false;                                                         //exit and return false
     }
     
     if(fullscreen){                                                            //if fullscreen mode
          DEVMODE dmScreenSettings;                                             //device mode
          memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));               //clears memory
          dmScreenSettings.dmSize       = sizeof(dmScreenSettings);             //size of devmode struct
          dmScreenSettings.dmPelsWidth  = width;                                //selected screen width
          dmScreenSettings.dmPelsHeight = height;                               //selected screen height
          dmScreenSettings.dmBitsPerPel = bits;                                 //selected bits per pixel
          dmScreenSettings.dmFields     = DM_BITSPERPEL;
          dmScreenSettings.dmFields    |= DM_PELSWIDTH | DM_PELSHEIGHT;
          
          //try to set to fullscreen [Note: CDS_FULLSCREEN gets rid of startbar]
          if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
               //if mode fails, give two options: windowed or quit
               if(MessageBox(NULL, "The requested fullscreen mode is not supported\nby your video card. Use windowed mode instead?", "Fullscreen Error", MB_YESNO | MB_ICONERROR) == IDYES)
                    fullscreen = false;                                         //use windowed mode
               else {
                   //let user know program is closing
                   MessageBox(NULL, "Program will now close", "Fullscreen Error", MB_OK | MB_ICONINFORMATION);
                   return false;                                                //just quit
               }
          }
     }
     
     if(fullscreen) {                                                           //if still fullscreen
          dwExStyle = WS_EX_APPWINDOW;                                          //window extended style
          dwStyle   = WS_POPUP;                                                 //window style
          ShowCursor(false);                                                    //hide mouse pointer
     }
     else {
          dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;                       //window extended style
          dwStyle   = WS_OVERLAPPEDWINDOW;                                      //window style
     }
     
     AdjustWindowRectEx(&Windowrect, dwStyle, false, dwExStyle);                //adjust wnd to true requested size
     
     if(!(hWnd=CreateWindowEx( dwExStyle,                                       //extended style for the window
                               "OpenGL",                                        //class name
                               title,                                           //window title
                               WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwStyle,     //required window styles
                               0, 0,                                            //window position
                               Windowrect.right-Windowrect.left,                //calculate adjusted wnd width
                               Windowrect.bottom-Windowrect.top,                //~~~~~~~~~~~~~~~~~~~~~~ height
                               NULL,                                            //no parent menu
                               NULL,                                            //no menu
                               hInstance,                                       //instane
                               NULL))) {                                        //don't pass anything to WM_CRETAE
                               
          KillGLWindow();                                                       //reset display
		  MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		  return FALSE;							                                //return false
	}
	
	static	PIXELFORMATDESCRIPTOR pfd=					                        // pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),					                        // Size Of This Pixel Format Descriptor
		1,								                                        // Version Number
		PFD_DRAW_TO_WINDOW |						                            // Format Must Support Window
		PFD_SUPPORT_OPENGL |						                            // Format Must Support OpenGL
		PFD_DOUBLEBUFFER,						                                // Must Support Double Buffering
		PFD_TYPE_RGBA,							                                // Request An RGBA Format
		bits,								                                    // Select Our Color Depth
		0, 0, 0, 0, 0, 0,						                                // Color Bits Ignored
		0,								                                        // No Alpha Buffer
		0,								                                        // Shift Bit Ignored
		0,								                                        // No Accumulation Buffer
		0, 0, 0, 0,							                                    // Accumulation Bits Ignored
		16,								                                        // 16Bit Z-Buffer (Depth Buffer)
		0,								                                        // No Stencil Buffer
		0,								                                        // No Auxiliary Buffer
		PFD_MAIN_PLANE,							                                // Main Drawing Layer
		0,								                                        // Reserved
		0, 0, 0								                                    // Layer Masks Ignored
	};

	if (!(hDC=GetDC(hWnd)))							                            // Did We Get A Device Context?
	{
		KillGLWindow();							                                // Reset The Display
		MessageBox(NULL, "Cannot create a GL Device Context.", "ERROR", MB_OK | MB_ICONERROR);
		return FALSE;							                                // Return FALSE
	}
	
	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))				                // Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();							                                // Reset The Display
		MessageBox(NULL, "Cannot find a suitable PixelFormat.", "ERROR", MB_OK | MB_ICONERROR);
		return FALSE;							                                // Return FALSE
	}
    
    if(!SetPixelFormat(hDC,PixelFormat,&pfd))				                    // Are We Able To Set The Pixel Format?
	{
		KillGLWindow();							                                // Reset The Display
		MessageBox(NULL, "Cannot Set The PixelFormat.", "ERROR", MB_OK | MB_ICONERROR);
		return FALSE;							                                // Return FALSE
	}

    
    if (!(hRC=wglCreateContext(hDC)))					                        // Are We Able To Get A Rendering Context?
	{
		KillGLWindow();							                                // Reset The Display
		MessageBox(NULL, "Cannot create a GL Rendering Context.", "ERROR", MB_OK | MB_ICONERROR);
		return FALSE;							                                // Return FALSE
	}
    
    if(!wglMakeCurrent(hDC,hRC))						                        // Try To Activate The Rendering Context
	{
		KillGLWindow();							                                // Reset The Display
		MessageBox(NULL, "Can't activate the GL Rendering Context.", "ERROR", MB_OK | MB_ICONERROR);
		return FALSE;							                                // Return FALSE
	}
    
    ShowWindow(hWnd, SW_SHOW);                                                  //show window
    SetForegroundWindow(hWnd);                                                  //slightly higher priority
    SetFocus(hWnd);                                                             //sets keyboard focus to window
    ReSizeGLScene(width, height);                                               //set up our perspective GL screen
    
    if(!InitGL()){
         KillGLWindow();                                                        //reset display
         MessageBox(NULL, "Initialisation Failed.", "ERROR", MB_OK | MB_ICONERROR);
         return false;                                                          //return false
    }
    
    return true;                                                                //success :D
    
}

LRESULT CALLBACK WndProc( HWND	    hWnd,					                    // Handle For This Window
				          UINT	    uMsg,					                    // Message For This Window
			              WPARAM	wParam,					                    // Additional Message Information
				          LPARAM	lParam)	{				                    // Additional Message Information
     switch(uMsg) {
          
          case WM_ACTIVATE: {                                                   //watch for window activate message
               if(!HIWORD(wParam))                                              //check minimisation state
                    active = true;                                              //prog is active
               else
                   active = false;                                              //prog is no longer active
               return 0;                                                        //return to message loop
          }
          
          case WM_SYSCOMMAND: {                                                 //intercept system commands
               switch(wParam) {                                                 //check system calls
                    case SC_SCREENSAVE:                                         //screensaver starting?
                    case SC_MONITORPOWER:                                       //monitor going to powersave?
                         return 0;                                              //prevent from happening
               }
               break;                                                           //exit
          }
          
          case WM_CLOSE: {                                                      //did we get a close message?
               PostQuitMessage(0);                                              //send quit message
               return 0;                                                        //jump back
          }
          
          case WM_KEYDOWN: {                                                    //has key been pressed?
               keys[wParam] = true;                                             //mark as true
               return 0;                                                        //jump back
          }
          
          case WM_KEYUP: {
               keys[wParam] = false;
               return 0;
          }
          
          case WM_SIZE: {                                                       //resize opengl window
               ReSizeGLScene(LOWORD(wParam), HIWORD(wParam));                   //LoWord=width; HiWord=height
               return 0;
          }
     }
     //Pass all unhandled messages to defWindowProc
     return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain( HINSTANCE hInstance,                                        //Instance
                    HINSTANCE hPrevInstance,                                    //previous instance
                    LPSTR lpCmdLine,                                            //command line parameters
                    int nCmdShow) {                                             //window show state
     
     MSG msg;                                                                   //windows message struct
     bool done = false;                                                         //bool var to exit loop
     
     // Ask The User Which Screen Mode They Prefer
	 if (MessageBox(NULL, "Would You Like To Run In Fullscreen Mode?", "Start FullScreen?", MB_YESNO | MB_ICONQUESTION) == IDNO)
	 {
          fullscreen = false;						                            //Windowed Mode
     }
     
     // Create Our OpenGL Window
	 if (!CreateGLWindow("Jeff's OpenGL Framework", 640, 480, 16, fullscreen))
	 {
          return 0;							                                    // Quit If Window Was Not Created
	 }
	 
	 while(!done) {
          if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {                        //is there a msg waiting?
               if(msg.message == WM_QUIT)                                       //got a quit message?
                    done = true;                                                //done
               else {                                                           //else, deal with window messages
                    TranslateMessage(&msg);                                     //translate message
                    DispatchMessage(&msg);                                      //dispatch message
               }
          }
          else {                                                                //if there are no messages
               //draw the scene. watch for ESC key and QUIT msgs from DrawGLScene()
               if(active) {
                    if(keys[VK_ESCAPE])                                         //ESC?
                         done = true;                                           //done
                    else {
                         DrawGLScene();                                         //draw scene
                         SwapBuffers(hDC);                                      //swap buffers (double buffering)
                    }
               }
               
               if(keys[VK_F11]) {                                               //F11?
                    keys[VK_F11] = false;                                       //make key false
                    KillGLWindow();                                             //kill current window
                    fullscreen = !fullscreen;                                   //toggle fullscreen
                    //Recreate openGL window
                    if(!CreateGLWindow("Jeff's OpenGL Framework", 640, 480, 16, fullscreen))
                         return 0;                                              //quit if fail
               }
          }
     }
     //Shutdown
     KillGLWindow();                                                            //kill window
     return (msg.wParam);                                                       //exit program
}



















