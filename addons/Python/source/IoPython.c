//metadoc Python copyrigth Aslak Gronflaten, 2006
//metadoc Python license BSD revised
/*metadoc Python description
This object provides access the world of python.
*/
//metadoc Python credit Based on code by Steve Dekorte
//metadoc Python category Server

#include "IoPython.h"
#include "IoState.h"
#include "IoNumber.h"
#include "IoList.h"
#include "IoDirectory.h"
#include <stdlib.h>
#include <stdio.h>

#define DATA(self) ((PythonData *)IoObject_dataPointer(self))

IoTag *IoPython_newTag(void *state)
{
	IoTag *tag = IoTag_newWithName_("Python");
	IoTag_state_(tag, state);
	IoTag_cloneFunc_(tag, (IoTagCloneFunc *)IoPython_rawClone);
	IoTag_freeFunc_(tag, (IoTagFreeFunc *)IoPython_free);
	return tag;
}

IoPython *IoPython_proto(void *state)
{
	Py_Initialize();
	IoObject *self = IoObject_new(state);
	IoObject_tag_(self, IoPython_newTag(state));
	IoObject_setDataPointer_(self, PythonData_new());
	fflush(stdout);
	IoState_registerProtoWithFunc_(state, self, IoPython_proto);

	{
		IoMethodTable methodTable[] = {
		{"credits", IoPython_credits},
		{"forward", IoPython_forward},
		{"import", IoPython_import},
		{"invoke", IoPython_call},
		{"print", IoPython_print},
		{NULL, NULL},
		};
		IoObject_addMethodTable_(self, methodTable);
	}
	DATA(self)->data = (void *) 1; // Hack
	return self;
}

IoPython *IoPython_rawClone(IoPython *proto)
{
	IoPython *self = IoObject_rawClonePrimitive(proto);
	IoObject_setDataPointer_(self, PythonData_new());
	return self;
}

/* ----------------------------------------------------------- */

IoPython *IoPython_new(void *state)
{
	IoObject *proto = IoState_protoWithInitFunction_(state, IoPython_proto);
	return IOCLONE(proto);
}

void IoPython_free(IoPython *self)
{
	PythonData_free(DATA(self));
}
/* ----------------------------------------------------------- */

IoObject *wrap(IoPython *self, PyObject *o) {
	IoPython *ret = IoPython_new(IOSTATE);
	DATA(ret)->data = o;
	return ret;
}

IoObject *IoPython_credits(IoPython *self, IoObject *locals, IoMessage *m)
{
	return IOSYMBOL("Python/Io bridge by Aslak Gronflaten");
}

IoObject *IoPython_print(IoPython *self, IoObject *locals, IoMessage *m)
{
	PyObject *obj = DATA(self)->data;
	if(obj > 1) {
		PyObject_Print(obj, stdout, 0);
	}
	return self;
}

/**
 * Convert an Io object to PyObject.
 * At the moment, we can't pass in any objects, just those that can be translated,
 * until I build a python wrapper around an io object, reverse of what I did here.
 * TODO: Memory management!!!
 */
PyObject *convertIo(IoObject *self, IoObject *obj) {
	PyObject *ret;
	if(ISNUMBER(obj)) {
		ret = PyFloat_FromDouble(CNUMBER(obj));
		Py_INCREF(ret);
		return ret;
	}
	if(ISSEQ(obj)) {
		ret = PyString_FromString(CSTRING(obj));
		Py_INCREF(ret);
		return ret;
	}
	printf("Unable to convert parameter to python.\n");
	return NULL;
}

/**
 * Tries to convert the obj to an Io object, if possible. If not, return null.
 * Can't decref if this method is used on both pFunc and pValue
 */
