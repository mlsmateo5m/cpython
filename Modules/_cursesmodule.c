/*
 *   This is a curses module for Python.
 *
 *   Based on prior work by Lance Ellinghaus and Oliver Andrich
 *   Version 1.2 of this module: Copyright 1994 by Lance Ellinghouse,
 *    Cathedral City, California Republic, United States of America.
 *
 *   Version 1.5b1, heavily extended for ncurses by Oliver Andrich:
 *   Copyright 1996,1997 by Oliver Andrich, Koblenz, Germany.
 *
 *   Tidied for Python 1.6, and currently maintained by <amk@amk.ca>.
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this source file to use, copy, modify, merge, or publish it
 *   subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included
 *   in all copies or in any new file that contains a substantial portion of
 *   this file.
 *
 *   THE  AUTHOR  MAKES  NO  REPRESENTATIONS ABOUT  THE  SUITABILITY  OF
 *   THE  SOFTWARE FOR  ANY  PURPOSE.  IT IS  PROVIDED  "AS IS"  WITHOUT
 *   EXPRESS OR  IMPLIED WARRANTY.  THE AUTHOR DISCLAIMS  ALL WARRANTIES
 *   WITH  REGARD TO  THIS  SOFTWARE, INCLUDING  ALL IMPLIED  WARRANTIES
 *   OF   MERCHANTABILITY,  FITNESS   FOR  A   PARTICULAR  PURPOSE   AND
 *   NON-INFRINGEMENT  OF THIRD  PARTY  RIGHTS. IN  NO  EVENT SHALL  THE
 *   AUTHOR  BE LIABLE  TO  YOU  OR ANY  OTHER  PARTY  FOR ANY  SPECIAL,
 *   INDIRECT,  OR  CONSEQUENTIAL  DAMAGES  OR  ANY  DAMAGES  WHATSOEVER
 *   WHETHER IN AN  ACTION OF CONTRACT, NEGLIGENCE,  STRICT LIABILITY OR
 *   ANY OTHER  ACTION ARISING OUT OF  OR IN CONNECTION WITH  THE USE OR
 *   PERFORMANCE OF THIS SOFTWARE.
 */

/* CVS: $Id$ */

/*

A number of SysV or ncurses functions don't have wrappers yet; if you
need a given function, add it and send a patch.  See
http://www.python.org/dev/patches/ for instructions on how to submit
patches to Python.

Here's a list of currently unsupported functions:

	addchnstr addchstr color_set define_key
	del_curterm delscreen dupwin inchnstr inchstr innstr keyok
	mcprint mvaddchnstr mvaddchstr mvcur mvinchnstr
	mvinchstr mvinnstr mmvwaddchnstr mvwaddchstr 
	mvwinchnstr mvwinchstr mvwinnstr newterm
	restartterm ripoffline scr_dump
	scr_init scr_restore scr_set scrl set_curterm set_term setterm
	tgetent tgetflag tgetnum tgetstr tgoto timeout tputs
	vidattr vidputs waddchnstr waddchstr
	wcolor_set winchnstr winchstr winnstr wmouse_trafo wscrl

Low-priority: 
	slk_attr slk_attr_off slk_attr_on slk_attr_set slk_attroff
	slk_attron slk_attrset slk_clear slk_color slk_init slk_label
	slk_noutrefresh slk_refresh slk_restore slk_set slk_touch

Menu extension (ncurses and probably SYSV):
	current_item free_item free_menu item_count item_description
	item_index item_init item_name item_opts item_opts_off
	item_opts_on item_term item_userptr item_value item_visible
	menu_back menu_driver menu_fore menu_format menu_grey
	menu_init menu_items menu_mark menu_opts menu_opts_off
	menu_opts_on menu_pad menu_pattern menu_request_by_name
	menu_request_name menu_spacing menu_sub menu_term menu_userptr
	menu_win new_item new_menu pos_menu_cursor post_menu
	scale_menu set_current_item set_item_init set_item_opts
	set_item_term set_item_userptr set_item_value set_menu_back
	set_menu_fore set_menu_format set_menu_grey set_menu_init
	set_menu_items set_menu_mark set_menu_opts set_menu_pad
	set_menu_pattern set_menu_spacing set_menu_sub set_menu_term
	set_menu_userptr set_menu_win set_top_row top_row unpost_menu

Form extension (ncurses and probably SYSV):
	current_field data_ahead data_behind dup_field
	dynamic_fieldinfo field_arg field_back field_buffer
	field_count field_fore field_index field_info field_init
	field_just field_opts field_opts_off field_opts_on field_pad
	field_status field_term field_type field_userptr form_driver
	form_fields form_init form_opts form_opts_off form_opts_on
	form_page form_request_by_name form_request_name form_sub
	form_term form_userptr form_win free_field free_form
	link_field link_fieldtype move_field new_field new_form
	new_page pos_form_cursor post_form scale_form
	set_current_field set_field_back set_field_buffer
	set_field_fore set_field_init set_field_just set_field_opts
	set_field_pad set_field_status set_field_term set_field_type
	set_field_userptr set_fieldtype_arg set_fieldtype_choice
	set_form_fields set_form_init set_form_opts set_form_page
	set_form_sub set_form_term set_form_userptr set_form_win
	set_max_field set_new_page unpost_form


 */

/* Release Number */

char *PyCursesVersion = "2.2";

/* Includes */

#include "Python.h"

#ifdef __osf__
#define STRICT_SYSV_CURSES      /* Don't use ncurses extensions */
#endif

#ifdef __hpux
#define STRICT_SYSV_CURSES
#endif

#define CURSES_MODULE
#include "py_curses.h"

/*  These prototypes are in <term.h>, but including this header 
    #defines many common symbols (such as "lines") which breaks the 
    curses module in other ways.  So the code will just specify 
    explicit prototypes here. */
extern int setupterm(char *,int,int *);
#ifdef __sgi
#include <term.h>
#endif

#if !defined(HAVE_NCURSES_H) && (defined(sgi) || defined(__sun) || defined(SCO5))
#define STRICT_SYSV_CURSES       /* Don't use ncurses extensions */
typedef chtype attr_t;           /* No attr_t type is available */
#endif

#if defined(_AIX)
#define STRICT_SYSV_CURSES
#endif

/* Definition of exception curses.error */

static PyObject *PyCursesError;

/* Tells whether setupterm() has been called to initialise terminfo.  */
static int initialised_setupterm = FALSE;

/* Tells whether initscr() has been called to initialise curses.  */
static int initialised = FALSE;

/* Tells whether start_color() has been called to initialise color usage. */
static int initialisedcolors = FALSE;

/* Utility Macros */
#define PyCursesSetupTermCalled \
  if (initialised_setupterm != TRUE) { \
                  PyErr_SetString(PyCursesError, \
                                  "must call (at least) setupterm() first"); \
                  return 0; }

#define PyCursesInitialised \
  if (initialised != TRUE) { \
                  PyErr_SetString(PyCursesError, \
                                  "must call initscr() first"); \
                  return 0; }

#define PyCursesInitialisedColor \
  if (initialisedcolors != TRUE) { \
                  PyErr_SetString(PyCursesError, \
                                  "must call start_color() first"); \
                  return 0; }

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

/* Utility Functions */

/*
 * Check the return code from a curses function and return None 
 * or raise an exception as appropriate.  These are exported using the
 * CObject API. 
 */

static PyObject *
PyCursesCheckERR(int code, char *fname)
{
  if (code != ERR) {
    Py_INCREF(Py_None);
    return Py_None;
  } else {
    if (fname == NULL) {
      PyErr_SetString(PyCursesError, catchall_ERR);
    } else {
      PyErr_Format(PyCursesError, "%s() returned ERR", fname);
    }
    return NULL;
  }
}

static int 
PyCurses_ConvertToChtype(PyObject *obj, chtype *ch)
{
  if (PyInt_CheckExact(obj)) {
    *ch = (chtype) PyInt_AsLong(obj);
  } else if(PyString_Check(obj) 
	    && (PyString_Size(obj) == 1)) {
    *ch = (chtype) *PyString_AsString(obj);
  } else if (PyUnicode_Check(obj) && PyUnicode_GetSize(obj) == 1) {
    *ch = (chtype) *PyUnicode_AS_UNICODE(obj);
  } else {
    return 0;
  }
  return 1;
}

/* Function versions of the 3 functions for tested whether curses has been
   initialised or not. */
   
static int func_PyCursesSetupTermCalled(void)
{
    PyCursesSetupTermCalled;
    return 1;
}

static int func_PyCursesInitialised(void)
{
    PyCursesInitialised;
    return 1;
}

static int func_PyCursesInitialisedColor(void)
{
    PyCursesInitialisedColor;
    return 1;
}

/*****************************************************************************
 The Window Object
******************************************************************************/

/* Definition of the window type */

PyTypeObject PyCursesWindow_Type;

/* Function prototype macros for Window object

   X - function name
   TYPE - parameter Type
   ERGSTR - format string for construction of the return value
   PARSESTR - format string for argument parsing
   */

