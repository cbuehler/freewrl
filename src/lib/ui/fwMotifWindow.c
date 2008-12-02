/*
=INSERT_TEMPLATE_HERE=

$Id: fwMotifWindow.c,v 1.1 2008/11/27 01:51:43 couannette Exp $

???

*/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeX3D.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/CScripts.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CParse.h"

#include <float.h>

#include "../x3d_parser/Bindable.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"


#ifdef HAVE_MOTIF

#define ABOUT_FREEWRL "FreeWRL Version %s\n \
%s %s.\n \n \
FreeWRL is a VRML/X3D Browser for OS X and Unix.\n \n \
FreeWRL is maintained by:\nJohn A. Stewart and Sarah J. Dumoulin.\n \n \
Contact: freewrl-09@rogers.com\n \
Telephone: +1 613-998-2079\nhttp://www.crc.ca/FreeWRL\n \n \
Thanks to the Open Source community for all the help received.\n \
Communications Research Centre\n \
Ottawa, Ontario, Canada.\nhttp://www.crc.ca"

#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>

#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/CascadeB.h>
#include <Xm/Frame.h>
#include <Xm/TextF.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>
#include <Xm/PanedW.h>
#include <Xm/Text.h>
#include <Xm/ScrolledW.h>
#include <Xm/FileSB.h>
extern WidgetClass glwDrawingAreaWidgetClass;

void setDefaultBackground(int colour);

/************************************************************************

Set window variables from FreeWRL 

************************************************************************/


/* background colours - must be sequential range */
#define colourBlack 	0
#define colourRed	1
#define colourGreen	2
#define colourBlue	3
#define colourMagenta	4
#define colourYellow	5
#define colourCyan	6
#define colourGrey	7
#define colourOrange	8
#define colourWhite	9

/* because of threading issues in Linux, if we can only use 1 thread, we
   delay setting of info until this time. */
int colbut; int colbutChanged = FALSE;
int headbut; int headbutChanged = FALSE;
int fl, ex, wa; int navbutChanged = FALSE;
int msgChanged = FALSE;
char *consMsg = NULL; int consmsgChanged = FALSE;
int localtexpri = TRUE; /* mimics textures_take_priority in CFuncs/RenderFuncs.c */
int localshapepri = TRUE; /* mimics textures_take_priority in CFuncs/RenderFuncs.c */
#define MAXSTAT 200
char fpsstr[MAXSTAT+20];

static String defaultResources[200];
int MainWidgetRealized = FALSE;
int msgWindowOnscreen = FALSE;
int consWindowOnscreen = FALSE;

Widget freewrlTopWidget, mainw, menubar,menuworkwindow;
Widget menumessagewindow = NULL; /* tested against NULL in code */
Widget frame, freewrlDrawArea;
Widget headlightButton;
Widget collisionButton;
Widget walkButton, flyButton, examineButton;
Widget menumessageButton;
Widget consolemessageButton;
Widget consoleTextArea;
Widget consoleTextWidget;
Widget about_widget;
Widget newFileWidget;
Widget tex128_button, tex256_button, texFull_button, texturesFirstButton, shapeThreadButton;

/* colour selection for default background */
Widget backgroundColourSelector[colourWhite+1];
String BackString[] = {"Black Background", "Red Background", "Green Background", "Blue Background", "Magenta Background", "Yellow Background", "Cyan Background", "Grey Background", "Orange Background", "White Background"};
float backgroundColours[] = {
		0.0, 0.0, 0.0, 		/* black */
		0.8, 0.0, 0.0, 		/* red */
		0.0, 0.8, 0.0,		/* green */
		0.0, 0.0, 0.8,		/* blue */
		0.8, 0.0, 0.8,		/* magenta */
		0.8, 0.8, 0.0,		/* yellow */		
		0.0, 0.8, 0.8,		/* cyan */
		0.8, 0.8, 0.8,		/* grey */
		1.0, 0.6, 0.0,		/* orange */
		1.0, 1.0, 1.0};		/* white */

Arg args[10];
Arg buttonArgs[10]; int buttonArgc = 0;
Arg textArgs[10]; int textArgc = 0;
XtAppContext freewrlXtAppContext;
extern char myMenuStatus[];
extern float myFps;

void createMenuBar(void);
void createDrawingFrame(void);


void myXtManageChild (int c, Widget child) {
	#ifdef XTDEBUG
	printf ("at %d, managing %d\n",c, child);
	#endif

	if (child != NULL) XtManageChild (child);
}


