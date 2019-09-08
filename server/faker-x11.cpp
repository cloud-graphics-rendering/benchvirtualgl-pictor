/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011-2016 D. R. Commander
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3.1 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#include "DisplayHash.h"
#include "PixmapHash.h"
#include "VisualHash.h"
#include "WindowHash.h"
#include "faker.h"
#include "vglconfigLauncher.h"
#ifdef FAKEXCB
#include "XCBConnHash.h"
#endif
#include "keycodetokeysym.h"
#include "Xlibint.h"
#include "utlist.h"

#include "timetrack.h"
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2proto.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdint.h>

#ifndef STOP_BENCH
timeTrack* timeTracker = NULL;
int timeTrackerAttached = 0;
int current_event_index = 1;
int read_clear = 0;
int keypointer_eventID = 0;
extern long long gettime_nanoTime();
#endif

//struct fd_pair *headerfd;
double lastMouseXPos = 649;
double lastMouseYPos = 481;
int InGameThreashold = 0;
int InGameStatus = 0;
extern FILE *globalLog;

#ifdef WIN32
#define ECHECK(err) (WSAGetLastError() == err)
#define ESET(val) WSASetLastError(val)
#else
#ifdef __UNIXOS2__
#define ECHECK(err) (errno == err)
#define ESET(val)
#else
#define ECHECK(err) (errno == err)
#define ESET(val) errno = val
#endif
#endif

using namespace vglserver;

// Interposed X11 functions


extern FILE* getLogFilePointer(pid_t cur_pid);
/*
FILE* getLogFilePointer(pid_t cur_pid){
     struct fd_pair *tmpfd = headerfd;
     struct fd_pair *lstfd = NULL;
     int try_find = 0;

     char str1[10];
     char logpath[80] ={'/','t','m','p','/','v','g','l','/'};

     while(tmpfd!=NULL){
         if(tmpfd->pid != cur_pid){
             lstfd = tmpfd;
             tmpfd = tmpfd->next;
             try_find = 1;
         }else{
             return tmpfd->fd;
         }
     }
     if(tmpfd == NULL){
          tmpfd = (struct fd_pair*)malloc(sizeof(struct fd_pair));
          tmpfd->pid = cur_pid;
          sprintf(str1, "%d", cur_pid);
          strcat(logpath, str1);
          tmpfd->fd = fopen(logpath, "ab+");
          //fprintf(globalLog, "logpath:%s, fd: %p\n", logpath, tmpfd->fd);
          tmpfd->status = 1;
          tmpfd->next = NULL;
          if(try_find == 0){
             headerfd = tmpfd;
          }else{
             lstfd->next = tmpfd;
          }
          return (tmpfd->fd!=NULL)?tmpfd->fd:NULL;
     }
}
*/