#define Window_NoArgNoReturnFunction(X) \
static PyObject *PyCursesWindow_ ## X (PyCursesWindowObject *self, PyObject *args) \
{ return PyCursesCheckERR(X(self->win), # X); }

#define Window_NoArgTrueFalseFunction(X) \
static PyObject * PyCursesWindow_ ## X (PyCursesWindowObject *self) \
{ \
  if (X (self->win) == FALSE) { Py_INCREF(Py_False); return Py_False; } \
  else { Py_INCREF(Py_True); return Py_True; } }

#define Window_NoArgNoReturnVoidFunction(X) \
static PyObject * PyCursesWindow_ ## X (PyCursesWindowObject *self) \
{ \
  X(self->win); Py_INCREF(Py_None); return Py_None; }

#define Window_NoArg2TupleReturnFunction(X, TYPE, ERGSTR) \
static PyObject * PyCursesWindow_ ## X (PyCursesWindowObject *self) \
{ \
  TYPE arg1, arg2; \
  X(self->win,arg1,arg2); return Py_BuildValue(ERGSTR, arg1, arg2); } 

#define Window_OneArgNoReturnVoidFunction(X, TYPE, PARSESTR) \
static PyObject * PyCursesWindow_ ## X (PyCursesWindowObject *self, PyObject *args) \
{ \
  TYPE arg1; \
  if (!PyArg_ParseTuple(args, PARSESTR, &arg1)) return NULL; \
  X(self->win,arg1); Py_INCREF(Py_None); return Py_None; }

#define Window_OneArgNoReturnFunction(X, TYPE, PARSESTR) \
static PyObject * PyCursesWindow_ ## X (PyCursesWindowObject *self, PyObject *args) \
{ \
  TYPE arg1; \
  if (!PyArg_ParseTuple(args,PARSESTR, &arg1)) return NULL; \
  return PyCursesCheckERR(X(self->win, arg1), # X); }

#define Window_TwoArgNoReturnFunction(X, TYPE, PARSESTR) \
static PyObject * PyCursesWindow_ ## X (PyCursesWindowObject *self, PyObject *args) \
{ \
  TYPE arg1, arg2; \
  if (!PyArg_ParseTuple(args,PARSESTR, &arg1, &arg2)) return NULL; \
  return PyCursesCheckERR(X(self->win, arg1, arg2), # X); }

/* ------------- WINDOW routines --------------- */

Window_NoArgNoReturnFunction(untouchwin)
Window_NoArgNoReturnFunction(touchwin)
Window_NoArgNoReturnFunction(redrawwin)
Window_NoArgNoReturnFunction(winsertln)
Window_NoArgNoReturnFunction(werase)
Window_NoArgNoReturnFunction(wdeleteln)

Window_NoArgTrueFalseFunction(is_wintouched)

Window_NoArgNoReturnVoidFunction(wsyncup)
Window_NoArgNoReturnVoidFunction(wsyncdown)
Window_NoArgNoReturnVoidFunction(wstandend)
Window_NoArgNoReturnVoidFunction(wstandout)
Window_NoArgNoReturnVoidFunction(wcursyncup)
Window_NoArgNoReturnVoidFunction(wclrtoeol)
Window_NoArgNoReturnVoidFunction(wclrtobot)
Window_NoArgNoReturnVoidFunction(wclear)

Window_OneArgNoReturnVoidFunction(idcok, int, "i;True(1) or False(0)")
Window_OneArgNoReturnVoidFunction(immedok, int, "i;True(1) or False(0)")
Window_OneArgNoReturnVoidFunction(wtimeout, int, "i;delay")

Window_NoArg2TupleReturnFunction(getyx, int, "ii")
Window_NoArg2TupleReturnFunction(getbegyx, int, "ii")
Window_NoArg2TupleReturnFunction(getmaxyx, int, "ii")
Window_NoArg2TupleReturnFunction(getparyx, int, "ii")

Window_OneArgNoReturnFunction(wattron, attr_t, "l;attr")
Window_OneArgNoReturnFunction(wattroff, attr_t, "l;attr")
Window_OneArgNoReturnFunction(wattrset, attr_t, "l;attr")
Window_OneArgNoReturnFunction(clearok, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(idlok, int, "i;True(1) or False(0)")
#if defined(__NetBSD__)
Window_OneArgNoReturnVoidFunction(keypad, int, "i;True(1) or False(0)")
#else
Window_OneArgNoReturnFunction(keypad, int, "i;True(1) or False(0)")
#endif
Window_OneArgNoReturnFunction(leaveok, int, "i;True(1) or False(0)")
#if defined(__NetBSD__)
Window_OneArgNoReturnVoidFunction(nodelay, int, "i;True(1) or False(0)")
#else
Window_OneArgNoReturnFunction(nodelay, int, "i;True(1) or False(0)")
#endif
Window_OneArgNoReturnFunction(notimeout, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(scrollok, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(winsdelln, int, "i;nlines")
Window_OneArgNoReturnFunction(syncok, int, "i;True(1) or False(0)")

Window_TwoArgNoReturnFunction(mvwin, int, "ii;y,x")
Window_TwoArgNoReturnFunction(mvderwin, int, "ii;y,x")
Window_TwoArgNoReturnFunction(wmove, int, "ii;y,x")
#ifndef STRICT_SYSV_CURSES
Window_TwoArgNoReturnFunction(wresize, int, "ii;lines,columns")
#endif

/* Allocation and deallocation of Window Objects */

static PyObject *
PyCursesWindow_New(WINDOW *win)
{
	PyCursesWindowObject *wo;

	wo = PyObject_NEW(PyCursesWindowObject, &PyCursesWindow_Type);
	if (wo == NULL) return NULL;
	wo->win = win;
	return (PyObject *)wo;
}

static void
PyCursesWindow_Dealloc(PyCursesWindowObject *wo)
{
  if (wo->win != stdscr) delwin(wo->win);
  PyObject_DEL(wo);
}

/* Addch, Addstr, Addnstr */

static PyObject *
PyCursesWindow_AddCh(PyCursesWindowObject *self, PyObject *args)
{
  int rtn, x, y, use_xy = FALSE;
  PyObject *temp;
  chtype ch = 0;
  attr_t attr = A_NORMAL;
  
  switch (PyTuple_Size(args)) {
  case 1:
    if (!PyArg_ParseTuple(args, "O;ch or int", &temp))
	  return NULL;
    break;
  case 2:
    if (!PyArg_ParseTuple(args, "Ol;ch or int,attr", &temp, &attr))
      return NULL;
    break;
  case 3:
    if (!PyArg_ParseTuple(args,"iiO;y,x,ch or int", &y, &x, &temp))
      return NULL;
    use_xy = TRUE;
    break;
  case 4:
    if (!PyArg_ParseTuple(args,"iiOl;y,x,ch or int, attr", 
		     &y, &x, &temp, &attr))
      return NULL;
    use_xy = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "addch requires 1 to 4 arguments");
    return NULL;
  }

  if (!PyCurses_ConvertToChtype(temp, &ch)) {
    PyErr_SetString(PyExc_TypeError, "argument 1 or 3 must be a ch or an int");
    return NULL;
  }
  
  if (use_xy == TRUE)
    rtn = mvwaddch(self->win,y,x, ch | attr);
  else {
    rtn = waddch(self->win, ch | attr);
  }
  return PyCursesCheckERR(rtn, "addch");
}

static PyObject *
PyCursesWindow_AddStr(PyCursesWindowObject *self, PyObject *args)
{
  int rtn;
  int x, y;
  char *str;
  attr_t attr = A_NORMAL , attr_old = A_NORMAL;
  int use_xy = FALSE, use_attr = FALSE;

  switch (PyTuple_Size(args)) {
  case 1:
    if (!PyArg_ParseTuple(args,"s;str", &str))
      return NULL;
    break;
  case 2:
    if (!PyArg_ParseTuple(args,"sl;str,attr", &str, &attr))
      return NULL;
    use_attr = TRUE;
    break;
  case 3:
    if (!PyArg_ParseTuple(args,"iis;int,int,str", &y, &x, &str))
      return NULL;
    use_xy = TRUE;
    break;
  case 4:
    if (!PyArg_ParseTuple(args,"iisl;int,int,str,attr", &y, &x, &str, &attr))
      return NULL;
    use_xy = use_attr = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "addstr requires 1 to 4 arguments");
    return NULL;
  }

  if (use_attr == TRUE) {
    attr_old = getattrs(self->win);
    wattrset(self->win,attr);
  }
  if (use_xy == TRUE)
    rtn = mvwaddstr(self->win,y,x,str);
  else
    rtn = waddstr(self->win,str);
  if (use_attr == TRUE)
    wattrset(self->win,attr_old);
  return PyCursesCheckERR(rtn, "addstr");
}

static PyObject *
PyCursesWindow_AddNStr(PyCursesWindowObject *self, PyObject *args)
{
  int rtn, x, y, n;
  char *str;
  attr_t attr = A_NORMAL , attr_old = A_NORMAL;
  int use_xy = FALSE, use_attr = FALSE;

  switch (PyTuple_Size(args)) {
  case 2:
    if (!PyArg_ParseTuple(args,"si;str,n", &str, &n))
      return NULL;
    break;
  case 3:
    if (!PyArg_ParseTuple(args,"sil;str,n,attr", &str, &n, &attr))
      return NULL;
    use_attr = TRUE;
    break;
  case 4:
    if (!PyArg_ParseTuple(args,"iisi;y,x,str,n", &y, &x, &str, &n))
      return NULL;
    use_xy = TRUE;
    break;
  case 5:
    if (!PyArg_ParseTuple(args,"iisil;y,x,str,n,attr", &y, &x, &str, &n, &attr))
      return NULL;
    use_xy = use_attr = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "addnstr requires 2 to 5 arguments");
    return NULL;
  }

  if (use_attr == TRUE) {
    attr_old = getattrs(self->win);
    wattrset(self->win,attr);
  }
  if (use_xy == TRUE)
    rtn = mvwaddnstr(self->win,y,x,str,n);
  else
    rtn = waddnstr(self->win,str,n);
  if (use_attr == TRUE)
    wattrset(self->win,attr_old);
  return PyCursesCheckERR(rtn, "addnstr");
}

static PyObject *
PyCursesWindow_Bkgd(PyCursesWindowObject *self, PyObject *args)
{
  PyObject *temp;
  chtype bkgd;
  attr_t attr = A_NORMAL;

  switch (PyTuple_Size(args)) {
    case 1:
      if (!PyArg_ParseTuple(args, "O;ch or int", &temp))
        return NULL;
      break;
    case 2:
      if (!PyArg_ParseTuple(args,"Ol;ch or int,attr", &temp, &attr))
        return NULL;
      break;
    default:
      PyErr_SetString(PyExc_TypeError, "bkgd requires 1 or 2 arguments");
      return NULL;
  }

  if (!PyCurses_ConvertToChtype(temp, &bkgd)) {
    PyErr_SetString(PyExc_TypeError, "argument 1 or 3 must be a ch or an int");
    return NULL;
  }

  return PyCursesCheckERR(wbkgd(self->win, bkgd | attr), "bkgd");
}

static PyObject *
PyCursesWindow_BkgdSet(PyCursesWindowObject *self, PyObject *args)
{
  PyObject *temp;
  chtype bkgd;
  attr_t attr = A_NORMAL;

  switch (PyTuple_Size(args)) {
    case 1:
      if (!PyArg_ParseTuple(args, "O;ch or int", &temp))
        return NULL;
      break;
    case 2:
      if (!PyArg_ParseTuple(args,"Ol;ch or int,attr", &temp, &attr))
        return NULL;
      break;
    default:
      PyErr_SetString(PyExc_TypeError, "bkgdset requires 1 or 2 arguments");
      return NULL;
  }

  if (!PyCurses_ConvertToChtype(temp, &bkgd)) {
    PyErr_SetString(PyExc_TypeError, "argument 1 must be a ch or an int");
    return NULL;
  }

  wbkgdset(self->win, bkgd | attr);
  return PyCursesCheckERR(0, "bkgdset");
}

static PyObject *
PyCursesWindow_Border(PyCursesWindowObject *self, PyObject *args)
{
  PyObject *temp[8];
  chtype ch[8];
  int i;

  /* Clear the array of parameters */
  for(i=0; i<8; i++) {
       temp[i] = NULL;
       ch[i] = 0;
  }    
  
  if (!PyArg_ParseTuple(args,"|OOOOOOOO;ls,rs,ts,bs,tl,tr,bl,br",
                        &temp[0], &temp[1], &temp[2], &temp[3],
                        &temp[4], &temp[5], &temp[6], &temp[7]))
    return NULL;

  for(i=0; i<8; i++) {
      if (temp[i] != NULL && !PyCurses_ConvertToChtype(temp[i], &ch[i])) {
          PyErr_Format(PyExc_TypeError,
                       "argument %i must be a ch or an int", i+1);
          return NULL;
      }
  }
  
  wborder(self->win,
          ch[0], ch[1], ch[2], ch[3],
          ch[4], ch[5], ch[6], ch[7]);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_Box(PyCursesWindowObject *self, PyObject *args)
{
  chtype ch1=0,ch2=0;
  switch(PyTuple_Size(args)){
  case 0: break;
  default:
    if (!PyArg_ParseTuple(args,"ll;vertint,horint", &ch1, &ch2))
      return NULL;
  }
  box(self->win,ch1,ch2);
  Py_INCREF(Py_None);
  return Py_None;
}

#if defined(HAVE_NCURSES_H) || defined(MVWDELCH_IS_EXPRESSION)
#define py_mvwdelch mvwdelch
#else
int py_mvwdelch(WINDOW *w, int y, int x)
{
  mvwdelch(w,y,x);
  /* On HP/UX, mvwdelch already returns. On other systems,
     we may well run into this return statement. */
  return 0;
}
#endif

/* chgat, added by Fabian Kreutz <fabian.kreutz at gmx.net> */

static PyObject *
PyCursesWindow_ChgAt(PyCursesWindowObject *self, PyObject *args)
{
  int rtn;
  int x, y;
  int num = -1;
  short color;
  attr_t attr = A_NORMAL;
  int use_xy = FALSE;

  switch (PyTuple_Size(args)) {
  case 1:
    if (!PyArg_ParseTuple(args,"l;attr", &attr))
      return NULL;
    break;
  case 2:
    if (!PyArg_ParseTuple(args,"il;n,attr", &num, &attr))
      return NULL;
    break;
  case 3:
    if (!PyArg_ParseTuple(args,"iil;int,int,attr", &y, &x, &attr))
      return NULL;
    use_xy = TRUE;
    break;
  case 4:
    if (!PyArg_ParseTuple(args,"iiil;int,int,n,attr", &y, &x, &num, &attr))
      return NULL;
    use_xy = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "chgat requires 1 to 4 arguments");
    return NULL;
  }

  color = (short)((attr >> 8) & 0xff);
  attr = attr - (color << 8);

  if (use_xy == TRUE) {
    rtn = mvwchgat(self->win,y,x,num,attr,color,NULL);
    touchline(self->win,y,1);
  } else {
    getyx(self->win,y,x);
    rtn = wchgat(self->win,num,attr,color,NULL);
    touchline(self->win,y,1);
  }
  return PyCursesCheckERR(rtn, "chgat");
}


static PyObject *
PyCursesWindow_DelCh(PyCursesWindowObject *self, PyObject *args)
{
  int rtn;
  int x, y;

  switch (PyTuple_Size(args)) {
  case 0:
    rtn = wdelch(self->win);
    break;
  case 2:
    if (!PyArg_ParseTuple(args,"ii;y,x", &y, &x))
      return NULL;
    rtn = py_mvwdelch(self->win,y,x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "delch requires 0 or 2 arguments");
    return NULL;
  }
  return PyCursesCheckERR(rtn, "[mv]wdelch");
}

static PyObject *
PyCursesWindow_DerWin(PyCursesWindowObject *self, PyObject *args)
{
  WINDOW *win;
  int nlines, ncols, begin_y, begin_x;

  nlines = 0;
  ncols  = 0;
  switch (PyTuple_Size(args)) {
  case 2:
    if (!PyArg_ParseTuple(args,"ii;begin_y,begin_x",&begin_y,&begin_x))
      return NULL;
    break;
  case 4:
    if (!PyArg_ParseTuple(args, "iiii;nlines,ncols,begin_y,begin_x",
		   &nlines,&ncols,&begin_y,&begin_x))
      return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "derwin requires 2 or 4 arguments");
    return NULL;
  }

  win = derwin(self->win,nlines,ncols,begin_y,begin_x);

  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }

  return (PyObject *)PyCursesWindow_New(win);
}

static PyObject *
PyCursesWindow_EchoChar(PyCursesWindowObject *self, PyObject *args)
{
  PyObject *temp;
  chtype ch;
  attr_t attr = A_NORMAL;

  switch (PyTuple_Size(args)) {
  case 1:
    if (!PyArg_ParseTuple(args,"O;ch or int", &temp))
      return NULL;
    break;
  case 2:
    if (!PyArg_ParseTuple(args,"Ol;ch or int,attr", &temp, &attr))
      return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "echochar requires 1 or 2 arguments");


    return NULL;
  }

  if (!PyCurses_ConvertToChtype(temp, &ch)) {
    PyErr_SetString(PyExc_TypeError, "argument 1 must be a ch or an int");
    return NULL;
  }
  
#ifdef WINDOW_HAS_FLAGS
  if (self->win->_flags & _ISPAD)
    return PyCursesCheckERR(pechochar(self->win, ch | attr), 
			    "echochar");
  else
#endif
    return PyCursesCheckERR(wechochar(self->win, ch | attr), 
			    "echochar");
}

#ifdef NCURSES_MOUSE_VERSION
static PyObject *
PyCursesWindow_Enclose(PyCursesWindowObject *self, PyObject *args)
{
	int x, y;
	if (!PyArg_ParseTuple(args,"ii;y,x", &y, &x))
		return NULL;

	return PyInt_FromLong( wenclose(self->win,y,x) );
}
#endif

static PyObject *
PyCursesWindow_GetBkgd(PyCursesWindowObject *self)
{
  return PyInt_FromLong((long) getbkgd(self->win));
}

static PyObject *
PyCursesWindow_GetCh(PyCursesWindowObject *self, PyObject *args)
{
  int x, y;
  int rtn;

  switch (PyTuple_Size(args)) {
  case 0:
    Py_BEGIN_ALLOW_THREADS
    rtn = wgetch(self->win);
    Py_END_ALLOW_THREADS
    break;
  case 2:
    if (!PyArg_ParseTuple(args,"ii;y,x",&y,&x))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    rtn = mvwgetch(self->win,y,x);
    Py_END_ALLOW_THREADS
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "getch requires 0 or 2 arguments");
    return NULL;
  }
  return PyInt_FromLong((long)rtn);
}

static PyObject *
PyCursesWindow_GetKey(PyCursesWindowObject *self, PyObject *args)
{
  int x, y;
  int rtn;

  switch (PyTuple_Size(args)) {
  case 0:
    Py_BEGIN_ALLOW_THREADS
    rtn = wgetch(self->win);
    Py_END_ALLOW_THREADS
    break;
  case 2:
    if (!PyArg_ParseTuple(args,"ii;y,x",&y,&x))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    rtn = mvwgetch(self->win,y,x);
    Py_END_ALLOW_THREADS
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "getkey requires 0 or 2 arguments");
    return NULL;
  }
  if (rtn == ERR) {
    /* getch() returns ERR in nodelay mode */
    PyErr_SetString(PyCursesError, "no input");
    return NULL;
  } else if (rtn<=255)
    return Py_BuildValue("c", rtn);
  else
#if defined(__NetBSD__)
    return PyUnicode_FromString(unctrl(rtn));
#else
    return PyUnicode_FromString((const char *)keyname(rtn));
#endif
}

static PyObject *
PyCursesWindow_GetStr(PyCursesWindowObject *self, PyObject *args)
{
  int x, y, n;
  char rtn[1024]; /* This should be big enough.. I hope */
  int rtn2;

  switch (PyTuple_Size(args)) {
  case 0:
    Py_BEGIN_ALLOW_THREADS
    rtn2 = wgetnstr(self->win,rtn, 1023);
    Py_END_ALLOW_THREADS
    break;
  case 1:
    if (!PyArg_ParseTuple(args,"i;n", &n))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
    rtn2 = wgetnstr(self->win,rtn,MIN(n, 1023));
    Py_END_ALLOW_THREADS
    break;
  case 2:
    if (!PyArg_ParseTuple(args,"ii;y,x",&y,&x))
      return NULL;
    Py_BEGIN_ALLOW_THREADS
#ifdef STRICT_SYSV_CURSES
    rtn2 = wmove(self->win,y,x)==ERR ? ERR : wgetnstr(self->win, rtn, 1023);
#else
    rtn2 = mvwgetnstr(self->win,y,x,rtn, 1023);
#endif
    Py_END_ALLOW_THREADS
    break;
  case 3:
    if (!PyArg_ParseTuple(args,"iii;y,x,n", &y, &x, &n))
      return NULL;
#ifdef STRICT_SYSV_CURSES
    Py_BEGIN_ALLOW_THREADS
    rtn2 = wmove(self->win,y,x)==ERR ? ERR :
      wgetnstr(self->win, rtn, MIN(n, 1023));
    Py_END_ALLOW_THREADS
#else
    Py_BEGIN_ALLOW_THREADS
    rtn2 = mvwgetnstr(self->win, y, x, rtn, MIN(n, 1023));
    Py_END_ALLOW_THREADS
#endif
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "getstr requires 0 to 3 arguments");
    return NULL;
  }
  if (rtn2 == ERR)
    rtn[0] = 0;
  return PyString_FromString(rtn);
}