/* see if/when we become iconified - if so, dont bother doing OpenGL stuff */
XtEventHandler StateWatcher (Widget w, caddr_t unused, XEvent *event) {
if (event->type == MapNotify) setDisplayed (TRUE);
        else if (event->type == UnmapNotify) setDisplayed (FALSE);
}

void createMotifMainWindow() {
        Dimension width, height;

	mainw = XmCreateMainWindow (freewrlTopWidget, "mainw", NULL, 0);
	myXtManageChild (29,mainw);

	/* Create a menu bar. */
	createMenuBar();

	/* Create a framed drawing area for OpenGL rendering. */
	createDrawingFrame();

	/* Set up the application's window layout. */
	XtVaSetValues(mainw, 
		XmNworkWindow, frame,
		XmNcommandWindow,  NULL,
		XmNmenuBar, menubar,
		XmNmessageWindow, menumessagewindow,
		NULL);


	XtRealizeWidget (freewrlTopWidget);
	MainWidgetRealized = TRUE;

	/* let the user ask for this one We do it here, again, because on Ubuntu,
	 * this pops up on startup. Maybe because it has scrollbars, or other internal
	 * widgets? */
	XtUnmanageChild(consoleTextWidget);


	Xwin = XtWindow (freewrlTopWidget);

	/* now, lets tell the OpenGL window what its dimensions are */

        XtVaGetValues (freewrlDrawArea, XmNwidth, &width, XmNheight, &height, NULL);
        setScreenDim (width,height);

	/* lets see when this goes iconic */
	XtAddEventHandler (freewrlTopWidget, StructureNotifyMask, FALSE, StateWatcher, NULL);

}

/************************************************************************

Callbacks to handle button presses, etc.

************************************************************************/

/* Label strings are "broken" on some Motifs. See:
 * http://www.faqs.org/faqs/motif-faq/part5/
 */
/* both of these fail on Ubuntu 6.06 */
/* diastring = XmStringCreateLtoR(ns,XmFONTLIST_DEFAULT_TAG); */
/*diastring = XmStringCreateLocalized(ns); */


XmString xec_NewString(char *s)
{
	XmString xms1;
	XmString xms2;
	XmString line;
	XmString separator;
	char     *p;
	char     *t = XtNewString(s);   /* Make a copy for strtok not to */
	                            /* damage the original string    */

	separator = XmStringSeparatorCreate();
	p         = strtok(t,"\n");
	xms1      = XmStringCreateLocalized(p);

	while (p = strtok(NULL,"\n"))
	{
		line = XmStringCreateLocalized(p);
		xms2 = XmStringConcat(xms1,separator);
		XmStringFree(xms1);
		xms1 = XmStringConcat(xms2,line);
		XmStringFree(xms2);
		XmStringFree(line);
	}

	XmStringFree(separator);
	XtFree(t);
	return xms1;
}

/* Callbacks */
void aboutFreeWRLpopUp (Widget w, XtPointer data, XtPointer callData) { 
	int ac;
	Arg args[10];
	char ns[2000];
	XmString diastring;
	ac = 0;
	/*
	printf ("version is %s\n",GL_VER);
	printf ("vendor is %s\n",GL_VEN);
	printf ("renderer is %s\n",GL_REN);
	*/

	/* set the string here - OpenGL is now opened. */
	sprintf (ns,ABOUT_FREEWRL,getLibVersion(),"Render: ",GL_REN);
	diastring = xec_NewString(ns);

	
	XtSetArg(args[ac], XmNmessageString, diastring); ac++;
	XtSetValues(about_widget,args,ac);
	XmStringFree(diastring);

	myXtManageChild(2,about_widget);
}

/* quit selected */
void quitMenuBar (Widget w, XtPointer data, XtPointer callData) { 
	doQuit();
}

void reloadFile (Widget w, XtPointer data, XtPointer callData) {
	/* ConsoleMessage ("reloading %s", BrowserFullPath); */
	/* Anchor - this is just a "replace world" call */
	Anchor_ReplaceWorld (BrowserFullPath);
}
void ViewpointFirst (Widget w, XtPointer data, XtPointer callData) {First_ViewPoint();}
void ViewpointLast (Widget w, XtPointer data, XtPointer callData) {Last_ViewPoint();}
void ViewpointNext (Widget w, XtPointer data, XtPointer callData) {Next_ViewPoint();}
void ViewpointPrev (Widget w, XtPointer data, XtPointer callData) {Prev_ViewPoint();}

/* selecting default background colours */

