
/* Map C struct members to Python object attributes */

#include "Python.h"

#include "structmember.h"

PyObject *
PyMember_GetOne(const char *addr, PyMemberDef *l)
{
	PyObject *v;

	addr += l->offset;
	switch (l->type) {
	case T_BYTE:
		v = PyInt_FromLong(*(char*)addr);
		break;
	case T_UBYTE:
		v = PyLong_FromUnsignedLong(*(unsigned char*)addr);
		break;
	case T_SHORT:
		v = PyInt_FromLong(*(short*)addr);
		break;
	case T_USHORT:
		v = PyLong_FromUnsignedLong(*(unsigned short*)addr);
		break;
	case T_INT:
		v = PyInt_FromLong(*(int*)addr);
		break;
	case T_UINT:
		v = PyLong_FromUnsignedLong(*(unsigned int*)addr);
		break;
	case T_LONG:
		v = PyInt_FromLong(*(long*)addr);
		break;
	case T_ULONG:
		v = PyLong_FromUnsignedLong(*(unsigned long*)addr);
		break;
	case T_PYSSIZET:
		v = PyInt_FromSsize_t(*(Py_ssize_t*)addr);
		break;
	case T_FLOAT:
		v = PyFloat_FromDouble((double)*(float*)addr);
		break;
	case T_DOUBLE:
		v = PyFloat_FromDouble(*(double*)addr);
		break;
	case T_STRING:
		if (*(char**)addr == NULL) {
			Py_INCREF(Py_None);
			v = Py_None;
		}
		else
			v = PyString_FromString(*(char**)addr);
		break;
	case T_STRING_INPLACE:
		v = PyString_FromString((char*)addr);
		break;
	case T_CHAR:
		v = PyString_FromStringAndSize((char*)addr, 1);
		break;
	case T_OBJECT:
		v = *(PyObject **)addr;
		if (v == NULL)
			v = Py_None;
		Py_INCREF(v);
		break;
	case T_OBJECT_EX:
		v = *(PyObject **)addr;
		if (v == NULL)
			PyErr_SetString(PyExc_AttributeError, l->name);
		Py_XINCREF(v);
		break;
#ifdef HAVE_LONG_LONG
	case T_LONGLONG:
		v = PyLong_FromLongLong(*(PY_LONG_LONG *)addr);
		break;
	case T_ULONGLONG:
		v = PyLong_FromUnsignedLongLong(*(unsigned PY_LONG_LONG *)addr);
		break;
#endif /* HAVE_LONG_LONG */
        case T_NONE:
		v = Py_None;
		Py_INCREF(v);
		break;
	default:
		PyErr_SetString(PyExc_SystemError, "bad memberdescr type");
		v = NULL;
	}
	return v;
}

#define WARN(msg)					\
    do {						\
	if (PyErr_Warn(PyExc_RuntimeWarning, msg) < 0)	\
		return -1;				\
    } while (0)