static PyObject *
PyCursesWindow_Hline(PyCursesWindowObject *self, PyObject *args)
{
  PyObject *temp;
  chtype ch;
  int n, x, y, code = OK;
  attr_t attr = A_NORMAL;

  switch (PyTuple_Size(args)) {
  case 2:
    if (!PyArg_ParseTuple(args, "Oi;ch or int,n", &temp, &n))
      return NULL;
    break;
  case 3:
    if (!PyArg_ParseTuple(args, "Oil;ch or int,n,attr", &temp, &n, &attr))
      return NULL;
    break;
  case 4:
    if (!PyArg_ParseTuple(args, "iiOi;y,x,ch or int,n", &y, &x, &temp, &n))
      return NULL;
    code = wmove(self->win, y, x);
    break;
  case 5:
    if (!PyArg_ParseTuple(args, "iiOil; y,x,ch or int,n,attr", 
		     &y, &x, &temp, &n, &attr))
      return NULL;
    code = wmove(self->win, y, x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "hline requires 2 to 5 arguments");
    return NULL;
  }

  if (code != ERR) {
    if (!PyCurses_ConvertToChtype(temp, &ch)) {
      PyErr_SetString(PyExc_TypeError, 
		      "argument 1 or 3 must be a ch or an int");
      return NULL;
    }
    return PyCursesCheckERR(whline(self->win, ch | attr, n), "hline");
  } else 
    return PyCursesCheckERR(code, "wmove");
}

static PyObject *
PyCursesWindow_InsCh(PyCursesWindowObject *self, PyObject *args)
{
  int rtn, x, y, use_xy = FALSE;
  PyObject *temp;
  chtype ch = 0;
  attr_t attr = A_NORMAL;
  
  switch (PyTuple_Size(args)) {
  case 1:
    if (!PyArg_ParseTuple(args, "O;ch or int", &temp))
      return NULL;
    break;
  case 2:
    if (!PyArg_ParseTuple(args, "Ol;ch or int,attr", &temp, &attr))
      return NULL;
    break;
  case 3:
    if (!PyArg_ParseTuple(args,"iiO;y,x,ch or int", &y, &x, &temp))
      return NULL;
    use_xy = TRUE;
    break;
  case 4:
    if (!PyArg_ParseTuple(args,"iiOl;y,x,ch or int, attr", &y, &x, &temp, &attr))
      return NULL;
    use_xy = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "insch requires 1 or 4 arguments");
    return NULL;
  }

  if (!PyCurses_ConvertToChtype(temp, &ch)) {
    PyErr_SetString(PyExc_TypeError, 
		    "argument 1 or 3 must be a ch or an int");
    return NULL;
  }
  
  if (use_xy == TRUE)
    rtn = mvwinsch(self->win,y,x, ch | attr);
  else {
    rtn = winsch(self->win, ch | attr);
  }
  return PyCursesCheckERR(rtn, "insch");
}

static PyObject *
PyCursesWindow_InCh(PyCursesWindowObject *self, PyObject *args)
{
  int x, y, rtn;

  switch (PyTuple_Size(args)) {
  case 0:
    rtn = winch(self->win);
    break;
  case 2:
    if (!PyArg_ParseTuple(args,"ii;y,x",&y,&x))
      return NULL;
    rtn = mvwinch(self->win,y,x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "inch requires 0 or 2 arguments");
    return NULL;
  }
  return PyInt_FromLong((long) rtn);
}

static PyObject *
PyCursesWindow_InStr(PyCursesWindowObject *self, PyObject *args)
{
  int x, y, n;
  char rtn[1024]; /* This should be big enough.. I hope */
  int rtn2;

  switch (PyTuple_Size(args)) {
  case 0:
    rtn2 = winnstr(self->win,rtn, 1023);
    break;
  case 1:
    if (!PyArg_ParseTuple(args,"i;n", &n))
      return NULL;
    rtn2 = winnstr(self->win,rtn,MIN(n,1023));
    break;
  case 2:
    if (!PyArg_ParseTuple(args,"ii;y,x",&y,&x))
      return NULL;
    rtn2 = mvwinnstr(self->win,y,x,rtn,1023);
    break;
  case 3:
    if (!PyArg_ParseTuple(args, "iii;y,x,n", &y, &x, &n))
      return NULL;
    rtn2 = mvwinnstr(self->win, y, x, rtn, MIN(n,1023));
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "instr requires 0 or 3 arguments");
    return NULL;
  }
  if (rtn2 == ERR)
    rtn[0] = 0;
  return PyString_FromString(rtn);
}

static PyObject *
PyCursesWindow_InsStr(PyCursesWindowObject *self, PyObject *args)
{
  int rtn;
  int x, y;
  char *str;
  attr_t attr = A_NORMAL , attr_old = A_NORMAL;
  int use_xy = FALSE, use_attr = FALSE;

  switch (PyTuple_Size(args)) {
  case 1:
    if (!PyArg_ParseTuple(args,"s;str", &str))
      return NULL;
    break;
  case 2:
    if (!PyArg_ParseTuple(args,"sl;str,attr", &str, &attr))
      return NULL;
    use_attr = TRUE;
    break;
  case 3:
    if (!PyArg_ParseTuple(args,"iis;y,x,str", &y, &x, &str))
      return NULL;
    use_xy = TRUE;
    break;
  case 4:
    if (!PyArg_ParseTuple(args,"iisl;y,x,str,attr", &y, &x, &str, &attr))
      return NULL;
    use_xy = use_attr = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "insstr requires 1 to 4 arguments");
    return NULL;
  }

  if (use_attr == TRUE) {
    attr_old = getattrs(self->win);
    wattrset(self->win,attr);
  }
  if (use_xy == TRUE)
    rtn = mvwinsstr(self->win,y,x,str);
  else
    rtn = winsstr(self->win,str);
  if (use_attr == TRUE)
    wattrset(self->win,attr_old);
  return PyCursesCheckERR(rtn, "insstr");
}

static PyObject *
PyCursesWindow_InsNStr(PyCursesWindowObject *self, PyObject *args)
{
  int rtn, x, y, n;
  char *str;
  attr_t attr = A_NORMAL , attr_old = A_NORMAL;
  int use_xy = FALSE, use_attr = FALSE;

  switch (PyTuple_Size(args)) {
  case 2:
    if (!PyArg_ParseTuple(args,"si;str,n", &str, &n))
      return NULL;
    break;
  case 3:
    if (!PyArg_ParseTuple(args,"sil;str,n,attr", &str, &n, &attr))
      return NULL;
    use_attr = TRUE;
    break;
  case 4:
    if (!PyArg_ParseTuple(args,"iisi;y,x,str,n", &y, &x, &str, &n))
      return NULL;
    use_xy = TRUE;
    break;
  case 5:
    if (!PyArg_ParseTuple(args,"iisil;y,x,str,n,attr", &y, &x, &str, &n, &attr))
      return NULL;
    use_xy = use_attr = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "insnstr requires 2 to 5 arguments");
    return NULL;
  }

  if (use_attr == TRUE) {
    attr_old = getattrs(self->win);
    wattrset(self->win,attr);
  }
  if (use_xy == TRUE)
    rtn = mvwinsnstr(self->win,y,x,str,n);
  else
    rtn = winsnstr(self->win,str,n);
  if (use_attr == TRUE)
    wattrset(self->win,attr_old);
  return PyCursesCheckERR(rtn, "insnstr");
}