void BackColour(Widget w, XtPointer data, XtPointer callData) {setDefaultBackground(data);};

void Tex128(Widget w, XtPointer data, XtPointer callData) {setTexSize(-128);};
void Tex256(Widget w, XtPointer data, XtPointer callData) {setTexSize(-256);};
void TexFull(Widget w, XtPointer data, XtPointer callData) {setTexSize(0);};
void texturesFirst(Widget w, XtPointer data, XtPointer callData) {
	/* default is set in CFuncs/RenderFuncs to be TRUE; we need to be in sync */
	localtexpri = !localtexpri;
	setTextures_take_priority (localtexpri);
}
void shapeMaker(Widget w, XtPointer data, XtPointer callData) {
	/* default is set in CFuncs/RenderFuncs to be TRUE; we need to be in sync */
	localshapepri = !localshapepri;
	setUseShapeThreadIfPossible (localshapepri);
}


/* do we want a message window displaying fps, viewpoint, etc? */
void toggleMessagebar (Widget w, XtPointer data, XtPointer callData) {
	msgWindowOnscreen = !msgWindowOnscreen; /* keep track of state */
	XmToggleButtonSetState (menumessageButton,msgWindowOnscreen,FALSE); /* display blip if on */
	if (msgWindowOnscreen) myXtManageChild (3,menumessagewindow); /* display (or not) message window */
	else XtUnmanageChild (menumessagewindow);
}

/* do we want a console window displaying errors, etc? */
void toggleConsolebar (Widget w, XtPointer data, XtPointer callData) {
	consWindowOnscreen = !consWindowOnscreen; /* keep track of state */
	XmToggleButtonSetState (consolemessageButton,consWindowOnscreen,FALSE); /* display blip if on */
	if (consWindowOnscreen) myXtManageChild (30,consoleTextWidget); /* display (or not) console window */
	else XtUnmanageChild (consoleTextWidget);
}

void WalkMode (Widget w, XtPointer data, XtPointer callData) {set_viewer_type (WALK); }
void ExamineMode (Widget w, XtPointer data, XtPointer callData) {set_viewer_type (EXAMINE);}
void FlyMode (Widget w, XtPointer data, XtPointer callData) {set_viewer_type (FLY); }
void Headlight (Widget w, XtPointer data, XtPointer callData) {toggle_headlight(); }
void Collision (Widget w, XtPointer data, XtPointer callData) {be_collision = !be_collision; }
void ViewpointStraighten (Widget w, XtPointer data, XtPointer callData) {printf ("not yet implemented\n"); }


/* file selection dialog box, ok button pressed */
void fileSelectPressed (Widget w, XtPointer data, XmFileSelectionBoxCallbackStruct *callData) {
	char *newfile;

	/* get the filename */
	XmStringGetLtoR(callData->value, 
		XmSTRING_DEFAULT_CHARSET, &newfile);

	/* Anchor - this is just a "replace world" call */
	setFullPath (newfile);
	Anchor_ReplaceWorld (BrowserFullPath);
	XtUnmanageChild(w);
}

/* file selection dialog box cancel button - just get rid of widget */
void unManageMe (Widget widget, XtPointer client_data, 
            XmFileSelectionBoxCallbackStruct *selection) {
		XtUnmanageChild(widget);}


/* new file popup - user wants to load a new file */ 
void newFilePopup(Widget cascade_button, char *text, XmPushButtonCallbackStruct *cbs) {   
	myXtManageChild(4,newFileWidget);
	XtPopup(XtParent(newFileWidget), XtGrabNone); 
}
 


#ifdef DOESNOTGETICONICSTATE
/* resize, configure events */
void GLAreaexpose (Widget w, XtPointer data, XtPointer callData) {
	XmDrawingAreaCallbackStruct *cd = (XmDrawingAreaCallbackStruct *) callData;
	switch (cd->reason) {
		case XmCR_EXPOSE: printf ("got expose event \n");
		default: printf ("not known event, %d\n",cd->reason);
	}
}
#endif

/* resize, configure events */
void GLArearesize (Widget w, XtPointer data, XtPointer callData) {
	XmDrawingAreaCallbackStruct *cd = (XmDrawingAreaCallbackStruct *) callData;
	Dimension width, height;

	XtVaGetValues (w, XmNwidth, &width, XmNheight, &height, NULL);
	setScreenDim (width,height);
}