extern "C" {



// XCloseDisplay() implicitly closes all windows and subwindows that were
// attached to the display handle, so we have to make sure that the
// corresponding VirtualWin instances are shut down.

int XCloseDisplay(Display *dpy)
{
	// MainWin calls various X11 functions from the destructor of one of its
	// shared libraries, which is executed after the VGL faker has shut down,
	// so we cannot access fconfig or vglout or winh without causing deadlocks or
	// other issues.  At this point, all we can safely do is hand off to libX11.
	if(vglfaker::deadYet || vglfaker::getFakerLevel() > 0)
		return _XCloseDisplay(dpy);

	int retval = 0;
	TRY();

		opentrace(XCloseDisplay);  prargd(dpy);  starttrace();

	#ifdef FAKEXCB
	if(fconfig.fakeXCB)
	{
		CHECKSYM_NONFATAL(XGetXCBConnection)
		if(!__XGetXCBConnection)
		{
			if(fconfig.verbose)
				vglout.print("[VGL] Disabling XCB interposer\n");
			fconfig.fakeXCB = 0;
		}
		else
		{
			xcb_connection_t *conn = _XGetXCBConnection(dpy);
			xcbconnhash.remove(conn);
		}
	}
	#endif

	winhash.remove(dpy);
	dpyhash.remove(dpy);
	retval = _XCloseDisplay(dpy);

		stoptrace();  closetrace();

	CATCH();
	return retval;
}


// We have to override this function in order to handle GLX pixmap rendering

int XCopyArea(Display *dpy, Drawable src, Drawable dst, GC gc, int src_x,
	int src_y, unsigned int width, unsigned int height, int dest_x, int dest_y)
{
	TRY();

	if(isExcluded(dpy))
		return _XCopyArea(dpy, src, dst, gc, src_x, src_y, width, height, dest_x,
			dest_y);

	VirtualDrawable *srcVW = NULL;  VirtualDrawable *dstVW = NULL;
	bool srcWin = false, dstWin = false;
	bool copy2d = true, copy3d = false, triggerRB = false;
	GLXDrawable glxsrc = 0, glxdst = 0;

	if(src == 0 || dst == 0) return BadDrawable;

		opentrace(XCopyArea);  prargd(dpy);  prargx(src);  prargx(dst);
		prargx(gc);  prargi(src_x);  prargi(src_y);  prargi(width);
		prargi(height);  prargi(dest_x);  prargi(dest_y);  starttrace();

	if(!(srcVW = (VirtualDrawable *)pmhash.find(dpy, src)))
	{
		srcVW = (VirtualDrawable *)winhash.find(dpy, src);
		if(srcVW) srcWin = true;
	}
	if(srcVW && !srcVW->isInit())
	{
		// If the 3D drawable hasn't been made current yet, then its contents will
		// be identical to the corresponding 2D drawable
		srcVW = NULL;
		srcWin = false;
	}
	if(!(dstVW = (VirtualDrawable *)pmhash.find(dpy, dst)))
	{
		dstVW = (VirtualDrawable *)winhash.find(dpy, dst);
		if(dstVW) dstWin = true;
	}
	if(dstVW && !dstVW->isInit())
	{
		dstVW = NULL;
		dstWin = false;
	}

	// GLX (3D) Pixmap --> non-GLX (2D) drawable
	// Sync pixels from the 3D pixmap (on the 3D X Server) to the corresponding
	// 2D pixmap (on the 2D X Server) and let the "real" XCopyArea() do the rest.
	if(srcVW && !srcWin && !dstVW) ((VirtualPixmap *)srcVW)->readback();

	// non-GLX (2D) drawable --> non-GLX (2D) drawable
	// Source and destination are not backed by a drawable on the 3D X Server, so
	// defer to the real XCopyArea() function.
	//
	// non-GLX (2D) drawable --> GLX (3D) drawable
	// We don't really handle this yet (and won't until we have to.)  Copy to the
	// 2D destination drawable only, without updating the corresponding 3D
	// drawable.
	//
	// GLX (3D) Window --> non-GLX (2D) drawable
	// We assume that glFinish() or another synchronization function has been
	// called prior to XCopyArea(), in order to copy the pixels from the
	// off-screen drawable on the 3D X Server to the corresponding window on the
	// 2D X Server.  Thus, we defer to the real XCopyArea() function (but this
	// may not work properly without VGL_SYNC=1.)
	{}

	// GLX (3D) Window --> GLX (3D) drawable
	// GLX (3D) Pixmap --> GLX (3D) Pixmap
	// Sync both 2D and 3D pixels.
	if(srcVW && srcWin && dstVW) copy3d = true;
	if(srcVW && !srcWin && dstVW && !dstWin) copy3d = true;

	// GLX (3D) Pixmap --> GLX (3D) Window
	// Copy 3D pixels to the window's corresponding off-screen drawable, then
	// trigger a VirtualGL readback to deliver the pixels from the off-screen
	// drawable to the window.
	if(srcVW && !srcWin && dstVW && dstWin)
	{
		copy2d = false;  copy3d = true;  triggerRB = true;
	}

	if(copy2d)
		_XCopyArea(dpy, src, dst, gc, src_x, src_y, width, height, dest_x, dest_y);

	if(copy3d)
	{
		glxsrc = srcVW->getGLXDrawable();
		glxdst = dstVW->getGLXDrawable();
		srcVW->copyPixels(src_x, src_y, width, height, dest_x, dest_y, glxdst);
		if(triggerRB)
			((VirtualWin *)dstVW)->readback(GL_FRONT, false, fconfig.sync);
	}

		stoptrace();  if(copy3d) prargx(glxsrc);  if(copy3d) prargx(glxdst);
		closetrace();

	CATCH();
	return 0;
}

// This is provided for the benefit of the FBX library, which needs to call the
// "real" XCopyArea() function from within the faker in order to implement the
// X11 Transport.
int XCopyArea_FBX(Display *dpy, Drawable src, Drawable dst, GC gc, int src_x,
	int src_y, unsigned int width, unsigned int height, int dest_x, int dest_y)
{
	return _XCopyArea(dpy, src, dst, gc, src_x, src_y, width, height, dest_x,
		dest_y);
}


// When a window is created, add it to the hash.  A VirtualWin instance does
// not get created and hashed to the window until/unless the window is made
// current in OpenGL.

Window XCreateSimpleWindow(Display *dpy, Window parent, int x, int y,
	unsigned int width, unsigned int height, unsigned int border_width,
	unsigned long border, unsigned long background)
{
	Window win = 0;
	TRY();

	if(isExcluded(dpy))
		return _XCreateSimpleWindow(dpy, parent, x, y, width, height, border_width,
			border, background);

		opentrace(XCreateSimpleWindow);  prargd(dpy);  prargx(parent);  prargi(x);
		prargi(y);  prargi(width);  prargi(height);  starttrace();

	win = _XCreateSimpleWindow(dpy, parent, x, y, width, height, border_width,
		border, background);
	if(win) winhash.add(dpy, win);

		stoptrace();  prargx(win);  closetrace();

	CATCH();
	return win;
}


Window XCreateWindow(Display *dpy, Window parent, int x, int y,
	unsigned int width, unsigned int height, unsigned int border_width,
	int depth, unsigned int c_class, Visual *visual, unsigned long valuemask,
	XSetWindowAttributes *attributes)
{
	Window win = 0;
	TRY();

	if(isExcluded(dpy))
		return _XCreateWindow(dpy, parent, x, y, width, height, border_width,
			depth, c_class, visual, valuemask, attributes);

		opentrace(XCreateWindow);  prargd(dpy);  prargx(parent);  prargi(x);
		prargi(y);  prargi(width);  prargi(height);  prargi(depth);
		prargi(c_class);  prargv(visual);  starttrace();

	win = _XCreateWindow(dpy, parent, x, y, width, height, border_width, depth,
		c_class, visual, valuemask, attributes);
	if(win) winhash.add(dpy, win);

		stoptrace();  prargx(win);  closetrace();

	CATCH();
	return win;
}


// When a window is destroyed, we shut down the corresponding VirtualWin
// instance, but we also have to walk the window tree to ensure that VirtualWin
// instances attached to subwindows are also shut down.

static void DeleteWindow(Display *dpy, Window win, bool subOnly = false)
{
	Window root, parent, *children = NULL;  unsigned int n = 0;

	if(!subOnly) winhash.remove(dpy, win);
	if(XQueryTree(dpy, win, &root, &parent, &children, &n)
		&& children && n > 0)
	{
		for(unsigned int i = 0; i < n; i++) DeleteWindow(dpy, children[i]);
	}
}


int XDestroySubwindows(Display *dpy, Window win)
{
	int retval = 0;
	TRY();

	if(isExcluded(dpy))
		return _XDestroySubwindows(dpy, win);

		opentrace(XDestroySubwindows);  prargd(dpy);  prargx(win);  starttrace();

	if(dpy && win) DeleteWindow(dpy, win, true);
	retval = _XDestroySubwindows(dpy, win);

		stoptrace();  closetrace();

	CATCH();
	return retval;
}


int XDestroyWindow(Display *dpy, Window win)
{
	int retval = 0;
	TRY();

	if(isExcluded(dpy))
		return _XDestroyWindow(dpy, win);

		opentrace(XDestroyWindow);  prargd(dpy);  prargx(win);  starttrace();

	if(dpy && win) DeleteWindow(dpy, win);
	retval = _XDestroyWindow(dpy, win);

		stoptrace();  closetrace();

	CATCH();
	return retval;
}


// If we're freeing a visual that is hashed to an FB config, then remove the
// corresponding hash entry.

int XFree(void *data)
{
	int ret = 0;
	TRY();
	ret = _XFree(data);
	if(data && !vglfaker::deadYet) vishash.remove(NULL, (XVisualInfo *)data);
	CATCH();
	return ret;
}


// Chromium is mainly to blame for this one.  Since it is using separate
// processes to do 3D and X11 rendering, the 3D process will call
// XGetGeometry() repeatedly to obtain the window size, and since the 3D
// process has no X event loop, monitoring this function is the only way for
// VirtualGL to know that the window size has changed.

Status XGetGeometry(Display *dpy, Drawable drawable, Window *root, int *x,
	int *y, unsigned int *width_return, unsigned int *height_return,
	unsigned int *border_width, unsigned int *depth)
{
	Status ret = 0;
	unsigned int width = 0, height = 0;
	TRY();

	if(isExcluded(dpy))
		return _XGetGeometry(dpy, drawable, root, x, y, width_return,
			height_return, border_width, depth);

		opentrace(XGetGeometry);  prargd(dpy);  prargx(drawable);  starttrace();

	VirtualWin *vw = NULL;
	if(winhash.find(drawable, vw))
	{
		// Apparently drawable is a GLX drawable ID that backs a window, so we need
		// to request the geometry of the window, not the GLX drawable.  This
		// prevents a BadDrawable error in Steam.
		dpy = vw->getX11Display();
		drawable = vw->getX11Drawable();
	}
	ret = _XGetGeometry(dpy, drawable, root, x, y, &width, &height, border_width,
		depth);
	if(winhash.find(dpy, drawable, vw) && width > 0 && height > 0)
		vw->resize(width, height);

		stoptrace();  if(root) prargx(*root);  if(x) prargi(*x);  if(y) prargi(*y);
		prargi(width);  prargi(height);  if(border_width) prargi(*border_width);
		if(depth) prargi(*depth);  closetrace();

	if(width_return) *width_return = width;
	if(height_return) *height_return = height;

	CATCH();
	return ret;
}


// If the pixmap has been used for 3D rendering, then we have to synchronize
// the contents of the 3D pixmap, which resides on the 3D X server, with the
// 2D pixmap on the 2D X server before calling the "real" XGetImage() function.

XImage *XGetImage(Display *dpy, Drawable drawable, int x, int y,
	unsigned int width, unsigned int height, unsigned long plane_mask,
	int format)
{
	XImage *xi = NULL;
	TRY();

	if(isExcluded(dpy))
		return _XGetImage(dpy, drawable, x, y, width, height, plane_mask, format);

		opentrace(XGetImage);  prargd(dpy);  prargx(drawable);  prargi(x);
		prargi(y);  prargi(width);  prargi(height);  prargx(plane_mask);
		prargi(format);  starttrace();

	VirtualPixmap *vpm = pmhash.find(dpy, drawable);
	if(vpm) vpm->readback();

	xi = _XGetImage(dpy, drawable, x, y, width, height, plane_mask, format);

		stoptrace();  closetrace();

	CATCH();
	return xi;
}


// Tell the application that the GLX extension is present, even if it isn't

char **XListExtensions(Display *dpy, int *next)
{
	char **list = NULL, *listStr = NULL;  int n, i;
	int hasGLX = 0, listLen = 0;

	TRY();

	if(isExcluded(dpy))
		return _XListExtensions(dpy, next);

		opentrace(XListExtensions);  prargd(dpy);  starttrace();

	list = _XListExtensions(dpy, &n);
	if(list && n > 0)
	{
		for(i = 0; i < n; i++)
		{
			if(list[i])
			{
				listLen += strlen(list[i]) + 1;
				if(!strcmp(list[i], "GLX")) hasGLX = 1;
			}
		}
	}
	if(!hasGLX)
	{
		char **newList = NULL;  int index = 0;
		listLen += 4;  // "GLX" + terminating NULL
		_errifnot(newList = (char **)malloc(sizeof(char *) * (n + 1)))
		_errifnot(listStr = (char *)malloc(listLen + 1))
		memset(listStr, 0, listLen + 1);
		listStr = &listStr[1];  // For compatibility with X.org implementation
		if(list && n > 0)
		{
			for(i = 0; i < n; i++)
			{
				newList[i] = &listStr[index];
				if(list[i])
				{
					strncpy(newList[i], list[i], strlen(list[i]));
					index += strlen(list[i]);
					listStr[index] = '\0';  index++;
				}
			}
			XFreeExtensionList(list);
		}
		newList[n] = &listStr[index];
		strncpy(newList[n], "GLX", 3);  newList[n][3] = '\0';
		list = newList;  n++;
	}

		stoptrace();  prargi(n);  closetrace();

	CATCH();

	if(next) *next = n;
	return list;
}


// This is normally where VirtualGL initializes, unless a GLX function is
// called first.

Display *XOpenDisplay(_Xconst char *name)
{
	Display *dpy = NULL;

	TRY();

	if(vglfaker::deadYet || vglfaker::getFakerLevel() > 0)
		return _XOpenDisplay(name);

		opentrace(XOpenDisplay);  prargs(name);  starttrace();

	vglfaker::init();
	dpy = _XOpenDisplay(name);
	if(dpy)
	{
		if(vglfaker::excludeDisplay(DisplayString(dpy)))
			dpyhash.add(dpy);
		else if(strlen(fconfig.vendor) > 0)
			ServerVendor(dpy) = strdup(fconfig.vendor);
	}

		stoptrace();  prargd(dpy);  closetrace();

	CATCH();
	return dpy;
}


// Tell the application that the GLX extension is present, even if it isn't.

Bool XQueryExtension(Display *dpy, _Xconst char *name, int *major_opcode,
	int *first_event, int *first_error)
{
	Bool retval = True;

	if(isExcluded(dpy))
		return _XQueryExtension(dpy, name, major_opcode, first_event, first_error);

		opentrace(XQueryExtension);  prargd(dpy);  prargs(name);  starttrace();

	retval = _XQueryExtension(dpy, name, major_opcode, first_event, first_error);
	if(!strcmp(name, "GLX")) retval = True;

		stoptrace();  if(major_opcode) prargi(*major_opcode);
		if(first_event) prargi(*first_event);
		if(first_error) prargi(*first_error);  closetrace();

	return retval;
}


// This was implemented because of Pro/E Wildfire v2 on SPARC.  Unless the X
// server vendor string was "Sun Microsystems, Inc.", it would assume a remote
// connection and disable OpenGL rendering.

char *XServerVendor(Display *dpy)
{
	TRY();

	if(isExcluded(dpy) || !strlen(fconfig.vendor))
		return _XServerVendor(dpy);
	return fconfig.vendor;

	CATCH();

	return NULL;
}


// The following functions are interposed so that VirtualGL can detect window
// resizes, key presses (to pop up the VGL configuration dialog), and window
// delete events from the window manager.

static void handleEvent(Display *dpy, XEvent *xe)
{
	VirtualWin *vw = NULL;

	if(isExcluded(dpy))
		return;

	if(xe && xe->type == ConfigureNotify)
	{
		if(winhash.find(dpy, xe->xconfigure.window, vw))
		{
				opentrace(handleEvent);  prargi(xe->xconfigure.width);
				prargi(xe->xconfigure.height);  prargx(xe->xconfigure.window);
				starttrace();

			vw->resize(xe->xconfigure.width, xe->xconfigure.height);

				stoptrace();  closetrace();
		}
	}
	else if(xe && xe->type == KeyPress)
	{
		unsigned int state2, state = (xe->xkey.state) & (~(LockMask));
		state2 = fconfig.guimod;
		if(state2 & Mod1Mask)
		{
			state2 &= (~(Mod1Mask));  state2 |= Mod2Mask;
		}
		if(fconfig.gui
			&& KeycodeToKeysym(dpy, xe->xkey.keycode, 0) == fconfig.guikey
			&& (state == fconfig.guimod || state == state2)
			&& fconfig_getshmid() != -1)
			vglpopup(dpy, fconfig_getshmid());
	}
	else if(xe && xe->type == ClientMessage)
	{
		XClientMessageEvent *cme = (XClientMessageEvent *)xe;
		Atom protoAtom = XInternAtom(dpy, "WM_PROTOCOLS", True);
		Atom deleteAtom = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
		if(protoAtom && deleteAtom && cme->message_type == protoAtom
			&& cme->data.l[0] == (long)deleteAtom
			&& winhash.find(dpy, cme->window, vw))
			vw->wmDelete();
	}
}


Bool XCheckMaskEvent(Display *dpy, long event_mask, XEvent *xe)
{
	Bool retval = 0;
	TRY();

	if((retval = _XCheckMaskEvent(dpy, event_mask, xe)) == True)
		handleEvent(dpy, xe);

	CATCH();
	return retval;
}


Bool XCheckTypedEvent(Display *dpy, int event_type, XEvent *xe)
{
	Bool retval = 0;
	TRY();

	if((retval = _XCheckTypedEvent(dpy, event_type, xe)) == True)
		handleEvent(dpy, xe);

	CATCH();
	return retval;
}


Bool XCheckTypedWindowEvent(Display *dpy, Window win, int event_type,
	XEvent *xe)
{
	Bool retval = 0;
	TRY();

	if((retval = _XCheckTypedWindowEvent(dpy, win, event_type, xe)) == True)
		handleEvent(dpy, xe);

	CATCH();
	return retval;
}


Bool XCheckWindowEvent(Display *dpy, Window win, long event_mask, XEvent *xe)
{
	Bool retval = 0;
	TRY();

	if((retval = _XCheckWindowEvent(dpy, win, event_mask, xe)) == True)
		handleEvent(dpy, xe);

	CATCH();
	return retval;
}


int XConfigureWindow(Display *dpy, Window win, unsigned int value_mask,
	XWindowChanges *values)
{
	int retval = 0;
	TRY();

	if(isExcluded(dpy))
		return _XConfigureWindow(dpy, win, value_mask, values);

		opentrace(XConfigureWindow);  prargd(dpy);  prargx(win);
		if(values && (value_mask & CWWidth)) { prargi(values->width); }
		if(values && (value_mask & CWHeight)) { prargi(values->height); }
		starttrace();

	VirtualWin *vw = NULL;
	if(winhash.find(dpy, win, vw) && values)
		vw->resize(value_mask & CWWidth ? values->width : 0,
			value_mask & CWHeight ? values->height : 0);
	retval = _XConfigureWindow(dpy, win, value_mask, values);

		stoptrace();  closetrace();

	CATCH();
	return retval;
}


int XMaskEvent(Display *dpy, long event_mask, XEvent *xe)
{
	int retval = 0;
	TRY();

	retval = _XMaskEvent(dpy, event_mask, xe);
	handleEvent(dpy, xe);

	CATCH();
	return retval;
}


int XMoveResizeWindow(Display *dpy, Window win, int x, int y,
	unsigned int width, unsigned int height)
{
	int retval = 0;
	TRY();

	if(isExcluded(dpy))
		return _XMoveResizeWindow(dpy, win, x, y, width, height);

		opentrace(XMoveResizeWindow);  prargd(dpy);  prargx(win);  prargi(x);
		prargi(y);  prargi(width);  prargi(height);  starttrace();

	VirtualWin *vw = NULL;
	if(winhash.find(dpy, win, vw)) vw->resize(width, height);
	retval = _XMoveResizeWindow(dpy, win, x, y, width, height);

		stoptrace();  closetrace();

	CATCH();
	return retval;
}


int XNextEvent(Display *dpy, XEvent *xe)
{
	int retval = 0;
	int i = 1;
	XEvent *event;
	double vals2[2] = {-1, -1};
	//TRY();
        /*
        pid_t cur_pid = getpid();
        pid_t cur_tid = syscall(SYS_gettid);
        FILE* tmpFp = getLogFilePointer(cur_pid);
        if(tmpFp == NULL){
           fprintf(globalLog, "tmpFp in Xnextevent is NULL\n");
        }*/

	register _XQEvent *qelt;
        LockDisplay(dpy);
        if (dpy->head != NULL){
          qelt = dpy->head;
          event = &(qelt->event);
	  XGenericEventCookie *cookie = &event->xcookie;
	  if(cookie != NULL && cookie->evtype == XI_RawMotion){ 
		const XIRawEvent *rawev = (const XIRawEvent*)cookie->data;
		if(rawev != NULL){
		    double *vals = rawev->raw_values;
		    double temp1 = vals[0]; vals[0] -= lastMouseXPos; lastMouseXPos = temp1;
		    double temp2 = vals[1]; vals[1] -= lastMouseYPos; lastMouseYPos = temp2;
		}
	  }
	}
        UnlockDisplay(dpy);
	retval = _XNextEvent(dpy, xe);
	//fprintf(tmpFp,"PID: %d, TID: %d, 111111 event type: %d, xmotion.x: %d, xmotion.y: %d\n", cur_pid, cur_tid, xe->type, xe->xmotion.x, xe->xmotion.y);
        //if(xe->type == 6 && xe->xmotion.x == 640 && xe->xmotion.y == 480){
        if(xe->type == 6 && xe->xmotion.x == 960 && xe->xmotion.y == 540){
            if(InGameStatus == 0){
                   InGameThreashold++;
                if(InGameThreashold > 10){
                   InGameStatus = 1;
                   InGameThreashold = 10;
                }
            }else{
                   InGameThreashold = 10;
            }
        }else if(xe->type == 6 && InGameStatus == 1){
            InGameThreashold--;
            if(InGameThreashold < 0){
                InGameThreashold = 0;
                InGameStatus = 0;
            }
            int tmp1 = xe->xmotion.x;
            int tmp2 = xe->xmotion.y;
            //xe->xmotion.x = xe->xmotion.x - lastMouseXPos + 640;
            //xe->xmotion.y = xe->xmotion.y - lastMouseYPos + 480;
            xe->xmotion.x = xe->xmotion.x - lastMouseXPos + 960;
            xe->xmotion.y = xe->xmotion.y - lastMouseYPos + 540;
            xe->xmotion.x_root = xe->xmotion.x;
            xe->xmotion.y_root = xe->xmotion.y;
            lastMouseXPos = tmp1;
            lastMouseYPos = tmp2;
        }
        //key_t key;
        //char* filename=NULL;
        if(!timeTrackerAttached){
            char hostname[256];
            char *home = getenv("HOME");
            char *vncfolder = "/.vnc/";
            gethostname(hostname, sizeof(hostname));
            char *num_string = getenv("DISPLAY");
            char *posfix = ".pid";
            //filename = (char*)malloc(strlen(posfix) + strlen(num_string)+strlen(hostname)+strlen(vncfolder)+strlen(home)+16); // +1 for the null-terminator
            char *filename = (char*)malloc(strlen(posfix) + strlen(num_string)+strlen(hostname)+strlen(vncfolder)+strlen(home)+16); // +1 for the null-terminator
            strcpy(filename, home);
            strcat(filename, vncfolder);
            strcat(filename, hostname);
            strcat(filename, num_string);
            strcat(filename, posfix);
            key_t key = ftok(filename, 65);
            key = ftok(filename, 65);

            int shmid = shmget(key, NUM_ROW * sizeof(timeTrack), 0666|IPC_CREAT);
            timeTracker = (timeTrack*) shmat(shmid, (void*)0, 0);
            timeTrackerAttached = 1;
	    read_clear = 0;
            free(filename);
        }
        XKeyEvent* xkey = (XKeyEvent*)xe;
        //if(filename != NULL)
	//    fprintf(stderr,"filename: %s, key:%d\n", filename, key);
	//if((xe->type == KeyPress || xe->type == 6) && read_clear == 0 && (xkey->time != keypointer_eventID)){
	if((xe->type == KeyPress || xe->type == 6) && (xkey->time != keypointer_eventID)){
            keypointer_eventID = xkey->time;
            for(i=1;i<NUM_ROW;i++){
               if(timeTracker[i].eventID == keypointer_eventID){
                  timeTracker[i].array[4] = (long long)gettime_nanoTime();//usTevent_pickup
                  current_event_index = i;
	          read_clear = 0xdeadbeef;
                  break;
               }
            }
            if(i == NUM_ROW){
	        read_clear = 0;
            }
        }else{
            read_clear = 0;
        }
	handleEvent(dpy, xe);

	//CATCH();
	return retval;
}

int XPutImage(Display *dpy, Drawable d, GC gc, XImage *image, int src_x, int src_y, int dest_x, int dest_y, unsigned int width, unsigned int height){
       int xputimage_value = _XPutImage(dpy, d, gc, image, src_x, src_y, dest_x, dest_y, width, height);
       return xputimage_value;
}

int XResizeWindow(Display *dpy, Window win, unsigned int width,
	unsigned int height)
{
	int retval = 0;
	TRY();

	if(isExcluded(dpy))
		return _XResizeWindow(dpy, win, width, height);

		opentrace(XResizeWindow);  prargd(dpy);  prargx(win);  prargi(width);
		prargi(height);  starttrace();

	VirtualWin *vw = NULL;
	if(winhash.find(dpy, win, vw)) vw->resize(width, height);
	retval = _XResizeWindow(dpy, win, width, height);

		stoptrace();  closetrace();

	CATCH();
	return retval;
}


int XWindowEvent(Display *dpy, Window win, long event_mask, XEvent *xe)
{
	int retval = 0;
	TRY();

	retval = _XWindowEvent(dpy, win, event_mask, xe);
	handleEvent(dpy, xe);

	CATCH();
	return retval;
}


}  // extern "C"