static PyObject *
PyCursesWindow_Is_LineTouched(PyCursesWindowObject *self, PyObject *args)
{
  int line, erg;
  if (!PyArg_ParseTuple(args,"i;line", &line))
    return NULL;
  erg = is_linetouched(self->win, line);
  if (erg == ERR) {
    PyErr_SetString(PyExc_TypeError, 
		    "is_linetouched: line number outside of boundaries");
    return NULL;
  } else 
    if (erg == FALSE) {
      Py_INCREF(Py_False);
      return Py_False;
    } else {
      Py_INCREF(Py_True);
      return Py_True;
    }
}

static PyObject *
PyCursesWindow_NoOutRefresh(PyCursesWindowObject *self, PyObject *args)
{
  int pminrow,pmincol,sminrow,smincol,smaxrow,smaxcol;
  int rtn;

#ifndef WINDOW_HAS_FLAGS
  if (0) {
#else
  if (self->win->_flags & _ISPAD) {
#endif
    switch(PyTuple_Size(args)) {
    case 6:
      if (!PyArg_ParseTuple(args, 
		       "iiiiii;" \
		       "pminrow,pmincol,sminrow,smincol,smaxrow,smaxcol", 
		       &pminrow, &pmincol, &sminrow, 
		       &smincol, &smaxrow, &smaxcol))
	return NULL;
      Py_BEGIN_ALLOW_THREADS
      rtn = pnoutrefresh(self->win,
			 pminrow, pmincol, sminrow, 
			 smincol, smaxrow, smaxcol);
      Py_END_ALLOW_THREADS
      return PyCursesCheckERR(rtn, "pnoutrefresh");
    default:
      PyErr_SetString(PyCursesError, 
		      "noutrefresh() called for a pad " 
		      "requires 6 arguments");
      return NULL;
    }
  } else {
    if (!PyArg_ParseTuple(args, ":noutrefresh"))
      return NULL;    

    Py_BEGIN_ALLOW_THREADS
    rtn = wnoutrefresh(self->win);
    Py_END_ALLOW_THREADS
    return PyCursesCheckERR(rtn, "wnoutrefresh");
  }
}

static PyObject *
PyCursesWindow_Overlay(PyCursesWindowObject *self, PyObject *args)
{
    PyCursesWindowObject *temp;
    int use_copywin = FALSE;
    int sminrow, smincol, dminrow, dmincol, dmaxrow, dmaxcol;
    int rtn;
    
    switch (PyTuple_Size(args)) {
    case 1:
	if (!PyArg_ParseTuple(args, "O!;window object",
			      &PyCursesWindow_Type, &temp))
	    return NULL;
	break;
    case 7:
	if (!PyArg_ParseTuple(args, "O!iiiiii;window object, int, int, int, int, int, int",
			      &PyCursesWindow_Type, &temp, &sminrow, &smincol,
			      &dminrow, &dmincol, &dmaxrow, &dmaxcol))
	    return NULL;
	use_copywin = TRUE;
	break;
    default:
	PyErr_SetString(PyExc_TypeError,
			"overlay requires one or seven arguments");
	return NULL;
    }

    if (use_copywin == TRUE) {
	    rtn = copywin(self->win, temp->win, sminrow, smincol,
			  dminrow, dmincol, dmaxrow, dmaxcol, TRUE);
	    return PyCursesCheckERR(rtn, "copywin");
    }
    else {
	    rtn = overlay(self->win, temp->win);
	    return PyCursesCheckERR(rtn, "overlay");
    }
}

static PyObject *
PyCursesWindow_Overwrite(PyCursesWindowObject *self, PyObject *args)
{
    PyCursesWindowObject *temp;
    int use_copywin = FALSE;
    int sminrow, smincol, dminrow, dmincol, dmaxrow, dmaxcol;
    int rtn;
    
    switch (PyTuple_Size(args)) {
    case 1:
	if (!PyArg_ParseTuple(args, "O!;window object",
			      &PyCursesWindow_Type, &temp))
	    return NULL;
	break;
    case 7:
	if (!PyArg_ParseTuple(args, "O!iiiiii;window object, int, int, int, int, int, int",
			      &PyCursesWindow_Type, &temp, &sminrow, &smincol,
			      &dminrow, &dmincol, &dmaxrow, &dmaxcol))
	    return NULL;
	use_copywin = TRUE;
	break;
    default:
	PyErr_SetString(PyExc_TypeError,
			"overwrite requires one or seven arguments");
	return NULL;
    }

    if (use_copywin == TRUE) {
	rtn = copywin(self->win, temp->win, sminrow, smincol,
		      dminrow, dmincol, dmaxrow, dmaxcol, FALSE);
        return PyCursesCheckERR(rtn, "copywin");
    }
    else {
	    rtn = overwrite(self->win, temp->win);
	    return PyCursesCheckERR(rtn, "overwrite");
    }
}

static PyObject *
PyCursesWindow_PutWin(PyCursesWindowObject *self, PyObject *stream)
{
  /* We have to simulate this by writing to a temporary FILE*,
     then reading back, then writing to the argument stream. */
  char fn[100];
  int fd;
  FILE *fp;
  PyObject *res;

  strcpy(fn, "/tmp/py.curses.putwin.XXXXXX");
  fd = mkstemp(fn);
  if (fd < 0)
    return PyErr_SetFromErrnoWithFilename(PyExc_IOError, fn);
  fp = fdopen(fd, "wb+");
  if (fp == NULL) {
    close(fd);
    return PyErr_SetFromErrnoWithFilename(PyExc_IOError, fn);
  }
  res = PyCursesCheckERR(putwin(self->win, fp), "putwin");
  if (res == NULL) {
    fclose(fp);
    return res;
  }
  fseek(fp, 0, 0);
  while (1) {
    char buf[BUFSIZ];
    int n = fread(buf, 1, BUFSIZ, fp);
    if (n <= 0)
      break;
    Py_DECREF(res);
    res = PyObject_CallMethod(stream, "write", "y#", buf, n);
    if (res == NULL)
      break;
  }
  fclose(fp);
  remove(fn);
  return res;
}

static PyObject *
PyCursesWindow_RedrawLine(PyCursesWindowObject *self, PyObject *args)
{
  int beg, num;
  if (!PyArg_ParseTuple(args, "ii;beg,num", &beg, &num))
    return NULL;
  return PyCursesCheckERR(wredrawln(self->win,beg,num), "redrawln");
}

static PyObject *
PyCursesWindow_Refresh(PyCursesWindowObject *self, PyObject *args)
{
  int pminrow,pmincol,sminrow,smincol,smaxrow,smaxcol;
  int rtn;
  
#ifndef WINDOW_HAS_FLAGS
  if (0) {
#else
  if (self->win->_flags & _ISPAD) {
#endif
    switch(PyTuple_Size(args)) {
    case 6:
      if (!PyArg_ParseTuple(args, 
		       "iiiiii;" \
		       "pminrow,pmincol,sminrow,smincol,smaxrow,smaxcol", 
		       &pminrow, &pmincol, &sminrow, 
		       &smincol, &smaxrow, &smaxcol))
	return NULL;

      Py_BEGIN_ALLOW_THREADS
      rtn = prefresh(self->win,
		     pminrow, pmincol, sminrow, 
		     smincol, smaxrow, smaxcol);
      Py_END_ALLOW_THREADS
      return PyCursesCheckERR(rtn, "prefresh");
    default:
      PyErr_SetString(PyCursesError, 
		      "refresh() for a pad requires 6 arguments");
      return NULL;
    }
  } else {
    if (!PyArg_ParseTuple(args, ":refresh"))
      return NULL;    
    Py_BEGIN_ALLOW_THREADS
    rtn = wrefresh(self->win);
    Py_END_ALLOW_THREADS
    return PyCursesCheckERR(rtn, "prefresh");    
  }
}

static PyObject *
PyCursesWindow_SetScrollRegion(PyCursesWindowObject *self, PyObject *args)
{
  int x, y;
  if (!PyArg_ParseTuple(args,"ii;top, bottom",&y,&x))
    return NULL;
  return PyCursesCheckERR(wsetscrreg(self->win,y,x), "wsetscrreg");
}

static PyObject *
PyCursesWindow_SubWin(PyCursesWindowObject *self, PyObject *args)
{
  WINDOW *win;
  int nlines, ncols, begin_y, begin_x;

  nlines = 0;
  ncols  = 0;
  switch (PyTuple_Size(args)) {
  case 2:
    if (!PyArg_ParseTuple(args,"ii;begin_y,begin_x",&begin_y,&begin_x))
      return NULL;
    break;
  case 4:
    if (!PyArg_ParseTuple(args, "iiii;nlines,ncols,begin_y,begin_x",
		   &nlines,&ncols,&begin_y,&begin_x))
      return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "subwin requires 2 or 4 arguments");
    return NULL;
  }

  /* printf("Subwin: %i %i %i %i   \n", nlines, ncols, begin_y, begin_x); */
#ifdef WINDOW_HAS_FLAGS
  if (self->win->_flags & _ISPAD)
    win = subpad(self->win, nlines, ncols, begin_y, begin_x);
  else
#endif
    win = subwin(self->win, nlines, ncols, begin_y, begin_x);

  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }
  
  return (PyObject *)PyCursesWindow_New(win);
}

static PyObject *
PyCursesWindow_Scroll(PyCursesWindowObject *self, PyObject *args)
{
  int nlines;
  switch(PyTuple_Size(args)) {
  case 0:
    return PyCursesCheckERR(scroll(self->win), "scroll");
  case 1:
    if (!PyArg_ParseTuple(args, "i;nlines", &nlines))
      return NULL;
    return PyCursesCheckERR(wscrl(self->win, nlines), "scroll");
  default:
    PyErr_SetString(PyExc_TypeError, "scroll requires 0 or 1 arguments");
    return NULL;
  }
}

static PyObject *
PyCursesWindow_TouchLine(PyCursesWindowObject *self, PyObject *args)
{
  int st, cnt, val;
  switch (PyTuple_Size(args)) {
  case 2:
    if (!PyArg_ParseTuple(args,"ii;start,count",&st,&cnt))
      return NULL;
    return PyCursesCheckERR(touchline(self->win,st,cnt), "touchline");
  case 3:
    if (!PyArg_ParseTuple(args, "iii;start,count,val", &st, &cnt, &val))
      return NULL;
    return PyCursesCheckERR(wtouchln(self->win, st, cnt, val), "touchline");
  default:
    PyErr_SetString(PyExc_TypeError, "touchline requires 2 or 3 arguments");
    return NULL;
  }
}

static PyObject *
PyCursesWindow_Vline(PyCursesWindowObject *self, PyObject *args)
{
  PyObject *temp;
  chtype ch;
  int n, x, y, code = OK;
  attr_t attr = A_NORMAL;

  switch (PyTuple_Size(args)) {
  case 2:
    if (!PyArg_ParseTuple(args, "Oi;ch or int,n", &temp, &n))
      return NULL;
    break;
  case 3:
    if (!PyArg_ParseTuple(args, "Oil;ch or int,n,attr", &temp, &n, &attr))
      return NULL;
    break;
  case 4:
    if (!PyArg_ParseTuple(args, "iiOi;y,x,ch or int,n", &y, &x, &temp, &n))
      return NULL;
    code = wmove(self->win, y, x);
    break;
  case 5:
    if (!PyArg_ParseTuple(args, "iiOil; y,x,ch or int,n,attr", 
		     &y, &x, &temp, &n, &attr))
      return NULL;
    code = wmove(self->win, y, x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "vline requires 2 to 5 arguments");
    return NULL;
  }

  if (code != ERR) {
    if (!PyCurses_ConvertToChtype(temp, &ch)) {
      PyErr_SetString(PyExc_TypeError, 
		      "argument 1 or 3 must be a ch or an int");
      return NULL;
    }
    return PyCursesCheckERR(wvline(self->win, ch | attr, n), "vline");
  } else
    return PyCursesCheckERR(code, "wmove");
}