/* Mouse, keyboard input when focus is in OpenGL window. */
void GLAreainput (Widget w, XtPointer data, XtPointer callData) {
	XmDrawingAreaCallbackStruct *cd = (XmDrawingAreaCallbackStruct *) callData;

	#ifdef XTDEBUG
	printEvent(*(cd->event));
	#endif

	handle_Xevents(*(cd->event));
}


/* remove this button from this SelectionBox widget */

void removeWidgetFromSelect (Widget parent, 
	#if NeedWidePrototypes
		unsigned int 
	#else
		unsigned char
	#endif
	button) {

	Widget tmp;

	tmp = XmSelectionBoxGetChild(parent, button);
	if (tmp == NULL) {
		printf ("hmmm - button does not exist\n");
	} else {
		XtUnmanageChild(tmp);
	}
}

/* start up the browser, and point it to www.crc.ca/FreeWRL */
void freewrlHomePopup (Widget w, XtPointer data, XtPointer callData) { 
#define MAXLINE 2000
	char *browser;
	char sysline[MAXLINE];

	browser = getenv("BROWSER");
	if (browser) {
		if (strlen(browser)>(MAXLINE-30)) return;
		strcpy (sysline, browser);
	} else
		strcpy (sysline, BROWSER);
	strcat (sysline, " http://www.crc.ca/FreeWRL &");
	freewrlSystem (sysline);
}


#ifdef XTDEBUG
 /* for debugging... */
 printEvent (XEvent event) {
 
 	switch (event.type) {
 		case KeyPress: printf ("KeyPress"); break;
 		case KeyRelease: printf ("KeyRelease"); break;
 		case ButtonPress: printf ("ButtonPress"); break;
 		case ButtonRelease: printf ("ButtonRelease"); break;
 		case MotionNotify: printf ("MotionNotify"); break;
 		case EnterNotify: printf ("EnterNotify"); break;
 		case LeaveNotify: printf ("LeaveNotify"); break;
 		case FocusIn: printf ("FocusIn"); break;
 		case FocusOut: printf ("FocusOut"); break;
 		case KeymapNotify: printf ("KeymapNotify"); break;
 		case Expose: printf ("Expose"); break;
 		case GraphicsExpose: printf ("GraphicsExpose"); break;
 		case NoExpose: printf ("NoExpose"); break;
 		case VisibilityNotify: printf ("VisibilityNotify"); break;
 		case CreateNotify: printf ("CreateNotify"); break;
 		case DestroyNotify: printf ("DestroyNotify"); break;
 		case UnmapNotify: printf ("UnmapNotify"); break;
 		case MapNotify: printf ("MapNotify"); break;
 		case MapRequest: printf ("MapRequest"); break;
 		case ReparentNotify: printf ("ReparentNotify"); break;
 		case ConfigureNotify: printf ("ConfigureNotify"); break;
 		case ConfigureRequest: printf ("ConfigureRequest"); break;
 		case GravityNotify: printf ("GravityNotify"); break;
 		case ResizeRequest: printf ("ResizeRequest"); break;
 		case CirculateNotify: printf ("CirculateNotify"); break;
 		case CirculateRequest: printf ("CirculateRequest"); break;
 		case PropertyNotify: printf ("PropertyNotify"); break;
 		case SelectionClear: printf ("SelectionClear"); break;
 		case SelectionRequest: printf ("SelectionRequest"); break;
 		case SelectionNotify: printf ("SelectionNotify"); break;
 		case ColormapNotify: printf ("ColormapNotify"); break;
 		case ClientMessage: printf ("ClientMessage"); break;
 		case MappingNotify: printf ("MappingNotify"); break;
 		default :printf ("Event out of range - %d",event.type);
 	}
 
 	printf ("\n");
 }
#endif

/* File pulldown menu */
void createFilePulldown () {
	Widget menupane, btn, cascade;

	XmString mask;
	int ac;
	Arg args[10];
	   
	/* Create the FileSelectionDialog */     
	ac = 0;
	mask  = XmStringCreateLocalized("*.wrl");
	XtSetArg(args[ac], XmNdirMask, mask); ac++;

	/* newFileWidget = XmCreateFileSelectionDialog(menubar, "select", args, 1); */
	newFileWidget = XmCreateFileSelectionDialog(mainw, "select", args, 1);        

	XtAddCallback(newFileWidget, XmNokCallback, (XtCallbackProc)fileSelectPressed, NULL);
	XtAddCallback(newFileWidget, XmNcancelCallback, (XtCallbackProc)unManageMe, NULL);
	/* delete buttons not wanted */
	removeWidgetFromSelect(newFileWidget,XmDIALOG_HELP_BUTTON);
	XtUnmanageChild(newFileWidget);


	menupane = XmCreatePulldownMenu (menubar, "menupane", NULL, 0);
		btn = XmCreatePushButton (menupane, "Reload", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)reloadFile, NULL);
		myXtManageChild (5,btn);
		btn = XmCreatePushButton (menupane, "New...", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)newFilePopup, NULL);
		myXtManageChild (6,btn);

		btn = XmCreatePushButton (menupane, "Quit", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)quitMenuBar, NULL);
		myXtManageChild (7,btn);
	XtSetArg (args[0], XmNsubMenuId, menupane);
	cascade = XmCreateCascadeButton (menubar, "File", args, 1);
	myXtManageChild (8,cascade);
}