IoObject *convertPy(IoObject *self, PyObject *obj) {
	//PyObject_Print(obj, stdout, 0);
	//PyObject *pType = PyObject_Type(obj);
	//PyObject_Print(pType, stdout, 0);
	IoObject *ret;  // Return value

	if(PyString_Check(obj)) {
		// Convert to Io sequence and return.
		IoSeq *ret = IoSeq_newWithCString_(IOSTATE, PyString_AsString(obj));
		// TODO:::: Memory management! Who's responsible here! (I am, that's who)
		return ret;
	}

	if(PyFloat_Check(obj)) {
		ret = IoNumber_newWithDouble_(IOSTATE, PyFloat_AS_DOUBLE(obj));
		//Py_DECREF(obj);
		return ret;
	}
	if(PyInt_Check(obj)) {
		ret = IoNumber_newWithDouble_(IOSTATE, PyInt_AS_LONG(obj));
		// Decref?
		return ret;
	}
	if(PyList_Check(obj)) {
		// We have a list. So, make an Io list, and convert every element, and insert them.
		int len = PyList_GET_SIZE(obj);
		ret = IoList_new(IOSTATE);
		int i;
		for(i=0;i<len;i++) {
			PyObject *o = PyList_GET_ITEM(obj, i);
			IoObject *x = convertPy(self, o);
			// insert in list
			IoList_rawAppend_(ret, x);
		}
		return ret;
	}
	if(PyDict_Check(obj)) {
		// We have a dictionary. Make an Io map, and convert all values.
		// or should we.... Io's map can only have string keys... hardly a replacement, now is it.
		// Would be better to build a good wrapper around the python dict.
	}
	if(PyCallable_Check(obj)) {
		//ret = IoState_doString_(IOSTATE, "method(return self invoke(\"\")");
		// TODO: We should return a callable object here... Don't know how though. Yet.
	}
	return wrap(self, obj);
}



IoObject *IoPython_call_int(IoPython *self, IoObject *locals, IoMessage *m, int argOffset, char *functionName)
{
	int argc = IoMessage_argCount(m);

	PyObject *pModule = DATA(self)->data;
	if(!pModule) {
		fprintf(stderr, "We have null pModule for function %s ", functionName);
		return IONIL(self);
	}
	if(!PyObject_HasAttrString(pModule, functionName)){
		fprintf(stderr, "Module has no function %s ", functionName);
		return IONIL(self);
	}

	PyObject *pFunc = PyObject_GetAttrString(pModule, functionName);
	/* pFunc is a new reference */

	if (pFunc && PyCallable_Check(pFunc)) {
		PyObject *pArgs = PyTuple_New(argc - argOffset); // argc
		int i;
		for(i = argOffset;i<argc;i++) {
			IoObject *param = IoMessage_locals_valueArgAt_(m, locals, i);
			PyObject *pyValue = convertIo(self, param);
			PyTuple_SetItem(pArgs, i-argOffset, pyValue);
		}
		PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
		Py_DECREF(pArgs);
		Py_XDECREF(pFunc);
		if (pValue != NULL) {
			return convertPy(self, pValue);
		} else {
			if (PyErr_Occurred())
				PyErr_Print();
			fprintf(stderr,"Call failed\n");
		}
	} else {
		if (PyErr_Occurred())
			PyErr_Print();
		else {
			return convertPy(self, pFunc);
		}
		fprintf(stderr, "Cannot find python function \"%s\"\n", functionName);
	}
	return IONIL(self);
}

IoObject *IoPython_forward(IoPython *self, IoObject *locals, IoMessage *m)
{
	IoSymbol *name = IoMessage_name(m);
	char *functionName = IoSeq_asCString(name);
	return IoPython_call_int(self, locals, m, 0, functionName);
}

IoObject *IoPython_call(IoPython *self, IoObject *locals, IoMessage *m)
{
	IoSeq *name = IoMessage_locals_seqArgAt_(m, locals, 0);
	char *functionName = IoSeq_asCString(name);
	return IoPython_call_int(self, locals, m, 1, functionName);
}


/**
 * Import a module, return reference to it as a Python object
 */
IoObject *IoPython_import(IoPython *self, IoObject *locals, IoMessage *m)
{
	IoSeq *name = IoMessage_locals_seqArgAt_(m, locals, 0);
	char *nameString = IoSeq_asCString(name);

	PyObject *pName, *pModule;
	pName = PyString_FromString(nameString);
	/* Error checking of pName left out */

	pModule = PyImport_Import(pName);

	if(!pModule) {
		fprintf(stderr, "Could not find module %s\n", nameString);
		return IONIL(self);
	}

	// Set slots (for easier introspection and use from io)
	/*
	PyObject *dict = PyModule_GetDict(pModule);
	PyObject *keys = PyDict_Keys(dict);
	int i;
	for(i = 0;i<PyList_Size(keys);i++) {
		PyObject *key = PyList_GetItem(keys, i);
		PyObject *value = PyDict_GetItem(dict, key);
		// TODO: Not allowed method vall IoSeq_newSymbolWithCString_
		if(!PyCallable_Check(value)) {// don't want methods blocking the forward
			IoObject_setSlot_to_(self, IOSYMBOL(PyString_AsString(key)), convertPy(self, value));
		}
	}
	*/
	//

	Py_DECREF(pName);

	// Now, we've got the module. Wrap it and return it.
	return wrap(self, pModule);
}