static PyMethodDef PyCursesWindow_Methods[] = {
	{"addch",           (PyCFunction)PyCursesWindow_AddCh, METH_VARARGS},
	{"addnstr",         (PyCFunction)PyCursesWindow_AddNStr, METH_VARARGS},
	{"addstr",          (PyCFunction)PyCursesWindow_AddStr, METH_VARARGS},
	{"attroff",         (PyCFunction)PyCursesWindow_wattroff, METH_VARARGS},
	{"attron",          (PyCFunction)PyCursesWindow_wattron, METH_VARARGS},
	{"attrset",         (PyCFunction)PyCursesWindow_wattrset, METH_VARARGS},
	{"bkgd",            (PyCFunction)PyCursesWindow_Bkgd, METH_VARARGS},
	{"chgat",           (PyCFunction)PyCursesWindow_ChgAt, METH_VARARGS},
	{"bkgdset",         (PyCFunction)PyCursesWindow_BkgdSet, METH_VARARGS},
	{"border",          (PyCFunction)PyCursesWindow_Border, METH_VARARGS},
	{"box",             (PyCFunction)PyCursesWindow_Box, METH_VARARGS},
	{"clear",           (PyCFunction)PyCursesWindow_wclear, METH_NOARGS},
	{"clearok",         (PyCFunction)PyCursesWindow_clearok, METH_VARARGS},
	{"clrtobot",        (PyCFunction)PyCursesWindow_wclrtobot, METH_NOARGS},
	{"clrtoeol",        (PyCFunction)PyCursesWindow_wclrtoeol, METH_NOARGS},
	{"cursyncup",       (PyCFunction)PyCursesWindow_wcursyncup, METH_NOARGS},
	{"delch",           (PyCFunction)PyCursesWindow_DelCh, METH_VARARGS},
	{"deleteln",        (PyCFunction)PyCursesWindow_wdeleteln, METH_NOARGS},
	{"derwin",          (PyCFunction)PyCursesWindow_DerWin, METH_VARARGS},
	{"echochar",        (PyCFunction)PyCursesWindow_EchoChar, METH_VARARGS},
#ifdef NCURSES_MOUSE_VERSION
	{"enclose",         (PyCFunction)PyCursesWindow_Enclose, METH_VARARGS},
#endif
	{"erase",           (PyCFunction)PyCursesWindow_werase, METH_NOARGS},
	{"getbegyx",        (PyCFunction)PyCursesWindow_getbegyx, METH_NOARGS},
	{"getbkgd",         (PyCFunction)PyCursesWindow_GetBkgd, METH_NOARGS},
	{"getch",           (PyCFunction)PyCursesWindow_GetCh, METH_VARARGS},
	{"getkey",          (PyCFunction)PyCursesWindow_GetKey, METH_VARARGS},
	{"getmaxyx",        (PyCFunction)PyCursesWindow_getmaxyx, METH_NOARGS},
	{"getparyx",        (PyCFunction)PyCursesWindow_getparyx, METH_NOARGS},
	{"getstr",          (PyCFunction)PyCursesWindow_GetStr, METH_VARARGS},
	{"getyx",           (PyCFunction)PyCursesWindow_getyx, METH_NOARGS},
	{"hline",           (PyCFunction)PyCursesWindow_Hline, METH_VARARGS},
	{"idcok",           (PyCFunction)PyCursesWindow_idcok, METH_VARARGS},
	{"idlok",           (PyCFunction)PyCursesWindow_idlok, METH_VARARGS},
	{"immedok",         (PyCFunction)PyCursesWindow_immedok, METH_VARARGS},
	{"inch",            (PyCFunction)PyCursesWindow_InCh, METH_VARARGS},
	{"insch",           (PyCFunction)PyCursesWindow_InsCh, METH_VARARGS},
	{"insdelln",        (PyCFunction)PyCursesWindow_winsdelln, METH_VARARGS},
	{"insertln",        (PyCFunction)PyCursesWindow_winsertln, METH_NOARGS},
	{"insnstr",         (PyCFunction)PyCursesWindow_InsNStr, METH_VARARGS},
	{"insstr",          (PyCFunction)PyCursesWindow_InsStr, METH_VARARGS},
	{"instr",           (PyCFunction)PyCursesWindow_InStr, METH_VARARGS},
	{"is_linetouched",  (PyCFunction)PyCursesWindow_Is_LineTouched, METH_VARARGS},
	{"is_wintouched",   (PyCFunction)PyCursesWindow_is_wintouched, METH_NOARGS},
	{"keypad",          (PyCFunction)PyCursesWindow_keypad, METH_VARARGS},
	{"leaveok",         (PyCFunction)PyCursesWindow_leaveok, METH_VARARGS},
	{"move",            (PyCFunction)PyCursesWindow_wmove, METH_VARARGS},
	{"mvderwin",        (PyCFunction)PyCursesWindow_mvderwin, METH_VARARGS},
	{"mvwin",           (PyCFunction)PyCursesWindow_mvwin, METH_VARARGS},
	{"nodelay",         (PyCFunction)PyCursesWindow_nodelay, METH_VARARGS},
	{"notimeout",       (PyCFunction)PyCursesWindow_notimeout, METH_VARARGS},
	{"noutrefresh",     (PyCFunction)PyCursesWindow_NoOutRefresh, METH_VARARGS},
        /* Backward compatibility alias -- remove in Python 2.3 */
	{"nooutrefresh",    (PyCFunction)PyCursesWindow_NoOutRefresh, METH_VARARGS},
	{"overlay",         (PyCFunction)PyCursesWindow_Overlay, METH_VARARGS},
	{"overwrite",       (PyCFunction)PyCursesWindow_Overwrite,
         METH_VARARGS},
	{"putwin",          (PyCFunction)PyCursesWindow_PutWin, METH_O},
	{"redrawln",        (PyCFunction)PyCursesWindow_RedrawLine, METH_VARARGS},
	{"redrawwin",       (PyCFunction)PyCursesWindow_redrawwin, METH_NOARGS},
	{"refresh",         (PyCFunction)PyCursesWindow_Refresh, METH_VARARGS},
#ifndef STRICT_SYSV_CURSES
	{"resize",          (PyCFunction)PyCursesWindow_wresize, METH_VARARGS},
#endif
	{"scroll",          (PyCFunction)PyCursesWindow_Scroll, METH_VARARGS},
	{"scrollok",        (PyCFunction)PyCursesWindow_scrollok, METH_VARARGS},
	{"setscrreg",       (PyCFunction)PyCursesWindow_SetScrollRegion, METH_VARARGS},
	{"standend",        (PyCFunction)PyCursesWindow_wstandend, METH_NOARGS},
	{"standout",        (PyCFunction)PyCursesWindow_wstandout, METH_NOARGS},
	{"subpad",          (PyCFunction)PyCursesWindow_SubWin, METH_VARARGS},
	{"subwin",          (PyCFunction)PyCursesWindow_SubWin, METH_VARARGS},
	{"syncdown",        (PyCFunction)PyCursesWindow_wsyncdown, METH_NOARGS},
	{"syncok",          (PyCFunction)PyCursesWindow_syncok, METH_VARARGS},
	{"syncup",          (PyCFunction)PyCursesWindow_wsyncup, METH_NOARGS},
	{"timeout",         (PyCFunction)PyCursesWindow_wtimeout, METH_VARARGS},
	{"touchline",       (PyCFunction)PyCursesWindow_TouchLine, METH_VARARGS},
	{"touchwin",        (PyCFunction)PyCursesWindow_touchwin, METH_NOARGS},
	{"untouchwin",      (PyCFunction)PyCursesWindow_untouchwin, METH_NOARGS},
	{"vline",           (PyCFunction)PyCursesWindow_Vline, METH_VARARGS},
	{NULL,		    NULL}   /* sentinel */
};

static PyObject *
PyCursesWindow_GetAttr(PyCursesWindowObject *self, char *name)
{
  return Py_FindMethod(PyCursesWindow_Methods, (PyObject *)self, name);
}

/* -------------------------------------------------------*/

PyTypeObject PyCursesWindow_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_curses.curses window",	/*tp_name*/
	sizeof(PyCursesWindowObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)PyCursesWindow_Dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)PyCursesWindow_GetAttr, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

/*********************************************************************
 Global Functions
**********************************************************************/

NoArgNoReturnFunction(beep)
NoArgNoReturnFunction(def_prog_mode)
NoArgNoReturnFunction(def_shell_mode)
NoArgNoReturnFunction(doupdate)
NoArgNoReturnFunction(endwin)
NoArgNoReturnFunction(flash)
NoArgNoReturnFunction(nocbreak)
NoArgNoReturnFunction(noecho)
NoArgNoReturnFunction(nonl)
NoArgNoReturnFunction(noraw)
NoArgNoReturnFunction(reset_prog_mode)
NoArgNoReturnFunction(reset_shell_mode)
NoArgNoReturnFunction(resetty)
NoArgNoReturnFunction(savetty)

NoArgOrFlagNoReturnFunction(cbreak)
NoArgOrFlagNoReturnFunction(echo)
NoArgOrFlagNoReturnFunction(nl)
NoArgOrFlagNoReturnFunction(raw)

NoArgReturnIntFunction(baudrate)
NoArgReturnIntFunction(termattrs)

NoArgReturnStringFunction(termname)
NoArgReturnStringFunction(longname)

NoArgTrueFalseFunction(can_change_color)
NoArgTrueFalseFunction(has_colors)
NoArgTrueFalseFunction(has_ic)
NoArgTrueFalseFunction(has_il)
NoArgTrueFalseFunction(isendwin)
NoArgNoReturnVoidFunction(filter)
NoArgNoReturnVoidFunction(flushinp)
NoArgNoReturnVoidFunction(noqiflush)

static PyObject *
PyCurses_Color_Content(PyObject *self, PyObject *args)
{
  short color,r,g,b;

  PyCursesInitialised
  PyCursesInitialisedColor

  if (!PyArg_ParseTuple(args, "h:color_content", &color)) return NULL;

  if (color_content(color, &r, &g, &b) != ERR)
    return Py_BuildValue("(iii)", r, g, b);
  else {
    PyErr_SetString(PyCursesError, 
		    "Argument 1 was out of range. Check value of COLORS.");
    return NULL;
  }
}

static PyObject *
PyCurses_color_pair(PyObject *self, PyObject *args)
{
  int n;

  PyCursesInitialised
  PyCursesInitialisedColor

  if (!PyArg_ParseTuple(args, "i:color_pair", &n)) return NULL;
  return PyInt_FromLong((long) (n << 8));
}

static PyObject *
PyCurses_Curs_Set(PyObject *self, PyObject *args)
{
  int vis,erg;

  PyCursesInitialised

  if (!PyArg_ParseTuple(args, "i:curs_set", &vis)) return NULL;

  erg = curs_set(vis);
  if (erg == ERR) return PyCursesCheckERR(erg, "curs_set");

  return PyInt_FromLong((long) erg);
}

static PyObject *
PyCurses_Delay_Output(PyObject *self, PyObject *args)
{
  int ms;

  PyCursesInitialised

  if (!PyArg_ParseTuple(args, "i:delay_output", &ms)) return NULL;

  return PyCursesCheckERR(delay_output(ms), "delay_output");
}

static PyObject *
PyCurses_EraseChar(PyObject *self)
{
  char ch;

  PyCursesInitialised

  ch = erasechar();

  return PyString_FromStringAndSize(&ch, 1);
}

static PyObject *
PyCurses_getsyx(PyObject *self)
{
  int x,y;

  PyCursesInitialised

  getsyx(y, x);

  return Py_BuildValue("(ii)", y, x);
}

#ifdef NCURSES_MOUSE_VERSION
static PyObject *
PyCurses_GetMouse(PyObject *self)
{
	int rtn;
	MEVENT event;

	PyCursesInitialised

	rtn = getmouse( &event );
	if (rtn == ERR) {
		PyErr_SetString(PyCursesError, "getmouse() returned ERR");
		return NULL;
	}
	return Py_BuildValue("(hiiil)", 
			     (short)event.id, 
			     event.x, event.y, event.z,
			     (long) event.bstate);
}

static PyObject *
PyCurses_UngetMouse(PyObject *self, PyObject *args)
{
	MEVENT event;

	PyCursesInitialised
	if (!PyArg_ParseTuple(args, "hiiil",
			     &event.id, 
			     &event.x, &event.y, &event.z,
			     (int *) &event.bstate))
	  return NULL;

	return PyCursesCheckERR(ungetmouse(&event), "ungetmouse");
}
#endif

