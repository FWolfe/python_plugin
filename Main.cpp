/**
 Python Plugin for FLHook-Plugin
 by Fenris_Wolf

0.1:
 Initial release



Notes for modifying this file:
When dealing with python objects and C code you need to keep track of a objects reference count (for pythons garbage collection).
Its important to be aware of which functions increase a reference count or simply borrow the reference or you'll end up with memory leaks.
Py_BuildValue() (used to pass values to python) and PyArg_ParseTuple() (used to fetch values from python) are the 2 most commonly used here.

When we create PyObject * objects in C and want to pass them along to a python function using Py_BuildValue() without having to worry about
the reference count, use N not O. N will steal the reference, while O creates a new reference. If we use O we're forced to call Py_DECREF()
on that object later after python is done with it, leading to much messier code.

When python calls a C function and we use PyArg_ParseTuple(), we use O instead. In this case O does NOT increase the reference count, and python
can properly dispose of the object later when its reference count hits 0 (falls out of scope). If we wish to store this object in C for later use,
remember to call Py_INCREF() and Py_DECREF() manually.

See https://docs.python.org/2/c-api/arg.html for more information on these 2 functions.


*/

#include "headers.h"

PLUGIN_RETURNCODE returncode;
bool g_bEnabled;

PyObject *pModule; // freelancer.embedded python module
PyObject *pCallback; // Our python callback function

PyObject *pException; // HK_ERROR exception
PyObject *pConstConverter; // const struct/class to namedtuple function
PyObject *pClassConverter; // struct/class to namedtuple function

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
RaisePyException - Takes a HK_ERROR and turns it into a python exception.
	this can be checked for with 'try: ... except FLHook.Error: ...'