int
PyMember_SetOne(char *addr, PyMemberDef *l, PyObject *v)
{
	PyObject *oldv;

	if ((l->flags & READONLY) || l->type == T_STRING)
	{
		PyErr_SetString(PyExc_AttributeError, "readonly attribute");
		return -1;
	}
	if (v == NULL && l->type != T_OBJECT_EX && l->type != T_OBJECT) {
		PyErr_SetString(PyExc_TypeError,
				"can't delete numeric/char attribute");
		return -1;
	}
	addr += l->offset;
	switch (l->type) {
	case T_BYTE:{
		long long_val = PyInt_AsLong(v);
		if ((long_val == -1) && PyErr_Occurred())
			return -1;
		*(char*)addr = (char)long_val;
		/* XXX: For compatibility, only warn about truncations
		   for now. */
		if ((long_val > CHAR_MAX) || (long_val < CHAR_MIN))
			WARN("Truncation of value to char");
		break;
		}
	case T_UBYTE:{
		long long_val = PyInt_AsLong(v);
		if ((long_val == -1) && PyErr_Occurred())
			return -1;
		*(unsigned char*)addr = (unsigned char)long_val;
		if ((long_val > UCHAR_MAX) || (long_val < 0))
			WARN("Truncation of value to unsigned char");
		break;
		}
	case T_SHORT:{
		long long_val = PyInt_AsLong(v);
		if ((long_val == -1) && PyErr_Occurred())
			return -1;
		*(short*)addr = (short)long_val;
		if ((long_val > SHRT_MAX) || (long_val < SHRT_MIN))
			WARN("Truncation of value to short");
		break;
		}
	case T_USHORT:{
		long long_val = PyInt_AsLong(v);
		if ((long_val == -1) && PyErr_Occurred())
			return -1;
		*(unsigned short*)addr = (unsigned short)long_val;
		if ((long_val > USHRT_MAX) || (long_val < 0))
			WARN("Truncation of value to unsigned short");
		break;
		}
  	case T_INT:{
		long long_val = PyInt_AsLong(v);
		if ((long_val == -1) && PyErr_Occurred())
			return -1;
		*(int *)addr = (int)long_val;
		if ((long_val > INT_MAX) || (long_val < INT_MIN))
			WARN("Truncation of value to int");
		break;
		}
	case T_UINT:{
		unsigned long ulong_val = PyLong_AsUnsignedLong(v);
		if ((ulong_val == (unsigned int)-1) && PyErr_Occurred()) {
			/* XXX: For compatibility, accept negative int values
			   as well. */
			PyErr_Clear();
			ulong_val = PyLong_AsLong(v);
			if ((ulong_val == (unsigned int)-1) && PyErr_Occurred())
				return -1;
			*(unsigned int *)addr = (unsigned int)ulong_val;
			WARN("Writing negative value into unsigned field");
		} else
			*(unsigned int *)addr = (unsigned int)ulong_val;
		if (ulong_val > UINT_MAX)
			WARN("Truncation of value to unsigned int");
		break;
		}
	case T_LONG:{
		*(long*)addr = PyLong_AsLong(v);
		if ((*(long*)addr == -1) && PyErr_Occurred())
			return -1;
		break;
		}
	case T_ULONG:{
		*(unsigned long*)addr = PyLong_AsUnsignedLong(v);
		if ((*(unsigned long*)addr == (unsigned long)-1)
		    && PyErr_Occurred()) {
			/* XXX: For compatibility, accept negative int values
			   as well. */
			PyErr_Clear();
			*(unsigned long*)addr = PyLong_AsLong(v);
			if ((*(unsigned long*)addr == (unsigned int)-1)
			    && PyErr_Occurred())
				return -1;
			WARN("Writing negative value into unsigned field");
		}
		break;
		}
	case T_PYSSIZET:{
		*(Py_ssize_t*)addr = PyInt_AsSsize_t(v);
		if ((*(Py_ssize_t*)addr == (Py_ssize_t)-1)
		    && PyErr_Occurred())
				return -1;
		break;
		}
	case T_FLOAT:{
		double double_val = PyFloat_AsDouble(v);
		if ((double_val == -1) && PyErr_Occurred())
			return -1;
		*(float*)addr = (float)double_val;
		break;
		}
	case T_DOUBLE:
		*(double*)addr = PyFloat_AsDouble(v);
		if ((*(double*)addr == -1) && PyErr_Occurred())
			return -1;
		break;
	case T_OBJECT:
	case T_OBJECT_EX:
		Py_XINCREF(v);
		oldv = *(PyObject **)addr;
		*(PyObject **)addr = v;
		Py_XDECREF(oldv);
		break;
	case T_CHAR:
		if (PyString_Check(v) && PyString_Size(v) == 1) {
			*(char*)addr = PyString_AsString(v)[0];
		}
		else {
			PyErr_BadArgument();
			return -1;
		}
		break;
#ifdef HAVE_LONG_LONG
	case T_LONGLONG:{
		PY_LONG_LONG value;
		*(PY_LONG_LONG*)addr = value = PyLong_AsLongLong(v);
		if ((value == -1) && PyErr_Occurred())
			return -1;
		break;
		}
	case T_ULONGLONG:{
		unsigned PY_LONG_LONG value;
		/* ??? PyLong_AsLongLong accepts an int, but PyLong_AsUnsignedLongLong
			doesn't ??? */
		if (PyLong_Check(v))
			*(unsigned PY_LONG_LONG*)addr = value = PyLong_AsUnsignedLongLong(v);
		else
			*(unsigned PY_LONG_LONG*)addr = value = PyInt_AsLong(v);
		if ((value == (unsigned PY_LONG_LONG)-1) && PyErr_Occurred())
			return -1;
		break;
		}
#endif /* HAVE_LONG_LONG */
	default:
		PyErr_Format(PyExc_SystemError,
			     "bad memberdescr type for %s", l->name);
		return -1;
	}
	return 0;
}