static PyObject *
PyCurses_GetWin(PyCursesWindowObject *self, PyObject *stream)
{
  char fn[100];
  int fd;
  FILE *fp;
  PyObject *data;
  WINDOW *win;

  PyCursesInitialised

  strcpy(fn, "/tmp/py.curses.getwin.XXXXXX");
  fd = mkstemp(fn);
  if (fd < 0)
    return PyErr_SetFromErrnoWithFilename(PyExc_IOError, fn);
  fp = fdopen(fd, "wb+");
  if (fp == NULL) {
    close(fd);
    return PyErr_SetFromErrnoWithFilename(PyExc_IOError, fn);
  }
  data = PyObject_CallMethod(stream, "read", "");
  if (data == NULL) {
    fclose(fp);
    return NULL;
  }
  if (!PyBytes_Check(data)) {
    PyErr_Format(PyExc_TypeError,
                 "f.read() returned %.100s instead of bytes",
                 data->ob_type->tp_name);
    Py_DECREF(data);
    fclose(fp);
    return NULL;
  }
  fwrite(PyBytes_AS_STRING(data), 1, PyBytes_GET_SIZE(data), fp);
  Py_DECREF(data);
  fseek(fp, 0, 0);
  win = getwin(fp);
  fclose(fp);
  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }
  return PyCursesWindow_New(win);
}

static PyObject *
PyCurses_HalfDelay(PyObject *self, PyObject *args)
{
  unsigned char tenths;

  PyCursesInitialised

  if (!PyArg_ParseTuple(args, "b:halfdelay", &tenths)) return NULL;

  return PyCursesCheckERR(halfdelay(tenths), "halfdelay");
}

#ifndef STRICT_SYSV_CURSES
 /* No has_key! */
static PyObject * PyCurses_has_key(PyObject *self, PyObject *args)
{
  int ch;

  PyCursesInitialised

  if (!PyArg_ParseTuple(args,"i",&ch)) return NULL;

  if (has_key(ch) == FALSE) {
    Py_INCREF(Py_False);
    return Py_False;
  }
  Py_INCREF(Py_True);
  return Py_True; 
}
#endif /* STRICT_SYSV_CURSES */

static PyObject *
PyCurses_Init_Color(PyObject *self, PyObject *args)
{
  short color, r, g, b;

  PyCursesInitialised
  PyCursesInitialisedColor

  switch(PyTuple_Size(args)) {
  case 4:
    if (!PyArg_ParseTuple(args, "hhhh;color,r,g,b", &color, &r, &g, &b)) return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "init_color requires 4 arguments");
    return NULL;
  }

  return PyCursesCheckERR(init_color(color, r, g, b), "init_color");
}

static PyObject *
PyCurses_Init_Pair(PyObject *self, PyObject *args)
{
  short pair, f, b;

  PyCursesInitialised
  PyCursesInitialisedColor

  if (PyTuple_Size(args) != 3) {
    PyErr_SetString(PyExc_TypeError, "init_pair requires 3 arguments");
    return NULL;
  }

  if (!PyArg_ParseTuple(args, "hhh;pair, f, b", &pair, &f, &b)) return NULL;

  return PyCursesCheckERR(init_pair(pair, f, b), "init_pair");
}

static PyObject *ModDict;

static PyObject * 
PyCurses_InitScr(PyObject *self)
{
  WINDOW *win;

  if (initialised == TRUE) {
    wrefresh(stdscr);
    return (PyObject *)PyCursesWindow_New(stdscr);
  }

  win = initscr();

  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }

  initialised = initialised_setupterm = TRUE;

/* This was moved from initcurses() because it core dumped on SGI,
   where they're not defined until you've called initscr() */
#define SetDictInt(string,ch) \
    do {							\
	PyObject *o = PyInt_FromLong((long) (ch));		\
	if (o && PyDict_SetItemString(ModDict, string, o) == 0)	{ \
	    Py_DECREF(o);					\
	}							\
    } while (0)

	/* Here are some graphic symbols you can use */
        SetDictInt("ACS_ULCORNER",      (ACS_ULCORNER));
	SetDictInt("ACS_LLCORNER",      (ACS_LLCORNER));
	SetDictInt("ACS_URCORNER",      (ACS_URCORNER));
	SetDictInt("ACS_LRCORNER",      (ACS_LRCORNER));
	SetDictInt("ACS_LTEE",          (ACS_LTEE));
	SetDictInt("ACS_RTEE",          (ACS_RTEE));
	SetDictInt("ACS_BTEE",          (ACS_BTEE));
	SetDictInt("ACS_TTEE",          (ACS_TTEE));
	SetDictInt("ACS_HLINE",         (ACS_HLINE));
	SetDictInt("ACS_VLINE",         (ACS_VLINE));
	SetDictInt("ACS_PLUS",          (ACS_PLUS));
#if !defined(__hpux) || defined(HAVE_NCURSES_H)
        /* On HP/UX 11, these are of type cchar_t, which is not an
           integral type. If this is a problem on more platforms, a
           configure test should be added to determine whether ACS_S1
           is of integral type. */
	SetDictInt("ACS_S1",            (ACS_S1));
	SetDictInt("ACS_S9",            (ACS_S9));
	SetDictInt("ACS_DIAMOND",       (ACS_DIAMOND));
	SetDictInt("ACS_CKBOARD",       (ACS_CKBOARD));
	SetDictInt("ACS_DEGREE",        (ACS_DEGREE));
	SetDictInt("ACS_PLMINUS",       (ACS_PLMINUS));
	SetDictInt("ACS_BULLET",        (ACS_BULLET));
	SetDictInt("ACS_LARROW",        (ACS_LARROW));
	SetDictInt("ACS_RARROW",        (ACS_RARROW));
	SetDictInt("ACS_DARROW",        (ACS_DARROW));
	SetDictInt("ACS_UARROW",        (ACS_UARROW));
	SetDictInt("ACS_BOARD",         (ACS_BOARD));
	SetDictInt("ACS_LANTERN",       (ACS_LANTERN));
	SetDictInt("ACS_BLOCK",         (ACS_BLOCK));
#endif
	SetDictInt("ACS_BSSB",          (ACS_ULCORNER));
	SetDictInt("ACS_SSBB",          (ACS_LLCORNER));
	SetDictInt("ACS_BBSS",          (ACS_URCORNER));
	SetDictInt("ACS_SBBS",          (ACS_LRCORNER));
	SetDictInt("ACS_SBSS",          (ACS_RTEE));
	SetDictInt("ACS_SSSB",          (ACS_LTEE));
	SetDictInt("ACS_SSBS",          (ACS_BTEE));
	SetDictInt("ACS_BSSS",          (ACS_TTEE));
	SetDictInt("ACS_BSBS",          (ACS_HLINE));
	SetDictInt("ACS_SBSB",          (ACS_VLINE));
	SetDictInt("ACS_SSSS",          (ACS_PLUS));

	/* The following are never available with strict SYSV curses */
#ifdef ACS_S3
	SetDictInt("ACS_S3",            (ACS_S3));
#endif
#ifdef ACS_S7
	SetDictInt("ACS_S7",            (ACS_S7));
#endif
#ifdef ACS_LEQUAL
	SetDictInt("ACS_LEQUAL",        (ACS_LEQUAL));
#endif
#ifdef ACS_GEQUAL
	SetDictInt("ACS_GEQUAL",        (ACS_GEQUAL));
#endif
#ifdef ACS_PI
	SetDictInt("ACS_PI",            (ACS_PI));
#endif
#ifdef ACS_NEQUAL
	SetDictInt("ACS_NEQUAL",        (ACS_NEQUAL));
#endif
#ifdef ACS_STERLING
	SetDictInt("ACS_STERLING",      (ACS_STERLING));
#endif

  SetDictInt("LINES", LINES);
  SetDictInt("COLS", COLS);

  return (PyObject *)PyCursesWindow_New(win);
}

static PyObject *
PyCurses_setupterm(PyObject* self, PyObject *args, PyObject* keywds)
{
	int fd = -1;
	int err;
	char* termstr = NULL;

	static char *kwlist[] = {"term", "fd", NULL};

	if (!PyArg_ParseTupleAndKeywords(
		args, keywds, "|zi:setupterm", kwlist, &termstr, &fd)) {
		return NULL;
	}
	
	if (fd == -1) {
		PyObject* sys_stdout;

		sys_stdout = PySys_GetObject("stdout");

		if (sys_stdout == NULL) {
			PyErr_SetString(
				PyCursesError,
				"lost sys.stdout");
			return NULL;
		}

		fd = PyObject_AsFileDescriptor(sys_stdout);

		if (fd == -1) {
			return NULL;
		}
	}

	if (setupterm(termstr,fd,&err) == ERR) {
		char* s = "setupterm: unknown error";
		
		if (err == 0) {
			s = "setupterm: could not find terminal";
		} else if (err == -1) {
			s = "setupterm: could not find terminfo database";
		}

		PyErr_SetString(PyCursesError,s);
		return NULL;
	}

	initialised_setupterm = TRUE;

	Py_INCREF(Py_None);
	return Py_None;	
}

static PyObject *
PyCurses_IntrFlush(PyObject *self, PyObject *args)
{
  int ch;

  PyCursesInitialised

  switch(PyTuple_Size(args)) {
  case 1:
    if (!PyArg_ParseTuple(args,"i;True(1), False(0)",&ch)) return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "intrflush requires 1 argument");
    return NULL;
  }

  return PyCursesCheckERR(intrflush(NULL,ch), "intrflush");
}

#ifdef HAVE_CURSES_IS_TERM_RESIZED
static PyObject *
PyCurses_Is_Term_Resized(PyObject *self, PyObject *args)
{
  int lines;
  int columns;
  int result;

  PyCursesInitialised

  if (!PyArg_ParseTuple(args,"ii:is_term_resized", &lines, &columns))
    return NULL;
  result = is_term_resized(lines, columns);
  if (result == TRUE) {
    Py_INCREF(Py_True);
    return Py_True;
  } else {
    Py_INCREF(Py_False);
    return Py_False;
  }
}
#endif /* HAVE_CURSES_IS_TERM_RESIZED */

#if !defined(__NetBSD__)
static PyObject *
PyCurses_KeyName(PyObject *self, PyObject *args)
{
  const char *knp;
  int ch;

  PyCursesInitialised

  if (!PyArg_ParseTuple(args,"i",&ch)) return NULL;

  if (ch < 0) {
    PyErr_SetString(PyExc_ValueError, "invalid key number");
    return NULL;
  }
  knp = keyname(ch);

  return PyString_FromString((knp == NULL) ? "" : (char *)knp);
}
#endif

static PyObject *  
PyCurses_KillChar(PyObject *self)  
{  
  char ch;  

  ch = killchar();  

  return PyString_FromStringAndSize(&ch, 1);  
}  

static PyObject *
PyCurses_Meta(PyObject *self, PyObject *args)
{
  int ch;

  PyCursesInitialised

  switch(PyTuple_Size(args)) {
  case 1:
    if (!PyArg_ParseTuple(args,"i;True(1), False(0)",&ch)) return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "meta requires 1 argument");
    return NULL;
  }

  return PyCursesCheckERR(meta(stdscr, ch), "meta");
}

#ifdef NCURSES_MOUSE_VERSION
static PyObject *
PyCurses_MouseInterval(PyObject *self, PyObject *args)
{
	int interval;
	PyCursesInitialised 

	if (!PyArg_ParseTuple(args,"i;interval",&interval)) 
		return NULL;
	return PyCursesCheckERR(mouseinterval(interval), "mouseinterval");
}

static PyObject *
PyCurses_MouseMask(PyObject *self, PyObject *args)
{
	int newmask;
	mmask_t oldmask, availmask;

	PyCursesInitialised 
	if (!PyArg_ParseTuple(args,"i;mousemask",&newmask)) 
		return NULL;
	availmask = mousemask(newmask, &oldmask);
	return Py_BuildValue("(ll)", (long)availmask, (long)oldmask);
}
#endif

static PyObject *
PyCurses_Napms(PyObject *self, PyObject *args)
{
    int ms;

    PyCursesInitialised
    if (!PyArg_ParseTuple(args, "i;ms", &ms)) return NULL;

    return Py_BuildValue("i", napms(ms));
}


static PyObject *
PyCurses_NewPad(PyObject *self, PyObject *args)
{
  WINDOW *win;
  int nlines, ncols;

  PyCursesInitialised 

  if (!PyArg_ParseTuple(args,"ii;nlines,ncols",&nlines,&ncols)) return NULL;

  win = newpad(nlines, ncols);
  
  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }

  return (PyObject *)PyCursesWindow_New(win);
}