*/
bool RaisePyException(HK_ERROR hkErr)
{
	if (hkErr != HKE_OK) {
		PyErr_SetObject(pException, Py_BuildValue("Is", hkErr, wstos(HkErrGetText(hkErr))));
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CheckPyException - Checks if a exception was raised in python and logs it
*/
bool CheckPyException()
{
	if (PyErr_Occurred() == NULL)
		return false;
	
	PyObject *pType, *pValue, *pTraceback;
	PyErr_Fetch(&pType, &pValue, &pTraceback);
	ERRMSG(pytows(pType) + L": "  + pytows(pValue));
	Py_XDECREF(pType);
	Py_XDECREF(pValue);
	Py_XDECREF(pTraceback);
	return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
pyBasicCall - calls a function in freelancer.embedded (with no arguments passed)
*/

bool pyBasicCall(const char *func)
{
	PyObject *pFunc, *pResult;
	bool ret = true;
	pFunc = PyObject_GetAttrString(pModule, func);
	if (CheckPyException()) { // bad function name?
		ret = false;
	}
	else {
		pResult = PyObject_CallObject(pFunc, NULL);
		if (CheckPyException()) {
			ERRMSG(L"ERROR: freelancer.embedded." + stows(func) + L" returned NULL");
			ret = false;
		}
	}
	Py_XDECREF(pFunc);
	Py_XDECREF(pResult);
	return ret;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
StartPython - Initializes python, sets our module paths and calls freelancer.embedded._init()
*/
void StartPython()
{
	ConPrint(L"Starting Python....\n");
	Py_Initialize();
	g_bEnabled = true;
	
	BuildEmbedded();
	// setup python module paths
	PyRun_SimpleString(
		"import sys\n"
		"sys.path.append('./flhook_plugins/python')\n"
		//"sys.path.append('C:/Development/PyFL/lib')\n" /// Fenris's Dev directory
	);

	// load the python module
	PyObject *pName;
	pName = PyString_FromString("freelancer.embedded");
	pModule = PyImport_Import(pName);
	Py_XDECREF(pName); // not needed anymore
	CHECK_AND_DISABLE(L"Python Import Error (Python Disabled)");
	
	// should actually check here and confirm these didnt get NULL values
	pConstConverter = PyObject_GetAttrString(pModule, "convertConst");
	CHECK_AND_DISABLE(L"Python Import Error (Python Disabled)");
	pClassConverter = PyObject_GetAttrString(pModule, "convertClass");
	CHECK_AND_DISABLE(L"Python Import Error (Python Disabled)");
	pCallback = PyObject_GetAttrString(pModule, "_callback");
	CHECK_AND_DISABLE(L"Python Import Error (Python Disabled)");

	// now we need to call our python freelancer.embedded._init() function
	if (!pyBasicCall("_init")) {
		g_bEnabled = false;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
StopPython - calls the python freelancer.embedded._shutdown function and unloads the interpreter
*/
void StopPython()
{
	ConPrint(L"Stopping Python....\n");
	try {
		if (g_bEnabled)
			pyBasicCall("_shutdown");
	}
	catch (...) {
		AddLog("Error Closing Python!");
	}
	Py_XDECREF(pException);
	Py_XDECREF(pCallback);
	Py_XDECREF(pConstConverter);
	Py_XDECREF(pClassConverter);
	Py_XDECREF(pModule);
	Py_XDECREF(pModule);
	Py_Finalize();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
pyCallback - calls the python freelancer.embedded._callback function
*/
void pyCallback(string scEvent, PyObject *pData)
{
	PyObject *pResult;
	if (pModule == NULL) {
		// somehow we lost python...something crashed
		CheckPyException();
		ERRMSG(L"ERROR Python Crash? (pModule == NULL)");
		Py_XDECREF(pData);
		g_bEnabled = false;
		return;
	}
	uint iResult = DEFAULT_RETURNCODE;

	if (pData == NULL) { // input data must have errored
		CheckPyException();
		ERRMSG(L"ERROR (" + stows(scEvent) + L") got NULL data");
		Py_XDECREF(pData);
		return;
	}

	try {
		// make the actual python call
		PyObject *pArgs = Py_BuildValue("sN", scEvent.c_str(), pData);
		pResult = PyObject_CallObject(pCallback, pArgs);
		Py_XDECREF(pArgs);
		if (CheckPyException()) {
			ERRMSG(L"ERROR (" + stows(scEvent) + L") Returned NULL");
		}
		else {
			iResult = (uint)PyInt_AsLong(pResult);
		}
	}
	catch (...) { 
		string msg = "Exception in pyCallback (" + scEvent +")";
		AddLog(msg.c_str());
	}
	Py_XDECREF(pResult);

	if (iResult == 1)
		returncode = SKIPPLUGINS;
	else if (iResult == 2)
		returncode = SKIPPLUGINS_NOFUNCTIONCALL;
	else if (iResult == 3)
		returncode = NOFUNCTIONCALL;
	return;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if(fdwReason == DLL_PROCESS_ATTACH) 
	{
		StartPython();
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		StopPython();
	}
	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Callbacks marked with '// TBD' are hooked, but not yet passed to python (for various reasons)
	these functions set DEFAULT_RETURNCODE and simply return.
*/
namespace HkIServerImpl
{
	EXPORT int __stdcall Update()
	{
		int result = 0;
		DEFAULT_CHECK_V(result);
		pyCallback("HkIServerImpl_Update", Py_BuildValue("O", Py_None)); // use O not N here we need to increase the ref count
		returncode = DEFAULT_RETURNCODE; // reset our returncode incase some foolish python script changed it...
		return result;
	}
	EXPORT void __stdcall SubmitChat(struct CHAT_ID cId, unsigned long lP1, void const *rdlReader, struct CHAT_ID cIdTo, int iP2)
	{
		DEFAULT_CHECK();
		// Group join/leave commands
		if (cIdTo.iID == 0x10004)
		{
			return;
		}

		// extract text from rdlReader
		BinaryRDLReader rdl;
		wchar_t wszBuf[1024] = L"";
		uint iRet1;
		rdl.extract_text_from_buffer((unsigned short*)wszBuf, sizeof(wszBuf), iRet1, (const char*)rdlReader, lP1);
		wstring wscBuf = wszBuf;
		uint iClientID = cId.iID;

		pyCallback("HkCbIServerImpl_SubmitChat", Py_BuildValue("INIi", cId.iID, ToPython(wscBuf), cIdTo.iID, iP2));
	}
	EXPORT void __stdcall SubmitChat_AFTER(struct CHAT_ID cId, unsigned long lP1, void const *rdlReader, struct CHAT_ID cIdTo, int iP2)
	{
		DEFAULT_CHECK();

		// Group join/leave commands
		if (cIdTo.iID == 0x10004)
		{
			return;
		}

		// extract text from rdlReader
		BinaryRDLReader rdl;
		wchar_t wszBuf[1024] = L"";
		uint iRet1;
		rdl.extract_text_from_buffer((unsigned short*)wszBuf, sizeof(wszBuf), iRet1, (const char*)rdlReader, lP1);
		wstring wscBuf = wszBuf;
		uint iClientID = cId.iID;

		pyCallback("HkCbIServerImpl_SubmitChat", Py_BuildValue("INIi", cId.iID, ToPython(wscBuf), cIdTo.iID, iP2));
	}
	EXPORT void __stdcall PlayerLaunch(unsigned int iShip, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_PlayerLaunch", Py_BuildValue("II", iShip, iClientID));
	}
	EXPORT void __stdcall PlayerLaunch_AFTER(unsigned int iShip, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_PlayerLaunch_AFTER", Py_BuildValue("II", iShip, iClientID));
	}
	EXPORT void __stdcall FireWeapon(unsigned int iClientID, struct XFireWeaponInfo const &wpn)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_FireWeapon", Py_BuildValue("IN", iClientID, ToPython(wpn)));
	}
	EXPORT void __stdcall FireWeapon_AFTER(unsigned int iClientID, struct XFireWeaponInfo const &wpn)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_FireWeapon_AFTER", Py_BuildValue("IN", iClientID, ToPython(wpn)));
	}
	EXPORT void __stdcall SPMunitionCollision(struct SSPMunitionCollisionInfo const & ci, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SPMunitionCollision", Py_BuildValue("NI", ToPython(ci), iClientID));
	}
	EXPORT void __stdcall SPMunitionCollision_AFTER(struct SSPMunitionCollisionInfo const & ci, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SPMunitionCollision_AFTER", Py_BuildValue("NI", ToPython(ci), iClientID));
	}
	EXPORT void __stdcall SPObjUpdate(struct SSPObjUpdateInfo const &ui, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SPObjUpdate", Py_BuildValue("NI", ToPython(ui), iClientID));
	}
	EXPORT void __stdcall SPObjUpdate_AFTER(struct SSPObjUpdateInfo const &ui, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SPObjUpdate_AFTER", Py_BuildValue("NI", ToPython(ui), iClientID));
	}
	EXPORT void __stdcall SPObjCollision(struct SSPObjCollisionInfo const &ci, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SPObjCollision", Py_BuildValue("NI", ToPython(ci), iClientID));
	}
	EXPORT void __stdcall SPObjCollision_AFTER(struct SSPObjCollisionInfo const &ci, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SPObjCollision_AFTER", Py_BuildValue("NI", ToPython(ci), iClientID));
	}
	EXPORT void __stdcall LaunchComplete(unsigned int iBaseID, unsigned int iShip)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_LaunchComplete", Py_BuildValue("II", iBaseID, iShip));
	}
	EXPORT void __stdcall LaunchComplete_AFTER(unsigned int iBaseID, unsigned int iShip)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_LaunchComplete_AFTER", Py_BuildValue("II", iBaseID, iShip));
	}
	EXPORT void __stdcall CharacterSelect(struct CHARACTER_ID const & cId, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_CharacterSelect", Py_BuildValue("NI", ToPython(cId), iClientID));
	}
	EXPORT void __stdcall CharacterSelect_AFTER(struct CHARACTER_ID const & cId, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_CharacterSelect_AFTER", Py_BuildValue("NI", ToPython(cId), iClientID));
	}
	EXPORT void __stdcall BaseEnter(unsigned int iBaseID, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_BaseEnter", Py_BuildValue("II", iBaseID, iClientID));
	}
	EXPORT void __stdcall BaseEnter_AFTER(unsigned int iBaseID, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_BaseEnter_AFTER", Py_BuildValue("II", iBaseID, iClientID));
	}
	EXPORT void __stdcall BaseExit(unsigned int iBaseID, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_BaseExit", Py_BuildValue("II", iBaseID, iClientID));
	}
	EXPORT void __stdcall BaseExit_AFTER(unsigned int iBaseID, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_BaseExit_AFTER", Py_BuildValue("II", iBaseID, iClientID));
	}
	EXPORT void __stdcall OnConnect(unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_OnConnect", Py_BuildValue("I", iClientID));
	}
	EXPORT void __stdcall OnConnect_AFTER(unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_OnConnect_AFTER", Py_BuildValue("I", iClientID));
	}
	EXPORT void __stdcall DisConnect(unsigned int iClientID, enum EFLConnection p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_DisConnect", Py_BuildValue("II", iClientID, p2));
	}
	EXPORT void __stdcall DisConnect_AFTER(unsigned int iClientID, enum EFLConnection p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_DisConnect_AFTER", Py_BuildValue("II", iClientID, p2));
	}
	EXPORT void __stdcall TerminateTrade(unsigned int iClientID, int iAccepted)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_TerminateTrade", Py_BuildValue("Ii", iClientID, iAccepted));
	}
	EXPORT void __stdcall TerminateTrade_AFTER(unsigned int iClientID, int iAccepted)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_TerminateTrade_AFTER", Py_BuildValue("Ii", iClientID, iAccepted));
	}
	EXPORT void __stdcall InitiateTrade(unsigned int iClientID1, unsigned int iClientID2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_InitiateTrade", Py_BuildValue("II", iClientID1, iClientID2));
	}
	EXPORT void __stdcall InitiateTrade_AFTER(unsigned int iClientID1, unsigned int iClientID2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_InitiateTrade_AFTER", Py_BuildValue("II", iClientID1, iClientID2));
	}
	EXPORT void __stdcall ActivateEquip(unsigned int iClientID, struct XActivateEquip const &aq)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ActivateEquip", Py_BuildValue("IN", iClientID, ToPython(aq)));
	}
	EXPORT void __stdcall ActivateEquip_AFTER(unsigned int iClientID, struct XActivateEquip const &aq)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ActivateEquip_AFTER", Py_BuildValue("IN", iClientID, ToPython(aq)));
	}
	EXPORT void __stdcall ActivateCruise(unsigned int iClientID, struct XActivateCruise const &ac)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ActivateCruise", Py_BuildValue("IN", iClientID, ToPython(ac)));
	}
	EXPORT void __stdcall ActivateCruise_AFTER(unsigned int iClientID, struct XActivateCruise const &ac)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ActivateCruise_AFTER", Py_BuildValue("IN", iClientID, ToPython(ac)));
	}
	EXPORT void __stdcall ActivateThrusters(unsigned int iClientID, struct XActivateThrusters const &at)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ActivateThrusters", Py_BuildValue("IN", iClientID, ToPython(at)));
	}
	EXPORT void __stdcall ActivateThrusters_AFTER(unsigned int iClientID, struct XActivateThrusters const &at)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ActivateThrusters_AFTER", Py_BuildValue("IN", iClientID, ToPython(at)));
	}
	EXPORT void __stdcall GFGoodSell(struct SGFGoodSellInfo const &gsi, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_GFGoodSell", Py_BuildValue("NI", ToPython(gsi), iClientID));
	}
	EXPORT void __stdcall GFGoodSell_AFTER(struct SGFGoodSellInfo const &gsi, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_GFGoodSell_AFTER", Py_BuildValue("NI", ToPython(gsi), iClientID));
	}
	EXPORT void __stdcall CharacterInfoReq(unsigned int iClientID, bool p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_CharacterInfoReq", Py_BuildValue("IO", iClientID, PY_BOOL(p2)));
	}
	EXPORT void __stdcall CharacterInfoReq_AFTER(unsigned int iClientID, bool p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_CharacterInfoReq_AFTER", Py_BuildValue("IO", iClientID, PY_BOOL(p2)));
	}
	EXPORT void __stdcall JumpInComplete(unsigned int iSystemID, unsigned int iShip)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_JumpInComplete", Py_BuildValue("II", iSystemID, iShip));
	}
	EXPORT void __stdcall JumpInComplete_AFTER(unsigned int iSystemID, unsigned int iShip)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_JumpInComplete_AFTER", Py_BuildValue("II", iSystemID, iShip));
	}
	EXPORT void __stdcall SystemSwitchOutComplete(unsigned int iShip, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SystemSwitchOutComplete", Py_BuildValue("II", iShip, iClientID));
	}
	EXPORT void __stdcall SystemSwitchOutComplete_AFTER(unsigned int iShip, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SystemSwitchOutComplete_AFTER", Py_BuildValue("II", iShip, iClientID));
	}
	EXPORT void __stdcall Login(struct SLoginInfo const &li, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_Login", Py_BuildValue("NI", ToPython(li), iClientID));
	}
	EXPORT void __stdcall Login_AFTER(struct SLoginInfo const &li, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_Login_AFTER", Py_BuildValue("NI", ToPython(li), iClientID));
	}
	EXPORT void __stdcall MineAsteroid(unsigned int p1, class Vector const &vPos, unsigned int iLookID, unsigned int iGoodID, unsigned int iCount, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_MineAsteroid", Py_BuildValue("INIIII", p1, ToPython(vPos), iLookID, iGoodID, iCount, iClientID));
	}
	EXPORT void __stdcall MineAsteroid_AFTER(unsigned int p1, class Vector const &vPos, unsigned int iLookID, unsigned int iGoodID, unsigned int iCount, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_MineAsteroid_AFTER", Py_BuildValue("INIIII", p1, ToPython(vPos), iLookID, iGoodID, iCount, iClientID));
	}
	EXPORT void __stdcall GoTradelane(unsigned int iClientID, struct XGoTradelane const &gtl)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_GoTradelane", Py_BuildValue("IN", iClientID, ToPython(gtl)));
	}
	EXPORT void __stdcall GoTradelane_AFTER(unsigned int iClientID, struct XGoTradelane const &gtl)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_GoTradelane_AFTER", Py_BuildValue("IN", iClientID, ToPython(gtl)));
	}
	EXPORT void __stdcall StopTradelane(unsigned int iClientID, unsigned int p2, unsigned int p3, unsigned int p4)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_AbortMission", Py_BuildValue("IIII", iClientID, p2, p3, p4));
	}
	EXPORT void __stdcall StopTradelane_AFTER(unsigned int iClientID, unsigned int p2, unsigned int p3, unsigned int p4)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_AbortMission", Py_BuildValue("IIII", iClientID, p2, p3, p4));
	}
	EXPORT void __stdcall AbortMission(unsigned int p1, unsigned int p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_AbortMission", Py_BuildValue("II", p1, p2));
	}
	EXPORT void __stdcall AbortMission_AFTER(unsigned int p1, unsigned int p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_AbortMission_AFTER", Py_BuildValue("II", p1, p2));
	}
	EXPORT void __stdcall AcceptTrade(unsigned int iClientID, bool p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_AcceptTrade", Py_BuildValue("IO", iClientID, PY_BOOL(p2)));
	}
	EXPORT void __stdcall AcceptTrade_AFTER(unsigned int iClientID, bool p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_AcceptTrade_AFTER", Py_BuildValue("IO", iClientID, PY_BOOL(p2)));
	}
	EXPORT void __stdcall AddTradeEquip(unsigned int iClientID, struct EquipDesc const &ed)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_AddTradeEquip", Py_BuildValue("IN", iClientID, ToPython(ed)));
	}
	EXPORT void __stdcall AddTradeEquip_AFTER(unsigned int iClientID, struct EquipDesc const &ed)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_AddTradeEquip_AFTER", Py_BuildValue("IN", iClientID, ToPython(ed)));
	}
	EXPORT void __stdcall BaseInfoRequest(unsigned int p1, unsigned int p2, bool p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_BaseInfoRequest", Py_BuildValue("IIO", p1, p2, PY_BOOL(p3)));
	}
	EXPORT void __stdcall BaseInfoRequest_AFTER(unsigned int p1, unsigned int p2, bool p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_BaseInfoRequest_AFTER", Py_BuildValue("IIO", p1, p2, PY_BOOL(p3)));
	}
	EXPORT void __stdcall CreateNewCharacter(struct SCreateCharacterInfo const & scci, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_CreateNewCharacter", Py_BuildValue("NI", ToPython(scci), iClientID));
	}
	EXPORT void __stdcall CreateNewCharacter_AFTER(struct SCreateCharacterInfo const & scci, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_CreateNewCharacter_AFTER", Py_BuildValue("NI", ToPython(scci), iClientID));
	}
	EXPORT void __stdcall DelTradeEquip(unsigned int iClientID, struct EquipDesc const &ed)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_DelTradeEquip", Py_BuildValue("IN", iClientID, ToPython(ed)));
	}
	EXPORT void __stdcall DelTradeEquip_AFTER(unsigned int iClientID, struct EquipDesc const &ed)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_DelTradeEquip_AFTER", Py_BuildValue("IN", iClientID, ToPython(ed)));
	}
	EXPORT void __stdcall DestroyCharacter(struct CHARACTER_ID const &cId, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_DestroyCharacter", Py_BuildValue("NI", ToPython(cId), iClientID));
	}
	EXPORT void __stdcall DestroyCharacter_AFTER(struct CHARACTER_ID const &cId, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_DestroyCharacter_AFTER", Py_BuildValue("NI", ToPython(cId), iClientID));
	}
	EXPORT void __stdcall GFGoodBuy(struct SGFGoodBuyInfo const &gbi, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_GFGoodBuy", Py_BuildValue("NI", ToPython(gbi), iClientID));
	}
	EXPORT void __stdcall GFGoodBuy_AFTER(struct SGFGoodBuyInfo const &gbi, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_GFGoodBuy_AFTER", Py_BuildValue("NI", ToPython(gbi), iClientID));
	}
	// TBD
	EXPORT void __stdcall GFGoodVaporized(struct SGFGoodVaporizedInfo const &gvi, unsigned int iClientID)
	{
		DEFAULT_CHECK();
	}
	// TBD
	EXPORT void __stdcall GFGoodVaporized_AFTER(struct SGFGoodVaporizedInfo const &gvi, unsigned int iClientID)
	{
		DEFAULT_CHECK();
	}
	EXPORT void __stdcall GFObjSelect(unsigned int p1, unsigned int p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_GFObjSelect", Py_BuildValue("II", p1, p2));
	}
	EXPORT void __stdcall GFObjSelect_AFTER(unsigned int p1, unsigned int p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_GFObjSelect_AFTER", Py_BuildValue("II", p1, p2));
	}
	EXPORT void __stdcall Hail(unsigned int p1, unsigned int p2, unsigned int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_Hail", Py_BuildValue("III", p1, p2, p3));
	}
	EXPORT void __stdcall Hail_AFTER(unsigned int p1, unsigned int p2, unsigned int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_Hail_AFTER", Py_BuildValue("III", p1, p2, p3));
	}
	EXPORT void __stdcall InterfaceItemUsed(unsigned int p1, unsigned int p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_InterfaceItemUsed", Py_BuildValue("II", p1, p2));
	}
	EXPORT void __stdcall InterfaceItemUsed_AFTER(unsigned int p1, unsigned int p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_InterfaceItemUsed_AFTER", Py_BuildValue("II", p1, p2));
	}
	EXPORT void __stdcall JettisonCargo(unsigned int iClientID, struct XJettisonCargo const &jc)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_JettisonCargo", Py_BuildValue("IN", iClientID, ToPython(jc)));
	}
	EXPORT void __stdcall JettisonCargo_AFTER(unsigned int iClientID, struct XJettisonCargo const &jc)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_JettisonCargo_AFTER", Py_BuildValue("IN", iClientID, ToPython(jc)));
	}
	EXPORT void __stdcall LocationEnter(unsigned int p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_LocationEnter", Py_BuildValue("II", p1, iClientID));
	}
	EXPORT void __stdcall LocationEnter_AFTER(unsigned int p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_LocationEnter_AFTER", Py_BuildValue("II", p1, iClientID));
	}
	EXPORT void __stdcall LocationExit(unsigned int p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_LocationExit", Py_BuildValue("II", p1, iClientID));
	}
	EXPORT void __stdcall LocationExit_AFTER(unsigned int p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_LocationExit_AFTER", Py_BuildValue("II", p1, iClientID));
	}
	EXPORT void __stdcall LocationInfoRequest(unsigned int p1,unsigned int p2, bool p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_LocationInfoRequest", Py_BuildValue("IIO", p1, p2, PY_BOOL(p3)));
	}
	EXPORT void __stdcall LocationInfoRequest_AFTER(unsigned int p1,unsigned int p2, bool p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_LocationInfoRequest_AFTER", Py_BuildValue("IIO", p1, p2, PY_BOOL(p3)));
	}
	EXPORT void __stdcall MissionResponse(unsigned int p1, unsigned long p2, bool p3, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_MissionResponse", Py_BuildValue("IIOI", p1, p2, PY_BOOL(p3), iClientID));
	}
	EXPORT void __stdcall MissionResponse_AFTER(unsigned int p1, unsigned long p2, bool p3, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_MissionResponse_AFTER", Py_BuildValue("IIOI", p1, p2, PY_BOOL(p3), iClientID));
	}
	EXPORT void __stdcall ReqAddItem(unsigned int p1, char const *p2, int p3, float p4, bool p5, unsigned int p6)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqAddItem", Py_BuildValue("IsifOI", p1, p2, p3, p4, PY_BOOL(p5), p6));
	}
	EXPORT void __stdcall ReqAddItem_AFTER(unsigned int p1, char const *p2, int p3, float p4, bool p5, unsigned int p6)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqAddItem_AFTER", Py_BuildValue("IsifOI", p1, p2, p3, p4, PY_BOOL(p5), p6));
	}
	EXPORT void __stdcall ReqChangeCash(int p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqChangeCash", Py_BuildValue("iI", p1, iClientID));
	}
	EXPORT void __stdcall ReqChangeCash_AFTER(int p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqChangeCash_AFTER", Py_BuildValue("iI", p1, iClientID));
	}
	// TBD
	EXPORT void __stdcall ReqCollisionGroups(class std::list<struct CollisionGroupDesc,class std::allocator<struct CollisionGroupDesc> > const &p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
	}
	// TBD
	EXPORT void __stdcall ReqCollisionGroups_AFTER(class std::list<struct CollisionGroupDesc,class std::allocator<struct CollisionGroupDesc> > const &p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
	}
	// TBD
	EXPORT void __stdcall ReqEquipment(class EquipDescList const &edl, unsigned int iClientID)
	{
		DEFAULT_CHECK();
	}
	// TBD
	EXPORT void __stdcall ReqEquipment_AFTER(class EquipDescList const &edl, unsigned int iClientID)
	{
		DEFAULT_CHECK();
	}
	EXPORT void __stdcall ReqHullStatus(float p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqHullStatus", Py_BuildValue("fI", p1, iClientID));
	}
	EXPORT void __stdcall ReqHullStatus_AFTER(float p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqHullStatus_AFTER", Py_BuildValue("fI", p1, iClientID));
	}
	EXPORT void __stdcall ReqModifyItem(unsigned short p1, char const *p2, int p3, float p4, bool p5, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqModifyItem", Py_BuildValue("HsifOI", p1, p2, p3, p4, PY_BOOL(p5), iClientID));
	}
	EXPORT void __stdcall ReqModifyItem_AFTER(unsigned short p1, char const *p2, int p3, float p4, bool p5, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqModifyItem_AFTER", Py_BuildValue("HsifOI", p1, p2, p3, p4, PY_BOOL(p5), iClientID));
	}
	EXPORT void __stdcall ReqRemoveItem(unsigned short p1, int p2, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqRemoveItem", Py_BuildValue("HiI", p1, p2, iClientID));
	}
	EXPORT void __stdcall ReqRemoveItem_AFTER(unsigned short p1, int p2, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqRemoveItem_AFTER", Py_BuildValue("HiI", p1, p2, iClientID));
	}
	EXPORT void __stdcall ReqSetCash(int p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqSetCash", Py_BuildValue("iI", p1, iClientID));
	}
	EXPORT void __stdcall ReqSetCash_AFTER(int p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqSetCash_AFTER", Py_BuildValue("iI", p1, iClientID));
	}
	EXPORT void __stdcall ReqShipArch(unsigned int p1, unsigned int p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqShipArch", Py_BuildValue("II", p1, p2));
	}
	EXPORT void __stdcall ReqShipArch_AFTER(unsigned int p1, unsigned int p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_ReqShipArch_AFTER", Py_BuildValue("II", p1, p2));
	}
	EXPORT void __stdcall RequestBestPath(unsigned int p1, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestBestPath", Py_BuildValue("Is#i", p1, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall RequestBestPath_AFTER(unsigned int p1, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestBestPath_AFTER", Py_BuildValue("Is#i", p1, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall RequestCancel(int iType, unsigned int iShip, unsigned int p3, unsigned long p4, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestCancel", Py_BuildValue("iIIkI", iType, iShip, p3, p4, iClientID));
	}
	EXPORT void __stdcall RequestCancel_AFTER(int iType, unsigned int iShip, unsigned int p3, unsigned long p4, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestCancel_AFTER", Py_BuildValue("iIIkI", iType, iShip, p3, p4, iClientID));
	}
	EXPORT void __stdcall RequestCreateShip(unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestCreateShip", Py_BuildValue("I", iClientID));
	}
	EXPORT void __stdcall RequestCreateShip_AFTER(unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestCreateShip_AFTER", Py_BuildValue("I", iClientID));
	}
	EXPORT void __stdcall RequestEvent(int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned long p5, unsigned int p6)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestEvent", Py_BuildValue("iIIIkI", p1, p2, p3, p4, p5, p6));
	}
	EXPORT void __stdcall RequestEvent_AFTER(int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned long p5, unsigned int p6)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestEvent_AFTER", Py_BuildValue("iIIIkI", p1, p2, p3, p4, p5, p6));
	}
	EXPORT void __stdcall RequestGroupPositions(unsigned int p1, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestGroupPositions", Py_BuildValue("Is#i", p1, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall RequestGroupPositions_AFTER(unsigned int p1, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestGroupPositions_AFTER", Py_BuildValue("Is#i", p1, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall RequestPlayerStats(unsigned int p1, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestPlayerStats", Py_BuildValue("Is#i", p1, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall RequestPlayerStats_AFTER(unsigned int p1, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestPlayerStats_AFTER", Py_BuildValue("Is#i", p1, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall RequestRankLevel(unsigned int p1, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestRankLevel", Py_BuildValue("Is#i", p1, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall RequestRankLevel_AFTER(unsigned int p1, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestRankLevel_AFTER", Py_BuildValue("Is#i", p1, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall RequestTrade(unsigned int p1, unsigned int p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestTrade", Py_BuildValue("II", p1, p2));
	}
	EXPORT void __stdcall RequestTrade_AFTER(unsigned int p1, unsigned int p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_RequestTrade_AFTER", Py_BuildValue("II", p1, p2));
	}
	EXPORT void __stdcall SPRequestInvincibility(unsigned int iShip, bool p2, enum InvincibilityReason p3, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SPRequestInvincibility", Py_BuildValue("IOII", iShip, PY_BOOL(p2), p3, iClientID));
	}
	EXPORT void __stdcall SPRequestInvincibility_AFTER(unsigned int iShip, bool p2, enum InvincibilityReason p3, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SPRequestInvincibility_AFTER", Py_BuildValue("IOII", iShip, PY_BOOL(p2), p3, iClientID));
	}
	// TBD
	EXPORT void __stdcall SPRequestUseItem(struct SSPUseItem const &p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
		//pyCallback("HkCbIServerImpl_SPRequestUseItem", Py_BuildValue("OI", ToPython(p1), iClientID));
	}
	// TBD
	EXPORT void __stdcall SPRequestUseItem_AFTER(struct SSPUseItem const &p1, unsigned int iClientID)
	{
		DEFAULT_CHECK();
	}
	EXPORT void __stdcall SPScanCargo(unsigned int const &p1, unsigned int const &p2, unsigned int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SPScanCargo", Py_BuildValue("III", p1, p2, p3));
	}
	EXPORT void __stdcall SPScanCargo_AFTER(unsigned int const &p1, unsigned int const &p2, unsigned int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SPScanCargo_AFTER", Py_BuildValue("III", p1, p2, p3));
	}
	EXPORT void __stdcall SetInterfaceState(unsigned int p1, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SetInterfaceState", Py_BuildValue("Is#i", p1, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall SetInterfaceState_AFTER(unsigned int p1, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SetInterfaceState_AFTER", Py_BuildValue("Is#i", p1, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall SetManeuver(unsigned int iClientID, struct XSetManeuver const &p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SetManeuver", Py_BuildValue("IN", iClientID, ToPython(p2)));
	}
	EXPORT void __stdcall SetManeuver_AFTER(unsigned int iClientID, struct XSetManeuver const &p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SetManeuver_AFTER", Py_BuildValue("IN", iClientID, ToPython(p2)));
	}
	EXPORT void __stdcall SetTarget(unsigned int iClientID, struct XSetTarget const &p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SetTarget", Py_BuildValue("IN", iClientID, ToPython(p2)));
	}
	EXPORT void __stdcall SetTarget_AFTER(unsigned int iClientID, struct XSetTarget const &p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SetTarget_AFTER", Py_BuildValue("IN", iClientID, ToPython(p2)));
	}
	EXPORT void __stdcall SetTradeMoney(unsigned int iClientID, unsigned long p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SetTradeMoney", Py_BuildValue("Ik", iClientID, p2));
	}
	EXPORT void __stdcall SetTradeMoney_AFTER(unsigned int iClientID, unsigned long p2)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SetTradeMoney_AFTER", Py_BuildValue("Ik", iClientID, p2));
	}
	EXPORT void __stdcall SetVisitedState(unsigned int iClientID, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SetVisitedState", Py_BuildValue("Is#i", iClientID, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall SetVisitedState_AFTER(unsigned int iClientID, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SetVisitedState_AFTER", Py_BuildValue("Is#i", iClientID, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall SetWeaponGroup(unsigned int iClientID, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SetWeaponGroup", Py_BuildValue("Is#i", iClientID, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall SetWeaponGroup_AFTER(unsigned int iClientID, unsigned char *p2, int p3)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_SetWeaponGroup_AFTER", Py_BuildValue("Is#i", iClientID, p2, strlen((char*)p2), p3));
	}
	EXPORT void __stdcall Shutdown(void)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_Shutdown", Py_BuildValue("O", Py_True)); // use O not N here we need to increase the ref count
	}
	EXPORT bool __stdcall Startup(struct SStartupInfo const &p1)
	{
		DEFAULT_CHECK_V(true);
		pyCallback("HkCbIServerImpl_Startup", Py_BuildValue("N", ToPython(p1)));
		return true;
	}
	EXPORT bool __stdcall Startup_AFTER(struct SStartupInfo const &p1)
	{
		DEFAULT_CHECK_V(true);
		pyCallback("HkCbIServerImpl_Startup_AFTER", Py_BuildValue("N", ToPython(p1)));
		return true;
	}
	EXPORT void __stdcall StopTradeRequest(unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_StopTradeRequest", Py_BuildValue("I", iClientID));
	}
	EXPORT void __stdcall StopTradeRequest_AFTER(unsigned int iClientID)
	{
		DEFAULT_CHECK();
		pyCallback("HkCbIServerImpl_StopTradeRequest_AFTER", Py_BuildValue("I", iClientID));
	}
	// TBD
	EXPORT void __stdcall TractorObjects(unsigned int iClientID, struct XTractorObjects const &p2)
	{
		DEFAULT_CHECK();
	}
	// TBD
	EXPORT void __stdcall TractorObjects_AFTER(unsigned int iClientID, struct XTractorObjects const &p2)
	{
		DEFAULT_CHECK();
	}
	// TBD
	EXPORT void __stdcall TradeResponse(unsigned char const *p1, int p2, unsigned int iClientID)
	{
		DEFAULT_CHECK();
	}
	// TBD
	EXPORT void __stdcall TradeResponse_AFTER(unsigned char const *p1, int p2, unsigned int iClientID)
	{
		DEFAULT_CHECK();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
EXPORT void ClearClientInfo(uint iClientID)
{
	DEFAULT_CHECK();
	pyCallback("ClearClientInfo", Py_BuildValue("I", iClientID));
}
EXPORT void LoadUserCharSettings(uint iClientID)
{
	DEFAULT_CHECK();
	pyCallback("LoadUserCharSettings", Py_BuildValue("I", iClientID));
}
// TBD
EXPORT void __stdcall HkCb_SendChat(uint iClientID, uint iTo, uint iSize, void *pRDL)
{
	DEFAULT_CHECK();
}
// TBD
EXPORT int __stdcall HkCB_MissileTorpHit(char *ECX, char *p1, DamageList *dmg)
{
	DEFAULT_CHECK_V(0);
	return 0;
}
EXPORT void __stdcall HkCb_AddDmgEntry(DamageList *dmg, unsigned short p1, float p2, enum DamageEntry::SubObjFate p3)
{
	DEFAULT_CHECK();
	pyCallback("HkCb_AddDmgEntry", Py_BuildValue("NHfI", ToPython(dmg), p1, p2, p3));
}
EXPORT void __stdcall HkCb_AddDmgEntry_AFTER(DamageList *dmg, unsigned short p1, float p2, enum DamageEntry::SubObjFate p3)
{
	DEFAULT_CHECK();
	pyCallback("HkCb_AddDmgEntry_AFTER", Py_BuildValue("NHfI", ToPython(dmg), p1, p2, p3));
}
// TBD
EXPORT void __stdcall HkCb_GeneralDmg(char *szECX)
{
	DEFAULT_CHECK();
	//pyCallback("HkCb_GeneralDmg", Py_BuildValue("s", szECX));
}
EXPORT bool AllowPlayerDamage(uint iClientID, uint iClientIDTarget)
{
	DEFAULT_CHECK_V(true);
	pyCallback("HkCb_AllowPlayerDamage", Py_BuildValue("II", iClientID, iClientIDTarget));
	if (returncode != DEFAULT_RETURNCODE)
		returncode = DEFAULT_RETURNCODE;
		return false;
	return true;
}
EXPORT void SendDeathMsg(const wstring &wscMsg, uint iSystemID, uint iClientIDVictim, uint iClientIDKiller)
{
	DEFAULT_CHECK();
	pyCallback("SendDeathMsg", Py_BuildValue("NIII", ToPython(wscMsg), iSystemID, iClientIDVictim, iClientIDKiller));
}
EXPORT void __stdcall ShipDestroyed(DamageList *_dmg, DWORD *ecx, uint iKill)
{
	DEFAULT_CHECK();
	pyCallback("ShipDestroyed", Py_BuildValue("NkI", ToPython(_dmg), ecx, iKill));
}
EXPORT void BaseDestroyed(uint iObject, uint iClientIDBy)
{
	DEFAULT_CHECK();
	pyCallback("BaseDestroyed", Py_BuildValue("II", iObject, iClientIDBy));
}
// TBD
EXPORT void __stdcall HkIEngine_CShip_init(CShip* ship)
{
	DEFAULT_CHECK();
	returncode = DEFAULT_RETURNCODE;
}
// TBD
EXPORT void __stdcall HkIEngine_CShip_destroy(CShip* ship)
{
	DEFAULT_CHECK();
	returncode = DEFAULT_RETURNCODE;
}
EXPORT void HkCb_Update_Time(double dInterval)
{
	DEFAULT_CHECK();
	pyCallback("HkCb_Update_Time", Py_BuildValue("d", dInterval));
}
EXPORT void HkCb_Update_Time_AFTER(double dInterval)
{
	DEFAULT_CHECK();
	pyCallback("HkCb_Update_Time_AFTER", Py_BuildValue("d", dInterval));
}
// TBD
EXPORT int HkCb_Dock_Call(unsigned int const &uShipID, unsigned int const &uSpaceID, int p3, enum DOCK_HOST_RESPONSE p4)
{
	DEFAULT_CHECK_V(0);
	return 0;
}
// TBD
EXPORT int HkCb_Dock_Call_AFTER(unsigned int const &uShipID, unsigned int const &uSpaceID, int p3, enum DOCK_HOST_RESPONSE p4)
{
	DEFAULT_CHECK_V(0);
	return 0;
}
EXPORT void __stdcall HkCb_Elapse_Time(float p1)
{
	DEFAULT_CHECK();
	pyCallback("HkCb_Elapse_Time", Py_BuildValue("f", p1));
}
EXPORT void __stdcall HkCb_Elapse_Time_AFTER(float p1)
{
	DEFAULT_CHECK();
	pyCallback("HkCb_Elapse_Time_AFTER", Py_BuildValue("f", p1));
}
// TBD
EXPORT bool __stdcall LaunchPosHook(uint iSpaceID, struct CEqObj &p1, Vector &p2, Matrix &p3, int iDock)
{
	DEFAULT_CHECK_V(false);
	return false;
}
EXPORT void HkTimerCheckKick()
{
	DEFAULT_CHECK();
	pyCallback("HkTimerCheckKick", Py_BuildValue("O", Py_True)); // use O not N here we need to increase the ref count
}
EXPORT void HkTimerNPCAndF1Check()
{
	DEFAULT_CHECK();
	pyCallback("HkTimerCheckKick", Py_BuildValue("O", Py_True)); // use O not N here we need to increase the ref count
}
// We dont actually need to hook this one, since /help is also sent by UserCmd_Process
EXPORT void UserCmd_Help(uint iClientID, const wstring &wscParam)
{
	DEFAULT_CHECK();
	pyCallback("UserCmd_Help", Py_BuildValue("IN", iClientID, ToPython(wscParam)));
}
EXPORT bool UserCmd_Process(uint iClientID, const wstring &wscCmd)
{
	DEFAULT_CHECK_V(false);
	pyCallback("UserCmd_Process", Py_BuildValue("IN", iClientID, ToPython(wscCmd)));
	if (returncode != DEFAULT_RETURNCODE)
		return true;
	return false;
}
// TBD
EXPORT void CmdHelp_Callback(CCmds* classptr)
{
	DEFAULT_CHECK();
}
EXPORT bool ExecuteCommandString_Callback(CCmds* classptr, const wstring &wscCmdStr)
{
	DEFAULT_CHECK_V(false);
	pyCallback("ExecuteCommandString_Callback", Py_BuildValue("ON", Py_None, ToPython(wscCmdStr)));
	if (returncode != DEFAULT_RETURNCODE)
		return true;
	return false;
}
EXPORT void ProcessEvent_BEFORE(wstring &wscText)
{
	DEFAULT_CHECK();
	pyCallback("ProcessEvent_BEFORE", Py_BuildValue("N", ToPython(wscText)));
}
EXPORT void LoadSettings()
{
	DEFAULT_CHECK();
	pyCallback("LoadSettings", Py_BuildValue("O", Py_True));
}
// TBD
EXPORT void Plugin_Communication_CallBack(PLUGIN_MESSAGE msg, void* data) {
	DEFAULT_CHECK();
}
/*
	These we dont need to hook

PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_CREATESHIP,
PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_CREATESHIP_AFTER,
PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_CREATELOOT,
PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_CREATELOOT_AFTER,
PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_CREATESOLAR,
PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_LAUNCH,

PLUGIN_HkIClientImpl_Send_FLPACKET_COMMON_UPDATEOBJECT,
PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_ACTIVATEOBJECT,
PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_DESTROYOBJECT,
PLUGIN_HkIClientImpl_Send_FLPACKET_COMMON_FIREWEAPON,
PLUGIN_HkIClientImpl_Send_FLPACKET_COMMON_ACTIVATEEQUIP,
PLUGIN_HkIClientImpl_Send_FLPACKET_COMMON_ACTIVATECRUISE,
PLUGIN_HkIClientImpl_Send_FLPACKET_COMMON_ACTIVATETHRUSTERS,

PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_MISCOBJUPDATE_3,
PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_MISCOBJUPDATE_3_AFTER,
PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_MISCOBJUPDATE_5,
PLUGIN_HkIClientImpl_Send_FLPACKET_SERVER_REQUESTCREATESHIPRESP,
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
	For performance issues, you should comment out all hooks you dont actually intend to use in your python scripts
	At the very least, comment out the least useful ones that are frequently called.
*/
EXPORT PLUGIN_INFO* Get_PluginInfo()
{
    PLUGIN_INFO* p_PI = new PLUGIN_INFO();
    p_PI->sName = "Python Plugin by Fenris_Wolf";
    p_PI->sShortName = "python";
    p_PI->bMayPause = true;
    p_PI->bMayUnload = true;
    p_PI->ePluginReturnCode = &returncode;

	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ClearClientInfo, PLUGIN_ClearClientInfo, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadUserCharSettings, PLUGIN_LoadUserCharSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_SendChat, PLUGIN_HkCb_SendChat, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCB_MissileTorpHit, PLUGIN_HkCB_MissileTorpHit, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_AddDmgEntry, PLUGIN_HkCb_AddDmgEntry, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_AddDmgEntry_AFTER, PLUGIN_HkCb_AddDmgEntry_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_GeneralDmg, PLUGIN_HkCb_GeneralDmg, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&AllowPlayerDamage, PLUGIN_AllowPlayerDamage, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&SendDeathMsg, PLUGIN_SendDeathMsg, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ShipDestroyed, PLUGIN_ShipDestroyed, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&BaseDestroyed, PLUGIN_BaseDestroyed, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIEngine_CShip_init, PLUGIN_HkIEngine_CShip_init, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIEngine_CShip_destroy, PLUGIN_HkIEngine_CShip_destroy, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_Update_Time, PLUGIN_HkCb_Update_Time, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_Update_Time_AFTER, PLUGIN_HkCb_Update_Time_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_Dock_Call, PLUGIN_HkCb_Dock_Call, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_Dock_Call_AFTER, PLUGIN_HkCb_Dock_Call_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_Elapse_Time, PLUGIN_HkCb_Elapse_Time, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_Elapse_Time_AFTER, PLUGIN_HkCb_Elapse_Time_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LaunchPosHook, PLUGIN_LaunchPosHook, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkTimerCheckKick, PLUGIN_HkTimerCheckKick, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkTimerNPCAndF1Check, PLUGIN_HkTimerNPCAndF1Check, 0));
	// redundant, /help is also called by UserCmd_Process
	//p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Help, PLUGIN_UserCmd_Help, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&CmdHelp_Callback, PLUGIN_CmdHelp_Callback, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ExecuteCommandString_Callback, PLUGIN_ExecuteCommandString_Callback, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ProcessEvent_BEFORE, PLUGIN_ProcessEvent_BEFORE, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&Plugin_Communication_CallBack, PLUGIN_Plugin_Communication, 0));

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Update, PLUGIN_HkIServerImpl_Update, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SubmitChat, PLUGIN_HkIServerImpl_SubmitChat, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SubmitChat_AFTER, PLUGIN_HkIServerImpl_SubmitChat_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::PlayerLaunch, PLUGIN_HkIServerImpl_PlayerLaunch, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::PlayerLaunch_AFTER, PLUGIN_HkIServerImpl_PlayerLaunch_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::FireWeapon, PLUGIN_HkIServerImpl_FireWeapon, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::FireWeapon_AFTER, PLUGIN_HkIServerImpl_FireWeapon_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPMunitionCollision, PLUGIN_HkIServerImpl_SPMunitionCollision, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPMunitionCollision_AFTER, PLUGIN_HkIServerImpl_SPMunitionCollision_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPObjUpdate, PLUGIN_HkIServerImpl_SPObjUpdate, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPObjUpdate_AFTER, PLUGIN_HkIServerImpl_SPObjUpdate_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPObjCollision, PLUGIN_HkIServerImpl_SPObjCollision, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPObjCollision_AFTER, PLUGIN_HkIServerImpl_SPObjCollision_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::LaunchComplete, PLUGIN_HkIServerImpl_LaunchComplete, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::LaunchComplete_AFTER, PLUGIN_HkIServerImpl_LaunchComplete_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::CharacterSelect, PLUGIN_HkIServerImpl_CharacterSelect, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::CharacterSelect_AFTER, PLUGIN_HkIServerImpl_CharacterSelect_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::BaseEnter, PLUGIN_HkIServerImpl_BaseEnter, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::BaseEnter_AFTER, PLUGIN_HkIServerImpl_BaseEnter_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::BaseExit, PLUGIN_HkIServerImpl_BaseExit, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::BaseExit_AFTER, PLUGIN_HkIServerImpl_BaseExit_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::OnConnect, PLUGIN_HkIServerImpl_OnConnect, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::OnConnect_AFTER, PLUGIN_HkIServerImpl_OnConnect_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::DisConnect_AFTER, PLUGIN_HkIServerImpl_DisConnect_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::TerminateTrade, PLUGIN_HkIServerImpl_TerminateTrade, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::TerminateTrade_AFTER, PLUGIN_HkIServerImpl_TerminateTrade_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::InitiateTrade, PLUGIN_HkIServerImpl_InitiateTrade, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::InitiateTrade_AFTER, PLUGIN_HkIServerImpl_InitiateTrade_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ActivateEquip, PLUGIN_HkIServerImpl_ActivateEquip, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ActivateEquip_AFTER, PLUGIN_HkIServerImpl_ActivateEquip_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ActivateCruise, PLUGIN_HkIServerImpl_ActivateCruise, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ActivateCruise_AFTER, PLUGIN_HkIServerImpl_ActivateCruise_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ActivateThrusters, PLUGIN_HkIServerImpl_ActivateThrusters, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ActivateThrusters_AFTER, PLUGIN_HkIServerImpl_ActivateThrusters_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::GFGoodSell, PLUGIN_HkIServerImpl_GFGoodSell, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::GFGoodSell_AFTER, PLUGIN_HkIServerImpl_GFGoodSell_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::CharacterInfoReq, PLUGIN_HkIServerImpl_CharacterInfoReq, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::CharacterInfoReq_AFTER, PLUGIN_HkIServerImpl_CharacterInfoReq_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::JumpInComplete, PLUGIN_HkIServerImpl_JumpInComplete, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::JumpInComplete_AFTER, PLUGIN_HkIServerImpl_JumpInComplete_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SystemSwitchOutComplete, PLUGIN_HkIServerImpl_SystemSwitchOutComplete, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SystemSwitchOutComplete_AFTER, PLUGIN_HkIServerImpl_SystemSwitchOutComplete_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Login, PLUGIN_HkIServerImpl_Login, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Login_AFTER, PLUGIN_HkIServerImpl_Login_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::MineAsteroid, PLUGIN_HkIServerImpl_MineAsteroid, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::MineAsteroid_AFTER, PLUGIN_HkIServerImpl_MineAsteroid_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::GoTradelane, PLUGIN_HkIServerImpl_GoTradelane, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::GoTradelane_AFTER, PLUGIN_HkIServerImpl_GoTradelane_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::StopTradelane, PLUGIN_HkIServerImpl_StopTradelane, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::StopTradelane_AFTER, PLUGIN_HkIServerImpl_StopTradelane_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::AbortMission, PLUGIN_HkIServerImpl_AbortMission, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::AbortMission_AFTER, PLUGIN_HkIServerImpl_AbortMission_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::AcceptTrade, PLUGIN_HkIServerImpl_AcceptTrade, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::AcceptTrade_AFTER, PLUGIN_HkIServerImpl_AcceptTrade_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::AddTradeEquip, PLUGIN_HkIServerImpl_AddTradeEquip, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::AddTradeEquip_AFTER, PLUGIN_HkIServerImpl_AddTradeEquip_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::BaseInfoRequest, PLUGIN_HkIServerImpl_BaseInfoRequest, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::BaseInfoRequest_AFTER, PLUGIN_HkIServerImpl_BaseInfoRequest_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::CreateNewCharacter, PLUGIN_HkIServerImpl_CreateNewCharacter, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::CreateNewCharacter_AFTER, PLUGIN_HkIServerImpl_CreateNewCharacter_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::DelTradeEquip, PLUGIN_HkIServerImpl_DelTradeEquip, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::DelTradeEquip_AFTER, PLUGIN_HkIServerImpl_DelTradeEquip_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::DestroyCharacter, PLUGIN_HkIServerImpl_DestroyCharacter, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::DestroyCharacter_AFTER, PLUGIN_HkIServerImpl_DestroyCharacter_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::GFGoodBuy, PLUGIN_HkIServerImpl_GFGoodBuy, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::GFGoodBuy_AFTER, PLUGIN_HkIServerImpl_GFGoodBuy_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::GFGoodVaporized, PLUGIN_HkIServerImpl_GFGoodVaporized, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::GFGoodVaporized_AFTER, PLUGIN_HkIServerImpl_GFGoodVaporized_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::GFObjSelect, PLUGIN_HkIServerImpl_GFObjSelect, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::GFObjSelect_AFTER, PLUGIN_HkIServerImpl_GFObjSelect_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Hail, PLUGIN_HkIServerImpl_Hail, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Hail_AFTER, PLUGIN_HkIServerImpl_Hail_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::InterfaceItemUsed, PLUGIN_HkIServerImpl_InterfaceItemUsed, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::InterfaceItemUsed_AFTER, PLUGIN_HkIServerImpl_InterfaceItemUsed_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::JettisonCargo, PLUGIN_HkIServerImpl_JettisonCargo, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::JettisonCargo_AFTER, PLUGIN_HkIServerImpl_JettisonCargo_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::LocationEnter, PLUGIN_HkIServerImpl_LocationEnter, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::LocationEnter_AFTER, PLUGIN_HkIServerImpl_LocationEnter_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::LocationExit, PLUGIN_HkIServerImpl_LocationExit, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::LocationExit_AFTER, PLUGIN_HkIServerImpl_LocationExit_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::LocationInfoRequest, PLUGIN_HkIServerImpl_LocationInfoRequest, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::LocationInfoRequest_AFTER, PLUGIN_HkIServerImpl_LocationInfoRequest_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::MissionResponse, PLUGIN_HkIServerImpl_MissionResponse, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::MissionResponse_AFTER, PLUGIN_HkIServerImpl_MissionResponse_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqAddItem, PLUGIN_HkIServerImpl_ReqAddItem, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqAddItem_AFTER, PLUGIN_HkIServerImpl_ReqAddItem_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqChangeCash, PLUGIN_HkIServerImpl_ReqChangeCash, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqChangeCash_AFTER, PLUGIN_HkIServerImpl_ReqChangeCash_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqCollisionGroups, PLUGIN_HkIServerImpl_ReqCollisionGroups, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqCollisionGroups_AFTER, PLUGIN_HkIServerImpl_ReqCollisionGroups_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqEquipment, PLUGIN_HkIServerImpl_ReqEquipment, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqEquipment_AFTER, PLUGIN_HkIServerImpl_ReqEquipment_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqHullStatus, PLUGIN_HkIServerImpl_ReqHullStatus, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqHullStatus_AFTER, PLUGIN_HkIServerImpl_ReqHullStatus_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqModifyItem, PLUGIN_HkIServerImpl_ReqModifyItem, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqModifyItem_AFTER, PLUGIN_HkIServerImpl_ReqModifyItem_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqRemoveItem, PLUGIN_HkIServerImpl_ReqRemoveItem, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqRemoveItem_AFTER, PLUGIN_HkIServerImpl_ReqRemoveItem_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqSetCash, PLUGIN_HkIServerImpl_ReqSetCash, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqSetCash_AFTER, PLUGIN_HkIServerImpl_ReqSetCash_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqShipArch, PLUGIN_HkIServerImpl_ReqShipArch, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqShipArch_AFTER, PLUGIN_HkIServerImpl_ReqShipArch_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestBestPath, PLUGIN_HkIServerImpl_RequestBestPath, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestBestPath_AFTER, PLUGIN_HkIServerImpl_RequestBestPath_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestCancel, PLUGIN_HkIServerImpl_RequestCancel, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestCancel_AFTER, PLUGIN_HkIServerImpl_RequestCancel_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestCreateShip, PLUGIN_HkIServerImpl_RequestCreateShip, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestCreateShip_AFTER, PLUGIN_HkIServerImpl_RequestCreateShip_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestEvent, PLUGIN_HkIServerImpl_RequestEvent, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestEvent_AFTER, PLUGIN_HkIServerImpl_RequestEvent_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestGroupPositions, PLUGIN_HkIServerImpl_RequestGroupPositions, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestGroupPositions_AFTER, PLUGIN_HkIServerImpl_RequestGroupPositions_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestPlayerStats, PLUGIN_HkIServerImpl_RequestPlayerStats, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestPlayerStats_AFTER, PLUGIN_HkIServerImpl_RequestPlayerStats_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestRankLevel, PLUGIN_HkIServerImpl_RequestRankLevel, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestRankLevel_AFTER, PLUGIN_HkIServerImpl_RequestRankLevel_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestTrade, PLUGIN_HkIServerImpl_RequestTrade, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestTrade_AFTER, PLUGIN_HkIServerImpl_RequestTrade_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPRequestInvincibility, PLUGIN_HkIServerImpl_SPRequestInvincibility, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPRequestInvincibility_AFTER, PLUGIN_HkIServerImpl_SPRequestInvincibility_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPRequestUseItem, PLUGIN_HkIServerImpl_SPRequestUseItem, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPRequestUseItem_AFTER, PLUGIN_HkIServerImpl_SPRequestUseItem_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPScanCargo, PLUGIN_HkIServerImpl_SPScanCargo, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPScanCargo_AFTER, PLUGIN_HkIServerImpl_SPScanCargo_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SetInterfaceState, PLUGIN_HkIServerImpl_SetInterfaceState, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SetInterfaceState_AFTER, PLUGIN_HkIServerImpl_SetInterfaceState_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SetManeuver, PLUGIN_HkIServerImpl_SetManeuver, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SetManeuver_AFTER, PLUGIN_HkIServerImpl_SetManeuver_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SetTarget, PLUGIN_HkIServerImpl_SetTarget, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SetTarget_AFTER, PLUGIN_HkIServerImpl_SetTarget_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SetTradeMoney, PLUGIN_HkIServerImpl_SetTradeMoney, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SetTradeMoney_AFTER, PLUGIN_HkIServerImpl_SetTradeMoney_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SetVisitedState, PLUGIN_HkIServerImpl_SetVisitedState, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SetVisitedState_AFTER, PLUGIN_HkIServerImpl_SetVisitedState_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SetWeaponGroup, PLUGIN_HkIServerImpl_SetWeaponGroup, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SetWeaponGroup_AFTER, PLUGIN_HkIServerImpl_SetWeaponGroup_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Shutdown, PLUGIN_HkIServerImpl_Shutdown, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Startup, PLUGIN_HkIServerImpl_Startup, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Startup_AFTER, PLUGIN_HkIServerImpl_Startup_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::StopTradeRequest, PLUGIN_HkIServerImpl_StopTradeRequest, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::StopTradeRequest_AFTER, PLUGIN_HkIServerImpl_StopTradeRequest_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::TractorObjects, PLUGIN_HkIServerImpl_TractorObjects, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::TractorObjects_AFTER, PLUGIN_HkIServerImpl_TractorObjects_AFTER, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::TradeResponse, PLUGIN_HkIServerImpl_TradeResponse, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::TradeResponse_AFTER, PLUGIN_HkIServerImpl_TradeResponse_AFTER, 0));

    return p_PI;
}