/* Navigate pulldown menu */
void createNavigatePulldown() {
	Widget cascade, btn, menupane;

	menupane = XmCreatePulldownMenu (menubar, "menupane", NULL, 0);
	
		/* Viewpoints */
		btn = XmCreatePushButton (menupane, "First Viewpoint", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)ViewpointFirst, NULL);
		myXtManageChild (30,btn);
		btn = XmCreatePushButton (menupane, "Next Viewpoint", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)ViewpointNext, NULL);
		myXtManageChild (9,btn);
		btn = XmCreatePushButton (menupane, "Prev Viewpoint", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)ViewpointPrev, NULL);
		myXtManageChild (10,btn);
		btn = XmCreatePushButton (menupane, "Last Viewpoint", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)ViewpointLast, NULL);
		myXtManageChild (31,btn);


		/* Navigation Mode Selection */
		myXtManageChild(11,XmCreateSeparator (menupane, "sep1", NULL, 0));

		walkButton = XtCreateManagedWidget("Walk Mode", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (walkButton, XmNvalueChangedCallback, (XtCallbackProc)WalkMode, NULL);
		myXtManageChild (12,walkButton);

		examineButton = XtCreateManagedWidget("Examine Mode", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (examineButton, XmNvalueChangedCallback, (XtCallbackProc)ExamineMode, NULL);
		myXtManageChild (13,examineButton);

		flyButton = XtCreateManagedWidget("Fly Mode", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (flyButton, XmNvalueChangedCallback, (XtCallbackProc)FlyMode, NULL);
		myXtManageChild (14,flyButton);

		/* Headlight, Collision */
		myXtManageChild(15,XmCreateSeparator (menupane, "sep1", NULL, 0));

		headlightButton = XtCreateManagedWidget("Headlight",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(headlightButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)Headlight, NULL);
		myXtManageChild (16,headlightButton);

		collisionButton = XtCreateManagedWidget("Collision",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(collisionButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)Collision, NULL);
		myXtManageChild (17,collisionButton);
	
		/* Straighten */
		/* BUTTON NOT WORKING - so make insensitive */
		XtSetArg (buttonArgs[buttonArgc], XmNsensitive, FALSE); 
		myXtManageChild(18,XmCreateSeparator (menupane, "sep1", NULL, 0));
		btn = XmCreatePushButton (menupane, "Straighten", buttonArgs, buttonArgc+1); /* NOTE THE +1 here for sensitive */
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)ViewpointStraighten, NULL);
		myXtManageChild (19,btn);

		consolemessageButton = XtCreateManagedWidget("Console Display",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(consolemessageButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)toggleConsolebar, NULL);
		myXtManageChild (21,consolemessageButton);
		menumessageButton = XtCreateManagedWidget("Message Display",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(menumessageButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)toggleMessagebar, NULL);
		myXtManageChild (20,menumessageButton);
	
	XtSetArg (args[0], XmNsubMenuId, menupane);
	cascade = XmCreateCascadeButton (menubar, "Navigate", args, 1);
	myXtManageChild (22,cascade);
}