static PyObject *
PyCurses_NewWindow(PyObject *self, PyObject *args)
{
  WINDOW *win;
  int nlines, ncols, begin_y=0, begin_x=0;

  PyCursesInitialised

  switch (PyTuple_Size(args)) {
  case 2:
    if (!PyArg_ParseTuple(args,"ii;nlines,ncols",&nlines,&ncols))
      return NULL;
    break;
  case 4:
    if (!PyArg_ParseTuple(args, "iiii;nlines,ncols,begin_y,begin_x",
		   &nlines,&ncols,&begin_y,&begin_x))
      return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "newwin requires 2 or 4 arguments");
    return NULL;
  }

  win = newwin(nlines,ncols,begin_y,begin_x);
  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }

  return (PyObject *)PyCursesWindow_New(win);
}

static PyObject *
PyCurses_Pair_Content(PyObject *self, PyObject *args)
{
  short pair,f,b;

  PyCursesInitialised
  PyCursesInitialisedColor

  switch(PyTuple_Size(args)) {
  case 1:
    if (!PyArg_ParseTuple(args, "h;pair", &pair)) return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "pair_content requires 1 argument");
    return NULL;
  }

  if (pair_content(pair, &f, &b)==ERR) {
    PyErr_SetString(PyCursesError,
		    "Argument 1 was out of range. (1..COLOR_PAIRS-1)");
    return NULL;
  }

  return Py_BuildValue("(ii)", f, b);
}

static PyObject *
PyCurses_pair_number(PyObject *self, PyObject *args)
{
  int n;

  PyCursesInitialised
  PyCursesInitialisedColor

  switch(PyTuple_Size(args)) {
  case 1:
    if (!PyArg_ParseTuple(args, "i;pairvalue", &n)) return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError,
                    "pair_number requires 1 argument");
    return NULL;
  }

  return PyInt_FromLong((long) ((n & A_COLOR) >> 8));
}

static PyObject *
PyCurses_Putp(PyObject *self, PyObject *args)
{
  char *str;

  if (!PyArg_ParseTuple(args,"s;str", &str)) return NULL;
  return PyCursesCheckERR(putp(str), "putp");
}

static PyObject *
PyCurses_QiFlush(PyObject *self, PyObject *args)
{
  int flag = 0;

  PyCursesInitialised

  switch(PyTuple_Size(args)) {
  case 0:
    qiflush();
    Py_INCREF(Py_None);
    return Py_None;
  case 1:
    if (!PyArg_ParseTuple(args, "i;True(1) or False(0)", &flag)) return NULL;
    if (flag) qiflush();
    else noqiflush();
    Py_INCREF(Py_None);
    return Py_None;
  default:
    PyErr_SetString(PyExc_TypeError, "qiflush requires 0 or 1 arguments");
    return NULL;
  }
}

/* Internal helper used for updating curses.LINES, curses.COLS, _curses.LINES
 * and _curses.COLS */
static int
update_lines_cols(void)
{
  PyObject *o;
  PyObject *m = PyImport_ImportModule("curses");

  if (!m)
    return 0;

  o = PyInt_FromLong(LINES);
  if (!o) {
    Py_DECREF(m);
    return 0;
  }
  if (PyObject_SetAttrString(m, "LINES", o)) {
    Py_DECREF(m);
    Py_DECREF(o);
    return 0;
  }
  if (PyDict_SetItemString(ModDict, "LINES", o)) {
    Py_DECREF(m);
    Py_DECREF(o);
    return 0;
  }
  Py_DECREF(o);
  o = PyInt_FromLong(COLS);
  if (!o) {
    Py_DECREF(m);
    return 0;
  }
  if (PyObject_SetAttrString(m, "COLS", o)) {
    Py_DECREF(m);
    Py_DECREF(o);
    return 0;
  }
  if (PyDict_SetItemString(ModDict, "COLS", o)) {
    Py_DECREF(m);
    Py_DECREF(o);
    return 0;
  }
  Py_DECREF(o);
  Py_DECREF(m);
  return 1;
}

#ifdef HAVE_CURSES_RESIZETERM
static PyObject *
PyCurses_ResizeTerm(PyObject *self, PyObject *args)
{
  int lines;
  int columns;
  PyObject *result;

  PyCursesInitialised

  if (!PyArg_ParseTuple(args,"ii:resizeterm", &lines, &columns))
    return NULL;

  result = PyCursesCheckERR(resizeterm(lines, columns), "resizeterm");
  if (!result)
    return NULL;
  if (!update_lines_cols())
    return NULL;
  return result;
}

#endif

#ifdef HAVE_CURSES_RESIZE_TERM
static PyObject *
PyCurses_Resize_Term(PyObject *self, PyObject *args)
{
  int lines;
  int columns;

  PyObject *result;

  PyCursesInitialised

  if (!PyArg_ParseTuple(args,"ii:resize_term", &lines, &columns))
    return NULL;

  result = PyCursesCheckERR(resize_term(lines, columns), "resize_term");
  if (!result)
    return NULL;
  if (!update_lines_cols())
    return NULL;
  return result;
}
#endif /* HAVE_CURSES_RESIZE_TERM */

static PyObject *
PyCurses_setsyx(PyObject *self, PyObject *args)
{
  int y,x;

  PyCursesInitialised

  if (PyTuple_Size(args)!=2) {
    PyErr_SetString(PyExc_TypeError, "setsyx requires 2 arguments");
    return NULL;
  }

  if (!PyArg_ParseTuple(args, "ii;y, x", &y, &x)) return NULL;

  setsyx(y,x);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCurses_Start_Color(PyObject *self)
{
  int code;
  PyObject *c, *cp;

  PyCursesInitialised

  code = start_color();
  if (code != ERR) {
    initialisedcolors = TRUE;
    c = PyInt_FromLong((long) COLORS);
    PyDict_SetItemString(ModDict, "COLORS", c);
    Py_DECREF(c);
    cp = PyInt_FromLong((long) COLOR_PAIRS);
    PyDict_SetItemString(ModDict, "COLOR_PAIRS", cp);
    Py_DECREF(cp);
    Py_INCREF(Py_None);
    return Py_None;
  } else {
    PyErr_SetString(PyCursesError, "start_color() returned ERR");
    return NULL;
  }
}

static PyObject *
PyCurses_tigetflag(PyObject *self, PyObject *args)
{
	char *capname;

	PyCursesSetupTermCalled;
		
	if (!PyArg_ParseTuple(args, "z", &capname))
		return NULL;

	return PyInt_FromLong( (long) tigetflag( capname ) );
}

static PyObject *
PyCurses_tigetnum(PyObject *self, PyObject *args)
{
	char *capname;

	PyCursesSetupTermCalled;
		
	if (!PyArg_ParseTuple(args, "z", &capname))
		return NULL;

	return PyInt_FromLong( (long) tigetnum( capname ) );
}

static PyObject *
PyCurses_tigetstr(PyObject *self, PyObject *args)
{
	char *capname;

	PyCursesSetupTermCalled;
		
	if (!PyArg_ParseTuple(args, "z", &capname))
		return NULL;

	capname = tigetstr( capname );
	if (capname == 0 || capname == (char*) -1) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString( capname );
}

static PyObject *
PyCurses_tparm(PyObject *self, PyObject *args)
{
	char* fmt;
	char* result = NULL;
	int i1=0,i2=0,i3=0,i4=0,i5=0,i6=0,i7=0,i8=0,i9=0;

	PyCursesSetupTermCalled;

	if (!PyArg_ParseTuple(args, "s|iiiiiiiii:tparm", 
			      &fmt, &i1, &i2, &i3, &i4, 
			      &i5, &i6, &i7, &i8, &i9)) {
		return NULL;
	}

	result = tparm(fmt,i1,i2,i3,i4,i5,i6,i7,i8,i9);
	if (!result) {
		PyErr_SetString(PyCursesError, "tparm() returned NULL");
  		return NULL;
	}

	return PyString_FromString(result);
}

static PyObject *
PyCurses_TypeAhead(PyObject *self, PyObject *args)
{
  int fd;

  PyCursesInitialised

  if (!PyArg_ParseTuple(args,"i;fd",&fd)) return NULL;

  return PyCursesCheckERR(typeahead( fd ), "typeahead");
}

static PyObject *
PyCurses_UnCtrl(PyObject *self, PyObject *args)
{
  PyObject *temp;
  chtype ch;

  PyCursesInitialised

  if (!PyArg_ParseTuple(args,"O;ch or int",&temp)) return NULL;

  if (!PyCurses_ConvertToChtype(temp, &ch)) {
    PyErr_SetString(PyExc_TypeError, "argument must be a ch or an int");
    return NULL;
  }

  return PyString_FromString(unctrl(ch));
}

static PyObject *
PyCurses_UngetCh(PyObject *self, PyObject *args)
{
  PyObject *temp;
  chtype ch;

  PyCursesInitialised

  if (!PyArg_ParseTuple(args,"O;ch or int",&temp)) return NULL;

  if (!PyCurses_ConvertToChtype(temp, &ch)) {
    PyErr_SetString(PyExc_TypeError, "argument must be a ch or an int");
    return NULL;
  }

  return PyCursesCheckERR(ungetch(ch), "ungetch");
}

static PyObject *
PyCurses_Use_Env(PyObject *self, PyObject *args)
{
  int flag;

  PyCursesInitialised

  switch(PyTuple_Size(args)) {
  case 1:
    if (!PyArg_ParseTuple(args,"i;True(1), False(0)",&flag))
      return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "use_env requires 1 argument");
    return NULL;
  }
  use_env(flag);
  Py_INCREF(Py_None);
  return Py_None;
}

#ifndef STRICT_SYSV_CURSES
static PyObject *
PyCurses_Use_Default_Colors(PyObject *self)
{
  int code;

  PyCursesInitialised
  PyCursesInitialisedColor

  code = use_default_colors();
  if (code != ERR) {
    Py_INCREF(Py_None);
    return Py_None;
  } else {
    PyErr_SetString(PyCursesError, "use_default_colors() returned ERR");
    return NULL;
  }
}
#endif /* STRICT_SYSV_CURSES */

/* List of functions defined in the module */

