#ifndef __MAIN_H__
#define __MAIN_H__ 1

#include <windows.h>
#include <stdio.h>
#include <string>
#include <time.h>
//#include <math.h>
#include <list>
//#include <map>
//#include <algorithm>
#include <FLHook.h>
#include <plugin.h>
#include <Python.h>

using namespace std;

extern bool g_bEnabled;

extern PyObject *pModule; // freelancer.embedded python module
extern PyObject *pCallback; // Our python callback function

extern PyObject *pException; // HK_ERROR exception
extern PyObject *pConstConverter; // const struct/class to namedtuple function
extern PyObject *pClassConverter; // struct/class to namedtuple function


// For Py_BuildValue - Remember to use O not N for increasing ref count
#define PY_BOOL(value) value ? Py_True : Py_False

// These 2 are for Hook callbacks
#define DEFAULT_CHECK() returncode = DEFAULT_RETURNCODE; \
	if (!g_bEnabled) return

#define DEFAULT_CHECK_V(ret) returncode = DEFAULT_RETURNCODE; \
	if (!g_bEnabled) return ret

// For embedded classes - pointer conversions
#define GET_CAPSULE_DATA(pointer_type) \
	PyObject *pPtr, *pFuncArgs; \
	uint iFuncID; \
	if (!PyArg_ParseTuple(pArgs, "OIO", &pPtr, &iFuncID, &pFuncArgs)) \
		return NULL; \
	pointer_type *ptr = (pointer_type*)PyCapsule_GetPointer(pPtr, NULL)

// logging macro print to log and console, so we dont have to dig through logs to find errors while testing....
#define ERRMSG(text) \
	wstring wscError = text; \
	ConPrint(wscError + L"\n"); \
	AddLog( wstos(wscError).c_str() )

#define CHECK_AND_DISABLE(text) \
	if (CheckPyException()) { \
		ERRMSG(text); \
		g_bEnabled = false; \
		return; \
	} \

#define CHECK_AND_DISABLE_NORET(text) \
	if (CheckPyException()) { \
		ERRMSG(text); \
		g_bEnabled = false; \
	} \

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
Converters.cpp
*/
PyObject* pyConstConverter(string scStructName, PyObject *pData);
PyObject* pyClassConverter(string scClassName, void *ptr);
wstring pytows(PyObject *pObj);
string pytos(PyObject *pObj);
PyObject* ToPython(wstring wscString);
PyObject* ToPython(Vector* hkInfo);
PyObject* ToPython(Vector hkInfo);
PyObject* ToPython(Quaternion* hkInfo);
PyObject* ToPython(SSPObjUpdateInfo hkInfo);
PyObject* ToPython(SSPObjCollisionInfo hkInfo);
PyObject* ToPython(SStartupInfo hkInfo);
PyObject* ToPython(SLoginInfo hkInfo);
PyObject* ToPython(SCreateCharacterInfo hkInfo);
PyObject* ToPython(XFireWeaponInfo hkInfo);
PyObject* ToPython(XActivateEquip hkInfo);
PyObject* ToPython(XActivateCruise hkInfo);
PyObject* ToPython(XActivateThrusters hkInfo);
PyObject* ToPython(XSetTarget hkInfo);
PyObject* ToPython(XGoTradelane hkInfo);
PyObject* ToPython(XJettisonCargo hkInfo);
PyObject* ToPython(XSetManeuver hkInfo);
PyObject* ToPython(SGFGoodSellInfo hkInfo);
PyObject* ToPython(SGFGoodBuyInfo hkInfo);
PyObject* ToPython(SSPMunitionCollisionInfo hkInfo);
PyObject* ToPython(EquipDesc hkInfo);
PyObject* ToPython(CHARACTER_ID hkInfo);
PyObject* ToPython(CARGO_INFO hkInfo);
PyObject* ToPython(MONEY_FIX hkInfo);
PyObject* ToPython(IGNORE_INFO hkInfo);
PyObject* ToPython(RESOLVE_IP hkInfo);
PyObject* ToPython(HKPLAYERINFO hkInfo);
PyObject* ToPython(DamageEntry hkInfo);
PyObject* ToPython(list<CARGO_INFO> &lstCargo);
PyObject* ToPython(list<uint> &lst);
PyObject* ToPython(list<DamageEntry> &lstDmg);


PyObject* ToPython(DamageList* hkInfo);
PyObject* ToPython(CLIENT_INFO* hkInfo);

//PyObject* ToPython(SSPUseItem hkInfo);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
EmbeddedMethods.cpp
*/
void BuildEmbedded();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
Main.cpp
*/
bool RaisePyException(HK_ERROR hkErr);
bool CheckPyException();
void pyCallback(string scEvent, PyObject *pData);
void StartPython();
void StopPython();

#endif