/* Preferences pulldown menu */
void createPreferencesPulldown() {
	Widget cascade, menupane;
	int count;

	menupane = XmCreatePulldownMenu (menubar, "menupane", NULL, 0);

		/* texture size on loading */	
		myXtManageChild(11,XmCreateSeparator (menupane, "sep1", NULL, 0));

		tex128_button = XtCreateManagedWidget("128x128 Textures", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (tex128_button, XmNvalueChangedCallback, (XtCallbackProc)Tex128, NULL);
		myXtManageChild (12,tex128_button);

		tex256_button = XtCreateManagedWidget("256x256 Textures", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (tex256_button, XmNvalueChangedCallback, (XtCallbackProc)Tex256, NULL);
		myXtManageChild (13,tex256_button);

		texFull_button = XtCreateManagedWidget("Fullsize Textures", xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback (texFull_button, XmNvalueChangedCallback, (XtCallbackProc)TexFull, NULL);
		myXtManageChild (14,texFull_button);

		/* default Background colour */	
		myXtManageChild(11,XmCreateSeparator (menupane, "sep1", NULL, 0));

		for (count = colourBlack; count <= colourWhite; count++ ){
			backgroundColourSelector[count] = 
				XtCreateManagedWidget(BackString[count], xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
			XtAddCallback (backgroundColourSelector[count], XmNvalueChangedCallback, (XtCallbackProc)BackColour, (XtPointer)count);
			myXtManageChild (40,backgroundColourSelector[count]);
		}
		XmToggleButtonSetState (backgroundColourSelector[colourBlack], TRUE, FALSE);

		/* texture, shape compiling  */
		myXtManageChild(15,XmCreateSeparator (menupane, "sep1", NULL, 0));

		texturesFirstButton = XtCreateManagedWidget("Textures take priority",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(texturesFirstButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)texturesFirst, NULL);
		myXtManageChild (16,texturesFirstButton);
	        XmToggleButtonSetState (texturesFirstButton, localtexpri, FALSE);

		/* texture, shape compiling  */
		myXtManageChild(15,XmCreateSeparator (menupane, "sep1", NULL, 0));

		/* what things can we NOT do if we dont have threads? */
		#ifndef DO_MULTI_OPENGL_THREADS
		XtSetArg (buttonArgs[buttonArgc], XmNsensitive, FALSE);  buttonArgc++;
		#endif
		shapeThreadButton = XtCreateManagedWidget("Shape maker uses thread",
			xmToggleButtonWidgetClass, menupane, buttonArgs, buttonArgc);
		XtAddCallback(shapeThreadButton, XmNvalueChangedCallback, 
			  (XtCallbackProc)shapeMaker, NULL);
		#ifndef DO_MULTI_OPENGL_THREADS
		buttonArgc--;
		#endif
		myXtManageChild (17,shapeThreadButton);

		#ifdef DO_MULTI_OPENGL_THREADS
	        XmToggleButtonSetState (shapeThreadButton, localshapepri, FALSE);
		#endif
	
	XtSetArg (args[0], XmNsubMenuId, menupane);
	cascade = XmCreateCascadeButton (menubar, "Preferences", args, 1);
	myXtManageChild (22,cascade);
}

void createHelpPulldown() {
	Widget btn, menupane, cascade;
	int ac;
	Arg args[10];


	menupane = XmCreatePulldownMenu (menubar, "menupane", NULL, 0);

		/* Helpity stuff */
		ac = 0;
		/*
		sprintf (ns,ABOUT_FREEWRL,getLibVersion(),"","");
		diastring = xec_NewString(ns);

		XtSetArg(args[ac], XmNmessageString, diastring); ac++;
		*/
		XtSetArg(args[ac], XmNmessageAlignment,XmALIGNMENT_CENTER); ac++;
		about_widget = XmCreateInformationDialog(menubar, "about", args, ac);        
		XtAddCallback(about_widget, XmNokCallback, (XtCallbackProc)unManageMe, NULL);
		removeWidgetFromSelect (about_widget, XmDIALOG_CANCEL_BUTTON);
		/*
		 causes segfault on Core3 removeWidgetFromSelect (about_widget, XmDIALOG_HELP_BUTTON);
		*/


		btn = XmCreatePushButton (menupane, "About FreeWRL...", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)aboutFreeWRLpopUp, NULL);
		myXtManageChild (23,btn);
		btn = XmCreatePushButton (menupane, "FreeWRL Homepage...", NULL, 0);
		XtAddCallback (btn, XmNactivateCallback, (XtCallbackProc)freewrlHomePopup, NULL);
		myXtManageChild (24,btn);

	XtSetArg (args[0], XmNsubMenuId, menupane);
	cascade = XmCreateCascadeButton (menubar, "Help", args, 1);
	myXtManageChild (25,cascade);
}

/**********************************/
void createMenuBar(void) {
	Arg menuArgs[10]; int menuArgc = 0;

	/* create the menu bar */
	menuArgc = 0;
	
	/* the following XtSetArg is not required; it only "pretties" up the display
	   in some circumstances. It came out in Motif 2.0, and is not always found */
	#ifdef XmNscrolledWindowChildType
	XtSetArg(menuArgs[menuArgc], XmNscrolledWindowChildType, XmMENU_BAR); menuArgc++;
	#endif

	menubar = XmCreateMenuBar (mainw, "menubar", menuArgs, menuArgc);
	myXtManageChild (26,menubar);

	menumessagewindow = 
		XtVaCreateWidget ("Message:", xmTextFieldWidgetClass, mainw,
		XmNeditable, False,
		XmNmaxLength, 200,
		NULL);

	/* generic toggle button resources */
	XtSetArg(buttonArgs[buttonArgc], XmCVisibleWhenOff, TRUE); buttonArgc++;
	XtSetArg(buttonArgs[buttonArgc],XmNindicatorType,XmN_OF_MANY); buttonArgc++;

	if (!RUNNINGASPLUGIN) createFilePulldown();
	createNavigatePulldown();
	createPreferencesPulldown();
	createHelpPulldown();

}

/**********************************************************************************/
/*
	create a frame for FreeWRL, and for messages
*/
void createDrawingFrame(void) {
	/* frame holds everything here */
        frame = XtVaCreateManagedWidget("form", xmPanedWindowWidgetClass, mainw, NULL);
	consoleTextWidget = XtVaCreateManagedWidget ("console text widget", xmScrolledWindowWidgetClass, frame,
	XmNtopAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNworkWindow, consoleTextArea,
	NULL);
	consoleTextArea = XtVaCreateManagedWidget ("console text area ", xmTextWidgetClass, consoleTextWidget,
	XmNrows, 5,
	XmNcolumns, 0,
	XmNeditable, False,
	XmNeditMode, XmMULTI_LINE_EDIT,
	NULL);

	/* create the FreeWRL OpenGL drawing area, and map it. */
	freewrlDrawArea = XtVaCreateManagedWidget ("freewrlDrawArea", glwDrawingAreaWidgetClass,
			frame, "visualInfo", Xvi, 
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
			NULL);

	#ifdef DOESNOTGETICONICSTATE
	XtAddCallback (freewrlDrawArea, XmNexposeCallback, GLAreaexpose, NULL);
	#endif

	XtAddCallback (freewrlDrawArea, XmNresizeCallback, GLArearesize, NULL);
	XtAddCallback (freewrlDrawArea, XmNinputCallback, GLAreainput, NULL);

	myXtManageChild(27,freewrlDrawArea);

	/* let the user ask for this one */
	XtUnmanageChild(consoleTextWidget);

}

void openMotifMainWindow (int argc, char **argv) {
	String dummyargc[] = { " ", " "};
	Arg initArgs[10]; int initArgc = 0;
	argc = 0;

	#ifdef DO_MULTI_OPENGL_THREADS
	XInitThreads();
	#endif

	XtSetArg(initArgs[initArgc],XmNlabelString,XmStringCreate("FreeWRL VRML X3D Browser",XmSTRING_DEFAULT_CHARSET)); initArgc++;
	XtSetArg(initArgs[initArgc],XmNheight, feHeight); initArgc++;
	XtSetArg(initArgs[initArgc],XmNwidth, feWidth); initArgc++;
	freewrlTopWidget = XtAppInitialize (&freewrlXtAppContext, "FreeWRL", NULL, 0, 
		&argc, dummyargc, NULL, initArgs, initArgc);

	Xdpy = XtDisplay (freewrlTopWidget);

	/* if we are running as a plugin, let the plugin control code map and manage this window. */
	if (RUNNINGASPLUGIN) {
		XtSetMappedWhenManaged (freewrlTopWidget,False);
	}
}


void setConsoleMessage (char *str) {
	char *tptr;
	int nl;

	/* is the consoleTextWidget created yet?? */
	if (ISDISPLAYINITIALIZED != TRUE) {
		printf ("ConsoleMessage: %s\n",str);
	} else {
		/* make sure console window is on screen */
		if (!consWindowOnscreen) {
			consWindowOnscreen = TRUE;
			myXtManageChild (1,consoleTextWidget); /* display console window */
			XmToggleButtonSetState (consolemessageButton,consWindowOnscreen,FALSE); /* display blip if on */
		}
		
		/* put the text here */
		#ifdef DO_MULTI_OPENGL_THREADS
			XmTextInsert (consoleTextArea, strlen(XmTextGetString(consoleTextArea)),str);
		#else
			nl = strlen(str);
			tptr = MALLOC (nl+10);
			strcpy (tptr,str);
			
			/* copy old string, if it exists */
			FREE_IF_NZ (consMsg);
			consMsg = tptr;
			consmsgChanged = TRUE;
		#endif
	}
}



void frontendUpdateButtons() {
	if (colbutChanged) {
		XmToggleButtonSetState (collisionButton,colbut,FALSE);
		colbutChanged = FALSE;
	}
	if (headbutChanged) {
		XmToggleButtonSetState (collisionButton,headbut,FALSE);
		headbutChanged = FALSE;
	}
	if (navbutChanged) {
			XmToggleButtonSetState (walkButton,wa,FALSE);
			XmToggleButtonSetState (flyButton,fl,FALSE);
			XmToggleButtonSetState (examineButton,ex,FALSE);
			navbutChanged = FALSE;
	}
	if (msgChanged) {
			XmTextSetString(menumessagewindow,fpsstr);
			msgChanged = FALSE;
	}
	if (consmsgChanged) {
			/* printf ("frontendUpateButtons, consmggchanged, posn %d oldstr %s consmsg %s\n",
				strlen(XmTextGetString(consoleTextArea)),
				XmTextGetString(consoleTextArea),
				consMsg);*/
			XmTextInsert (consoleTextArea, strlen(XmTextGetString(consoleTextArea)),consMsg);
			consmsgChanged = FALSE;
	}
}

void setMenuButton_collision (int val) {
		#ifdef DO_MULTI_OPENGL_THREADS
			XmToggleButtonSetState (collisionButton,val,FALSE);
		#else
			colbut = val;
			colbutChanged = TRUE;
		
		#endif
}
void setMenuButton_headlight (int val) {
		#ifdef DO_MULTI_OPENGL_THREADS
			XmToggleButtonSetState (headlightButton,val,FALSE);
		#else
			headbut = val;
			headbutChanged = TRUE;
		
		#endif
}
void setMenuButton_navModes (int type) {
		fl = FALSE; ex = FALSE; wa = FALSE;
		switch(type) {
			case NONE: break;
			case EXAMINE: ex = TRUE; break;
			case WALK: wa = TRUE; break;
			case FLY: fl = TRUE; break;
			default: break;
		}
		#ifdef DO_MULTI_OPENGL_THREADS
			XmToggleButtonSetState (walkButton,wa,FALSE);
			XmToggleButtonSetState (flyButton,fl,FALSE);
			XmToggleButtonSetState (examineButton,ex,FALSE);
		#else
			navbutChanged = TRUE;
		#endif
}

void setMenuButton_texSize (int size) {
	int val;
	/* this is called from the texture thread, so there is not a threading problem here */
	val = FALSE;
	/* set all thread buttons to FALSE */
	XmToggleButtonSetState (tex128_button, val, FALSE);
	XmToggleButtonSetState (tex256_button, val, FALSE);
	XmToggleButtonSetState (texFull_button, val, FALSE);
	val = TRUE;
	if (size <= 128) {XmToggleButtonSetState (tex128_button, val, FALSE);
	} else if (size <=256) {XmToggleButtonSetState (tex256_button, val, FALSE);
	} else {XmToggleButtonSetState (texFull_button, val, FALSE); }
}

void setMessageBar() {
	
	if (menumessagewindow != NULL) {
		/* make up new line to display */
		if (strlen(myMenuStatus) == 0) {
			strcat (myMenuStatus, "NONE");
		}
		if (isShapeCompilerParsing() || isinputThreadParsing() || isTextureParsing() || (!isInputThreadInitialized())) {
			sprintf (fpsstr, "(Loading...)  speed: %4.1f", myFps);
		} else {
			sprintf (fpsstr,"fps: %4.1f Viewpoint: %s",myFps,myMenuStatus);
		}
		#ifdef DO_MULTI_OPENGL_THREADS
			XmTextSetString(menumessagewindow,fpsstr);
		#else
			msgChanged = TRUE;
		#endif
	}

}

void  getMotifWindowedGLwin(Window *win) {
        *win = XtWindow(freewrlDrawArea);
}


int isMotifDisplayInitialized (void) {
        	return  (MainWidgetRealized);
}               

void setDefaultBackground(int colour) {
	int count;


	if ((colour<colourBlack) || (colour > colourWhite)) return; /* an error... */

	for (count = colourBlack; count <= colourWhite; count++) {
		XmToggleButtonSetState (backgroundColourSelector[count], FALSE, FALSE);
	}
	XmToggleButtonSetState (backgroundColourSelector[colour], TRUE, FALSE);
	setglClearColor (&(backgroundColours[colour*3]));

 }
 
#endif