static PyMethodDef PyCurses_methods[] = {
  {"baudrate",            (PyCFunction)PyCurses_baudrate, METH_NOARGS},
  {"beep",                (PyCFunction)PyCurses_beep, METH_NOARGS},
  {"can_change_color",    (PyCFunction)PyCurses_can_change_color, METH_NOARGS},
  {"cbreak",              (PyCFunction)PyCurses_cbreak, METH_VARARGS},
  {"color_content",       (PyCFunction)PyCurses_Color_Content, METH_VARARGS},
  {"color_pair",          (PyCFunction)PyCurses_color_pair, METH_VARARGS},
  {"curs_set",            (PyCFunction)PyCurses_Curs_Set, METH_VARARGS},
  {"def_prog_mode",       (PyCFunction)PyCurses_def_prog_mode, METH_NOARGS},
  {"def_shell_mode",      (PyCFunction)PyCurses_def_shell_mode, METH_NOARGS},
  {"delay_output",        (PyCFunction)PyCurses_Delay_Output, METH_VARARGS},
  {"doupdate",            (PyCFunction)PyCurses_doupdate, METH_NOARGS},
  {"echo",                (PyCFunction)PyCurses_echo, METH_VARARGS},
  {"endwin",              (PyCFunction)PyCurses_endwin, METH_NOARGS},
  {"erasechar",           (PyCFunction)PyCurses_EraseChar, METH_NOARGS},
  {"filter",              (PyCFunction)PyCurses_filter, METH_NOARGS},
  {"flash",               (PyCFunction)PyCurses_flash, METH_NOARGS},
  {"flushinp",            (PyCFunction)PyCurses_flushinp, METH_NOARGS},
#ifdef NCURSES_MOUSE_VERSION
  {"getmouse",            (PyCFunction)PyCurses_GetMouse, METH_NOARGS},
  {"ungetmouse",          (PyCFunction)PyCurses_UngetMouse, METH_VARARGS},
#endif
  {"getsyx",              (PyCFunction)PyCurses_getsyx, METH_NOARGS},
  {"getwin",              (PyCFunction)PyCurses_GetWin, METH_O},
  {"has_colors",          (PyCFunction)PyCurses_has_colors, METH_NOARGS},
  {"has_ic",              (PyCFunction)PyCurses_has_ic, METH_NOARGS},
  {"has_il",              (PyCFunction)PyCurses_has_il, METH_NOARGS},
#ifndef STRICT_SYSV_CURSES
  {"has_key",             (PyCFunction)PyCurses_has_key, METH_VARARGS},
#endif
  {"halfdelay",           (PyCFunction)PyCurses_HalfDelay, METH_VARARGS},
  {"init_color",          (PyCFunction)PyCurses_Init_Color, METH_VARARGS},
  {"init_pair",           (PyCFunction)PyCurses_Init_Pair, METH_VARARGS},
  {"initscr",             (PyCFunction)PyCurses_InitScr, METH_NOARGS},
  {"intrflush",           (PyCFunction)PyCurses_IntrFlush, METH_VARARGS},
  {"isendwin",            (PyCFunction)PyCurses_isendwin, METH_NOARGS},
#ifdef HAVE_CURSES_IS_TERM_RESIZED
  {"is_term_resized",     (PyCFunction)PyCurses_Is_Term_Resized, METH_VARARGS},
#endif
#if !defined(__NetBSD__)
  {"keyname",             (PyCFunction)PyCurses_KeyName, METH_VARARGS},
#endif
  {"killchar",            (PyCFunction)PyCurses_KillChar, METH_NOARGS}, 
  {"longname",            (PyCFunction)PyCurses_longname, METH_NOARGS}, 
  {"meta",                (PyCFunction)PyCurses_Meta, METH_VARARGS},
#ifdef NCURSES_MOUSE_VERSION
  {"mouseinterval",       (PyCFunction)PyCurses_MouseInterval, METH_VARARGS},
  {"mousemask",           (PyCFunction)PyCurses_MouseMask, METH_VARARGS},
#endif
  {"napms",               (PyCFunction)PyCurses_Napms, METH_VARARGS},
  {"newpad",              (PyCFunction)PyCurses_NewPad, METH_VARARGS},
  {"newwin",              (PyCFunction)PyCurses_NewWindow, METH_VARARGS},
  {"nl",                  (PyCFunction)PyCurses_nl, METH_VARARGS},
  {"nocbreak",            (PyCFunction)PyCurses_nocbreak, METH_NOARGS},
  {"noecho",              (PyCFunction)PyCurses_noecho, METH_NOARGS},
  {"nonl",                (PyCFunction)PyCurses_nonl, METH_NOARGS},
  {"noqiflush",           (PyCFunction)PyCurses_noqiflush, METH_NOARGS},
  {"noraw",               (PyCFunction)PyCurses_noraw, METH_NOARGS},
  {"pair_content",        (PyCFunction)PyCurses_Pair_Content, METH_VARARGS},
  {"pair_number",         (PyCFunction)PyCurses_pair_number, METH_VARARGS},
  {"putp",                (PyCFunction)PyCurses_Putp, METH_VARARGS},
  {"qiflush",             (PyCFunction)PyCurses_QiFlush, METH_VARARGS},
  {"raw",                 (PyCFunction)PyCurses_raw, METH_VARARGS},
  {"reset_prog_mode",     (PyCFunction)PyCurses_reset_prog_mode, METH_NOARGS},
  {"reset_shell_mode",    (PyCFunction)PyCurses_reset_shell_mode, METH_NOARGS},
  {"resetty",             (PyCFunction)PyCurses_resetty, METH_NOARGS},
#ifdef HAVE_CURSES_RESIZETERM
  {"resizeterm",          (PyCFunction)PyCurses_ResizeTerm, METH_VARARGS},
#endif
#ifdef HAVE_CURSES_RESIZE_TERM
  {"resize_term",         (PyCFunction)PyCurses_Resize_Term, METH_VARARGS},
#endif
  {"savetty",             (PyCFunction)PyCurses_savetty, METH_NOARGS},
  {"setsyx",              (PyCFunction)PyCurses_setsyx, METH_VARARGS},
  {"setupterm",           (PyCFunction)PyCurses_setupterm,
   METH_VARARGS|METH_KEYWORDS},
  {"start_color",         (PyCFunction)PyCurses_Start_Color, METH_NOARGS},
  {"termattrs",           (PyCFunction)PyCurses_termattrs, METH_NOARGS},
  {"termname",            (PyCFunction)PyCurses_termname, METH_NOARGS},
  {"tigetflag",		  (PyCFunction)PyCurses_tigetflag, METH_VARARGS},
  {"tigetnum",		  (PyCFunction)PyCurses_tigetnum, METH_VARARGS},
  {"tigetstr",		  (PyCFunction)PyCurses_tigetstr, METH_VARARGS},
  {"tparm",               (PyCFunction)PyCurses_tparm, METH_VARARGS},
  {"typeahead",           (PyCFunction)PyCurses_TypeAhead, METH_VARARGS},
  {"unctrl",              (PyCFunction)PyCurses_UnCtrl, METH_VARARGS},
  {"ungetch",             (PyCFunction)PyCurses_UngetCh, METH_VARARGS},
  {"use_env",             (PyCFunction)PyCurses_Use_Env, METH_VARARGS},
#ifndef STRICT_SYSV_CURSES
  {"use_default_colors",  (PyCFunction)PyCurses_Use_Default_Colors, METH_NOARGS},
#endif
  {NULL,		  NULL}		/* sentinel */
};

/* Initialization function for the module */

PyMODINIT_FUNC
init_curses(void)
{
	PyObject *m, *d, *v, *c_api_object;
	static void *PyCurses_API[PyCurses_API_pointers];

	/* Initialize object type */
	Py_Type(&PyCursesWindow_Type) = &PyType_Type;

	/* Initialize the C API pointer array */
	PyCurses_API[0] = (void *)&PyCursesWindow_Type;
	PyCurses_API[1] = (void *)func_PyCursesSetupTermCalled;
	PyCurses_API[2] = (void *)func_PyCursesInitialised;
	PyCurses_API[3] = (void *)func_PyCursesInitialisedColor;

	/* Create the module and add the functions */
	m = Py_InitModule("_curses", PyCurses_methods);
	if (m == NULL)
    		return;

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	if (d == NULL)
		return;
	ModDict = d; /* For PyCurses_InitScr to use later */

	/* Add a CObject for the C API */
	c_api_object = PyCObject_FromVoidPtr((void *)PyCurses_API, NULL);
	PyDict_SetItemString(d, "_C_API", c_api_object);
	Py_DECREF(c_api_object);

	/* For exception curses.error */
	PyCursesError = PyErr_NewException("_curses.error", NULL, NULL);
	PyDict_SetItemString(d, "error", PyCursesError);

	/* Make the version available */
	v = PyString_FromString(PyCursesVersion);
	PyDict_SetItemString(d, "version", v);
	PyDict_SetItemString(d, "__version__", v);
	Py_DECREF(v);

        SetDictInt("ERR", ERR);
        SetDictInt("OK", OK);

	/* Here are some attributes you can add to chars to print */
	
	SetDictInt("A_ATTRIBUTES",      A_ATTRIBUTES);
	SetDictInt("A_NORMAL",		A_NORMAL);
	SetDictInt("A_STANDOUT",	A_STANDOUT);
	SetDictInt("A_UNDERLINE",	A_UNDERLINE);
	SetDictInt("A_REVERSE",		A_REVERSE);
	SetDictInt("A_BLINK",		A_BLINK);
	SetDictInt("A_DIM",		A_DIM);
	SetDictInt("A_BOLD",		A_BOLD);
	SetDictInt("A_ALTCHARSET",	A_ALTCHARSET);
#if !defined(__NetBSD__)
	SetDictInt("A_INVIS",           A_INVIS);
#endif
	SetDictInt("A_PROTECT",         A_PROTECT);
	SetDictInt("A_CHARTEXT",        A_CHARTEXT);
	SetDictInt("A_COLOR",           A_COLOR);

	/* The following are never available with strict SYSV curses */
#ifdef A_HORIZONTAL
	SetDictInt("A_HORIZONTAL",      A_HORIZONTAL);
#endif
#ifdef A_LEFT
	SetDictInt("A_LEFT",            A_LEFT);
#endif
#ifdef A_LOW
	SetDictInt("A_LOW",             A_LOW);
#endif
#ifdef A_RIGHT
	SetDictInt("A_RIGHT",           A_RIGHT);
#endif
#ifdef A_TOP
	SetDictInt("A_TOP",             A_TOP);
#endif
#ifdef A_VERTICAL
	SetDictInt("A_VERTICAL",        A_VERTICAL);
#endif

	SetDictInt("COLOR_BLACK",       COLOR_BLACK);
	SetDictInt("COLOR_RED",         COLOR_RED);
	SetDictInt("COLOR_GREEN",       COLOR_GREEN);
	SetDictInt("COLOR_YELLOW",      COLOR_YELLOW);
	SetDictInt("COLOR_BLUE",        COLOR_BLUE);
	SetDictInt("COLOR_MAGENTA",     COLOR_MAGENTA);
	SetDictInt("COLOR_CYAN",        COLOR_CYAN);
	SetDictInt("COLOR_WHITE",       COLOR_WHITE);

#ifdef NCURSES_MOUSE_VERSION
	/* Mouse-related constants */
	SetDictInt("BUTTON1_PRESSED",          BUTTON1_PRESSED);
	SetDictInt("BUTTON1_RELEASED",         BUTTON1_RELEASED);
	SetDictInt("BUTTON1_CLICKED",          BUTTON1_CLICKED);
	SetDictInt("BUTTON1_DOUBLE_CLICKED",   BUTTON1_DOUBLE_CLICKED);
	SetDictInt("BUTTON1_TRIPLE_CLICKED",   BUTTON1_TRIPLE_CLICKED);

	SetDictInt("BUTTON2_PRESSED",          BUTTON2_PRESSED);
	SetDictInt("BUTTON2_RELEASED",         BUTTON2_RELEASED);
	SetDictInt("BUTTON2_CLICKED",          BUTTON2_CLICKED);
	SetDictInt("BUTTON2_DOUBLE_CLICKED",   BUTTON2_DOUBLE_CLICKED);
	SetDictInt("BUTTON2_TRIPLE_CLICKED",   BUTTON2_TRIPLE_CLICKED);

	SetDictInt("BUTTON3_PRESSED",          BUTTON3_PRESSED);
	SetDictInt("BUTTON3_RELEASED",         BUTTON3_RELEASED);
	SetDictInt("BUTTON3_CLICKED",          BUTTON3_CLICKED);
	SetDictInt("BUTTON3_DOUBLE_CLICKED",   BUTTON3_DOUBLE_CLICKED);
	SetDictInt("BUTTON3_TRIPLE_CLICKED",   BUTTON3_TRIPLE_CLICKED);

	SetDictInt("BUTTON4_PRESSED",          BUTTON4_PRESSED);
	SetDictInt("BUTTON4_RELEASED",         BUTTON4_RELEASED);
	SetDictInt("BUTTON4_CLICKED",          BUTTON4_CLICKED);
	SetDictInt("BUTTON4_DOUBLE_CLICKED",   BUTTON4_DOUBLE_CLICKED);
	SetDictInt("BUTTON4_TRIPLE_CLICKED",   BUTTON4_TRIPLE_CLICKED);

	SetDictInt("BUTTON_SHIFT",             BUTTON_SHIFT);
	SetDictInt("BUTTON_CTRL",              BUTTON_CTRL);
	SetDictInt("BUTTON_ALT",               BUTTON_ALT);

	SetDictInt("ALL_MOUSE_EVENTS",         ALL_MOUSE_EVENTS);
	SetDictInt("REPORT_MOUSE_POSITION",    REPORT_MOUSE_POSITION);
#endif
	/* Now set everything up for KEY_ variables */
	{
	  int key;
	  char *key_n;
	  char *key_n2;
#if !defined(__NetBSD__)
	  for (key=KEY_MIN;key < KEY_MAX; key++) {
	    key_n = (char *)keyname(key);
	    if (key_n == NULL || strcmp(key_n,"UNKNOWN KEY")==0)
	      continue;
	    if (strncmp(key_n,"KEY_F(",6)==0) {
	      char *p1, *p2;
	      key_n2 = malloc(strlen(key_n)+1);
	      if (!key_n2) {
		PyErr_NoMemory();
		break;
              }
	      p1 = key_n;
	      p2 = key_n2;
	      while (*p1) {
		if (*p1 != '(' && *p1 != ')') {
		  *p2 = *p1;
		  p2++;
		}
		p1++;
	      }
	      *p2 = (char)0;
	    } else
	      key_n2 = key_n;
	    SetDictInt(key_n2,key);
	    if (key_n2 != key_n)
	      free(key_n2);
	  }
#endif
	  SetDictInt("KEY_MIN", KEY_MIN);
	  SetDictInt("KEY_MAX", KEY_MAX);
	}
}
