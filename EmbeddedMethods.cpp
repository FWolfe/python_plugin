#include "headers.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
Embedded Methods for the python FLHook module
*/

static PyObject* emb_ConPrint(PyObject *self, PyObject *pArgs)
{
	PyObject *pText;
	if (!PyArg_ParseTuple(pArgs, "O", &pText))
		return NULL;
	ConPrint(pytows(pText));
	Py_RETURN_NONE;
}
static PyObject* emb_GetClientInfo(PyObject *self, PyObject *pArgs)
{
	uint iClientID;
	if (!PyArg_ParseTuple(pArgs, "I", &iClientID))
		return NULL;
	return ToPython(&ClientInfo[iClientID]);
}
static PyObject* emb_PrintUserCmdText(PyObject *self, PyObject *pArgs)
{
	uint iClientID;
	PyObject* pText;
	if (!PyArg_ParseTuple(pArgs, "IO", &iClientID, &pText))
		return NULL;
	PrintUserCmdText(iClientID, pytows(pText));
	Py_RETURN_NONE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HkFuncMsg
static PyObject* emb_HkMsg(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharName, *pText;
	if (!PyArg_ParseTuple(pArgs, "OO", &pCharName, &pText))
		return NULL;
	if (RaisePyException(HkMsg(pytows(pCharName), pytows(pText))))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkMsgS(PyObject *self, PyObject *pArgs)
{
	PyObject *pSystem, *pText;
	if (!PyArg_ParseTuple(pArgs, "OO", &pSystem, &pText))
		return NULL;
	if (RaisePyException(HkMsgS(pytows(pSystem), pytows(pText))))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkMsgU(PyObject *self, PyObject *pArgs)
{
	PyObject *pText;
	if (!PyArg_ParseTuple(pArgs, "O", &pText))
		return NULL;
	if (RaisePyException(HkMsgU(pytows(pText))))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkFMsg(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharName, *pText;
	if (!PyArg_ParseTuple(pArgs, "OO", &pCharName, &pText))
		return NULL;
	if (RaisePyException(HkFMsg(pytows(pCharName), pytows(pText))))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkFMsgS(PyObject *self, PyObject *pArgs)
{
	PyObject *pSystem, *pText;
	if (!PyArg_ParseTuple(pArgs, "OO", &pSystem, &pText))
		return NULL;
	if (RaisePyException(HkFMsgS(pytows(pSystem), pytows(pText))))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkFMsgU(PyObject *self, PyObject *pArgs)
{
	PyObject *pText;
	if (!PyArg_ParseTuple(pArgs, "O", &pText))
		return NULL;
	if (RaisePyException(HkFMsgU(pytows(pText))))
		return NULL;
	Py_RETURN_NONE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HkFuncLog
//void AddDebugLog(const char *szString, ...)
static PyObject* emb_AddLog(PyObject *self, PyObject *pArgs)
{
	PyObject *pText;
	if (!PyArg_ParseTuple(pArgs, "O", &pText))
		return NULL;
	const char* cString = PyString_AsString(pText);
	AddLog(cString);
	Py_RETURN_NONE;
}
//void HkHandleCheater(uint iClientID, bool bBan, wstring wscReason, ...)
//bool HkAddCheaterLog(const uint &iClientID, const wstring &wscReason)
//bool HkAddKickLog(uint iClientID, wstring wscReason, ...)
//bool HkAddConnectLog(uint iClientID, wstring wscReason, ...)
//void HkAddAdminCmdLog(const char *szString, ...)
//void HkAddSocketCmdLog(const char *szString, ...)
//void HkAddUserCmdLog(const char *szString, ...)
//void HkAddPerfTimerLog(const char *szString, ...)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HkFuncOther
static PyObject* emb_HkGetPlayerIP(PyObject *self, PyObject *pArgs)
{
	uint iClientID;
	if (!PyArg_ParseTuple(pArgs, "I", &iClientID))
		return NULL;
	wstring wscIP;
	HkGetPlayerIP(iClientID, wscIP);
	return Py_BuildValue("s", wstos(wscIP));
}

//HK_ERROR HkGetPlayerInfo(const wstring &wscCharname, HKPLAYERINFO &pi, bool bAlsoCharmenu)
//list<HKPLAYERINFO> HkGetPlayers()
//HK_ERROR HkGetConnectionStats(uint iClientID, DPN_CONNECTION_INFO &ci)
static PyObject* emb_HkSetAdmin(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname, *pRights;
	if (!PyArg_ParseTuple(pArgs, "O", &pCharname, &pRights))
		return NULL;
	if (RaisePyException(HkSetAdmin(pytows(pCharname), pytows(pRights))))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkGetAdmin(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname;
	if (!PyArg_ParseTuple(pArgs, "O", &pCharname))
		return NULL;
	wstring wscRights;
	if (RaisePyException(HkGetAdmin(pytows(pCharname), wscRights)))
		return NULL;
	return Py_BuildValue("s", wstos(wscRights));
}
static PyObject* emb_HkDelAdmin(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname;
	if (!PyArg_ParseTuple(pArgs, "O", &pCharname))
		return NULL;
	if (RaisePyException(HkDelAdmin(pytows(pCharname))))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkChangeNPCSpawn(PyObject *self, PyObject *pArgs)
{
	int bDisable;
	if (!PyArg_ParseTuple(pArgs, "i", &bDisable))
		return NULL;
	//ConPrint(bDisable ? L"true\n" : L"false\n");
	if (RaisePyException(HkChangeNPCSpawn(bDisable ? true : false)))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkGetBaseStatus(PyObject *self, PyObject *pArgs)
{
	PyObject *pBasename;
	if (!PyArg_ParseTuple(pArgs, "O", &pBasename))
		return NULL;
	float fHealth, fMaxHealth;
	if (RaisePyException(HkGetBaseStatus(pytows(pBasename), fHealth, fMaxHealth)))
		return NULL;
	return Py_BuildValue("ff", fHealth, fMaxHealth);
}
//Fuse* HkGetFuseFromID(uint iFuseID)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HkFuncPlayers
static PyObject* emb_HkGetCash(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname;
	if (!PyArg_ParseTuple(pArgs, "O", &pCharname))
		return NULL;
	int iCash = 0;
	if (RaisePyException(HkGetCash(pytows(pCharname), iCash)))
		return NULL;
	return Py_BuildValue("i", iCash);
}
static PyObject* emb_HkAddCash(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname;
	int iCash;
	if (!PyArg_ParseTuple(pArgs, "Oi", &pCharname, &iCash))
		return NULL;
	if (RaisePyException(HkAddCash(pytows(pCharname), iCash)))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkKick(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname;
	if (!PyArg_ParseTuple(pArgs, "O", &pCharname))
		return NULL;
	if (RaisePyException(HkKick(pytows(pCharname))))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkKickReason(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname, *pReason;
	if (!PyArg_ParseTuple(pArgs, "OO", &pCharname, &pReason))
		return NULL;
	if (RaisePyException(HkKickReason(pytows(pCharname), pytows(pReason))))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkBan(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname;
	int bBan;
	if (!PyArg_ParseTuple(pArgs, "Oi", &pCharname, &bBan))
		return NULL;
	if (RaisePyException(HkBan(pytows(pCharname), bBan ? true : false)))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkBeam(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname, *pBasename;
	if (!PyArg_ParseTuple(pArgs, "OO", &pCharname, &pBasename))
		return NULL;
	if (RaisePyException(HkBeam(pytows(pCharname), pytows(pBasename))))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkSaveChar(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname;
	if (!PyArg_ParseTuple(pArgs, "O", &pCharname))
		return NULL;
	if (RaisePyException(HkSaveChar(pytows(pCharname))))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkEnumCargo(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname;
	if (!PyArg_ParseTuple(pArgs, "O", &pCharname))
		return NULL;
	list<CARGO_INFO> lstCargo;
	int iRemainingHoldSize = 0;
	if (RaisePyException(HkEnumCargo(pytows(pCharname), lstCargo, iRemainingHoldSize)))
		return NULL;
	return ToPython(lstCargo);
}
static PyObject* emb_HkRemoveCargo(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname;
	uint iID;
	int iCount;
	if (!PyArg_ParseTuple(pArgs, "OIi", &pCharname, &iID, &iCount))
		return NULL;
	if (RaisePyException(HkRemoveCargo(pytows(pCharname), iID, iCount)))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkAddCargo(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname, *pGood;
	int iCount;
	int bMission;
	if (!PyArg_ParseTuple(pArgs, "OOii", &pCharname, &pGood, &iCount, &bMission))
		return NULL;
	if (RaisePyException(HkAddCargo(pytows(pCharname), pytows(pGood), iCount, bMission ? true : false)))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkRename(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname, *pNewName;
	int bBan;
	if (PyArg_ParseTuple(pArgs, "OOi", &pCharname, &pNewName, &bBan))
		return NULL;
	if (RaisePyException(HkRename(pytows(pCharname), pytows(pNewName), bBan ? true : false)))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkMsgAndKick(PyObject *self, PyObject *pArgs)
{
	PyObject *pReason;
	uint iClientID, iIntervall;
	if (!PyArg_ParseTuple(pArgs, "IOI", &iClientID, &pReason, &iIntervall))
		return NULL;
	if (RaisePyException(HkMsgAndKick(iClientID, pytows(pReason), iIntervall)))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkKill(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname;
	if (!PyArg_ParseTuple(pArgs, "O", &pCharname))
		return NULL;
	if (RaisePyException(HkKill(pytows(pCharname))))
		return NULL;
	Py_RETURN_NONE;
}
//HK_ERROR HkGetReservedSlot(const wstring &wscCharname, bool &bResult)
//HK_ERROR HkSetReservedSlot(const wstring &wscCharname, bool bReservedSlot)
//int HkPlayerAutoBuyGetCount(list<CARGO_INFO> &lstCargo, uint iItemArchID)
//void HkPlayerAutoBuy(uint iClientID, uint iBaseID)
//HK_ERROR HkResetRep(const wstring &wscCharname)
static PyObject* emb_HkSetRep(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname, *pFaction;
	float fValue;
	if (!PyArg_ParseTuple(pArgs, "OOf", &pCharname, &pFaction, &fValue))
		return NULL;
	if (RaisePyException(HkSetRep(pytows(pCharname), pytows(pFaction), fValue)))
		return NULL;
	Py_RETURN_NONE;
}
static PyObject* emb_HkGetRep(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname, *pFaction;
	if (!PyArg_ParseTuple(pArgs, "OO", &pCharname, &pFaction))
		return NULL;
	float fRep = 0.0;
	if (RaisePyException(HkGetRep(pytows(pCharname), pytows(pFaction), fRep)))
		return NULL;
	return Py_BuildValue("f", fRep);
}
//HK_ERROR HkGetGroupMembers(const wstring &wscCharname, list<GROUP_MEMBER> &lstMembers)
//HK_ERROR HkReadCharFile(const wstring &wscCharname, list<wstring> &lstOutput)
//HK_ERROR HkWriteCharFile(const wstring &wscCharname, wstring wscData)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HkFuncTools
//uint HkGetClientIdFromAccount(CAccount *acc)
//uint HkGetClientIdFromPD(struct PlayerData *pPD)
//CAccount* HkGetAccountByCharname(const wstring &wscCharname)
static PyObject* emb_HkGetClientIdFromCharname(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname;
	if (!PyArg_ParseTuple(pArgs, "O", &pCharname))
		return NULL;
	return Py_BuildValue("I", HkGetClientIdFromCharname(pytows(pCharname)));
}
//wstring HkGetAccountID(CAccount *acc)
//bool HkIsEncoded(const string &scFilename)
static PyObject* emb_HkIsInCharSelectMenu(PyObject *self, PyObject *pArgs)
{
	uint iClientID;
	if (!PyArg_ParseTuple(pArgs, "I", &iClientID))
		return NULL;
	return Py_BuildValue("O", PY_BOOL(HkIsInCharSelectMenu(iClientID)));
}
static PyObject* emb_HkIsValidClientID(PyObject *self, PyObject *pArgs)
{
	uint iClientID;
	if (!PyArg_ParseTuple(pArgs, "I", &iClientID))
		return NULL;
	return Py_BuildValue("O", PY_BOOL(HkIsValidClientID(iClientID)));
}
//HK_ERROR HkResolveId(const wstring &wscCharname, uint &iClientID)
//HK_ERROR HkResolveShortCut(const wstring &wscShortcut, uint &_iClientID)
static PyObject* emb_HkGetClientIDByShip(PyObject *self, PyObject *pArgs)
{
	uint iShip;
	if (!PyArg_ParseTuple(pArgs, "I", &iShip))
		return NULL;
	return Py_BuildValue("I", HkGetClientIDByShip(iShip));
}
static PyObject* emb_HkGetAccountDirName(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname;
	if (!PyArg_ParseTuple(pArgs, "O", &pCharname))
		return NULL;
	wstring wscAccountDir;
	if (RaisePyException(HkGetAccountDirName(pytows(pCharname), wscAccountDir)))
		return NULL;
	return Py_BuildValue("N", ToPython(wscAccountDir));
}
static PyObject* emb_HkGetCharFileName(PyObject *self, PyObject *pArgs)
{
	PyObject *pCharname;
	if (!PyArg_ParseTuple(pArgs, "O", &pCharname))
		return NULL;
	wstring wscFileName;
	if (RaisePyException(HkGetCharFileName(pytows(pCharname), wscFileName)))
		return NULL;
	return Py_BuildValue("N", ToPython(wscFileName));
}
static PyObject* emb_HkGetBaseNickByID(PyObject *self, PyObject *pArgs)
{
	uint iBaseID;
	if (!PyArg_ParseTuple(pArgs, "I", &iBaseID))
		return NULL;
	return Py_BuildValue("N", ToPython(HkGetBaseNickByID(iBaseID)));
}
static PyObject* emb_HkGetSystemNickByID(PyObject *self, PyObject *pArgs)
{
	uint iSystemID;
	if (!PyArg_ParseTuple(pArgs, "I", &iSystemID))
		return NULL;
	return Py_BuildValue("N", ToPython(HkGetSystemNickByID(iSystemID)));
}
static PyObject* emb_HkGetPlayerSystem(PyObject *self, PyObject *pArgs)
{
	uint iClientID;
	if (!PyArg_ParseTuple(pArgs, "I", &iClientID))
		return NULL;
	return Py_BuildValue("N", ToPython(HkGetPlayerSystem(iClientID)));
}
//void HkGetItemsForSale(uint iBaseID, list<uint> &lstItems)
//IObjInspectImpl* HkGetInspect(uint iClientID)
//ENGINE_STATE HkGetEngineState(uint iClientID)
//EQ_TYPE HkGetEqType(Archetype::Equipment *eq)
static PyObject* emb_HkGetCharnameFromClientId(PyObject *self, PyObject *pArgs)
{
	uint iClientID;
	if (!PyArg_ParseTuple(pArgs, "I", &iClientID))
		return NULL;
	wchar_t *wszActiveCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
	if (!wszActiveCharname)
		Py_RETURN_NONE;
	wstring wscActiveCharname = wszActiveCharname;
	return Py_BuildValue("N", ToPython(wscActiveCharname));
}



static PyMethodDef FLHookMethods[] = {
	{ "ConPrint", emb_ConPrint, METH_VARARGS, "ConPrint(str text)" },
	{ "GetClientInfo", emb_GetClientInfo, METH_VARARGS, "namedtuple info = GetClientInfo(int client_id)" },
	{ "PrintUserCmdText", emb_PrintUserCmdText, METH_VARARGS, "PrintUserCmdText(int client_id, str text)" },

	// HkFuncMsg
	{ "HkMsg", emb_HkMsg, METH_VARARGS, "HkMsg(str charname, str text)" },
	{ "HkMsgS", emb_HkMsgS, METH_VARARGS, "HkMsgS(str systemname, str text)" },
	{ "HkMsgU", emb_HkMsgU, METH_VARARGS, "HkMsgU(str text)" },
	{ "HkFMsg", emb_HkFMsg, METH_VARARGS, "HkFMsg(str charname, str text)" },
	{ "HkFMsgS", emb_HkFMsgS, METH_VARARGS, "HkFMsgS(str systemname, str text)" },
	{ "HkFMsgU", emb_HkFMsgU, METH_VARARGS, "HkFMsgU(str text)" },

	// HkFuncLog
	{ "AddLog", emb_AddLog, METH_VARARGS, "AddLog(str text)" },

	// HkFuncOther
	{ "HkGetPlayerIP", emb_HkGetPlayerIP, METH_VARARGS, "str ip = HkGetPlayerIP(int client_id)" },
	{ "HkSetAdmin", emb_HkSetAdmin, METH_VARARGS, "HkSetAdmin(str charname, str rights)" },
	{ "HkGetAdmin", emb_HkGetAdmin, METH_VARARGS, "str rights = HkGetAdmin(str charname)" },
	{ "HkDelAdmin", emb_HkDelAdmin, METH_VARARGS, "HkDelAdmin(str charname)" },
	{ "HkChangeNPCSpawn", emb_HkChangeNPCSpawn, METH_VARARGS, "HkChangeNPCSpawn(bool disable)" },
	{ "HkGetBaseStatus", emb_HkGetBaseStatus, METH_VARARGS, "(float, float) = HkGetBaseStatus(str basename)" },

	// HkFuncPlayers
	{ "HkGetCash", emb_HkGetCash, METH_VARARGS, "int = HkGetCash(str charname)" },
	{ "HkAddCash", emb_HkAddCash, METH_VARARGS, "HkAddCash(str charname, int amount)" },
	{ "HkKick", emb_HkKick, METH_VARARGS, "HkKick(str charname)" },
	{ "HkKickReason", emb_HkKickReason, METH_VARARGS, "HkKickReason(str charname, str reason)" },
	{ "HkBan", emb_HkBan, METH_VARARGS, "HkBan(str charname, bool ban)" },
	{ "HkBeam", emb_HkBeam, METH_VARARGS, "HkBeam(str charname, str basename)" },
	{ "HkSaveChar", emb_HkSaveChar, METH_VARARGS, "HkSaveChar(str charname)" },
	{ "HkEnumCargo", emb_HkEnumCargo, METH_VARARGS, "HkEnumCargo(str charname)" },
	{ "HkRemoveCargo", emb_HkRemoveCargo, METH_VARARGS, "HkRemoveCargo(str charname, int id, int count)" },
	{ "HkAddCargo", emb_HkAddCargo, METH_VARARGS, "HkRemoveCargo(str charname, str good, int count, bool mission)" },
	{ "HkRename", emb_HkRename, METH_VARARGS, "HkBeam(str charname, str newname, bool delete_only)" },
	{ "HkMsgAndKick", emb_HkMsgAndKick, METH_VARARGS, "HkMsgAndKick(int client_id, str reason, int interval)" },
	{ "HkKill", emb_HkKill, METH_VARARGS, "HkKill(str charname)" },
	{ "HkSetRep", emb_HkSetRep, METH_VARARGS, "HkSetRep(str charname, str faction, float value)" },
	{ "HkGetRep", emb_HkGetRep, METH_VARARGS, "float value = HkGetRep(str charname, str faction)" },

	// HkFuncTools
	{ "HkGetClientIdFromCharname", emb_HkGetClientIdFromCharname, METH_VARARGS, "int client_id = HkGetClientIdFromCharname(str charname)" },
	{ "HkIsInCharSelectMenu", emb_HkIsInCharSelectMenu, METH_VARARGS, "bool in_menu = HkIsInCharSelectMenu(int client_id)" },
	{ "HkIsValidClientID", emb_HkIsValidClientID, METH_VARARGS, "bool valid = HkIsValidClientID(int client_id)" },
	{ "HkGetClientIDByShip", emb_HkGetClientIDByShip, METH_VARARGS, "int client_id = HkGetClientIDByShip(int ship_id)" },
	{ "HkGetAccountDirName", emb_HkGetAccountDirName, METH_VARARGS, "str dirname = HkGetAccountDirName(str charname)" },
	{ "HkGetCharFileName", emb_HkGetCharFileName, METH_VARARGS, "str filename = HkGetCharFileName(str charname)" },
	{ "HkGetBaseNickByID", emb_HkGetBaseNickByID, METH_VARARGS, "str basename = HkGetBaseNickByID(int base_id)" },
	{ "HkGetSystemNickByID", emb_HkGetSystemNickByID, METH_VARARGS, "str systemname = HkGetSystemNickByID(int system_id)" },
	{ "HkGetPlayerSystem", emb_HkGetPlayerSystem, METH_VARARGS, "str systemname = HkGetPlayerSystem(int client_id)" },

	// Custom
	{ "HkGetCharnameFromClientId", emb_HkGetCharnameFromClientId, METH_VARARGS, "str charname = HkGetCharnameFromClientId(int client_id)" },

	{ NULL, NULL, 0, NULL }
};


static PyObject* emb_Vector_GetAttr(PyObject *self, PyObject *pArgs)
{
	GET_CAPSULE_DATA(Vector);
	switch (iFuncID) {
	case 0:
		return Py_BuildValue("f", ptr->x);
	case 1:
		return Py_BuildValue("f", ptr->y);
	case 2:
		return Py_BuildValue("f", ptr->z);
	}
	Py_RETURN_NONE;
}
static PyObject* emb_Vector_SetAttr(PyObject *self, PyObject *pArgs)
{
	GET_CAPSULE_DATA(Vector);
	float nv;
	if (!PyArg_ParseTuple(pFuncArgs, "f", &nv))
		return NULL;
	switch (iFuncID) {
	case 0:
		ptr->x = nv;
	case 1:
		ptr->y = nv;
	case 2:
		ptr->z = nv;
	}
	Py_RETURN_NONE;
}
static PyObject* emb_Quaternion_GetAttr(PyObject *self, PyObject *pArgs)
{
	GET_CAPSULE_DATA(Quaternion);
	switch (iFuncID) {
	case 0:
		return Py_BuildValue("f", ptr->w);
	case 1:
		return Py_BuildValue("f", ptr->x);
	case 2:
		return Py_BuildValue("f", ptr->y);
	case 3:
		return Py_BuildValue("f", ptr->z);
	}
	Py_RETURN_NONE;
}
static PyObject* emb_Quaternion_SetAttr(PyObject *self, PyObject *pArgs)
{
	GET_CAPSULE_DATA(Quaternion);
	float nv;
	if (!PyArg_ParseTuple(pFuncArgs, "f", &nv))
		return NULL;
	switch (iFuncID) {
	case 0:
		ptr->w = nv;
	case 1:
		ptr->x = nv;
	case 2:
		ptr->y = nv;
	case 3:
		ptr->z = nv;
	}
	Py_RETURN_NONE;
}
/*
static PyObject* emb_SSPObjUpdateInfo_GetAttr(PyObject *self, PyObject *pArgs)
{
	GET_CAPSULE_DATA(SSPObjUpdateInfo);
	switch (iFuncID) {
	case 0:
		return Py_BuildValue("I", ptr->iShip);
	case 1:
		return Py_BuildValue("N", ToPython(&ptr->vDir));
	case 2:
		return Py_BuildValue("N", ToPython(&ptr->vPos));
	case 3:
		return Py_BuildValue("f", ptr->fTimestamp);
	case 4:
		return Py_BuildValue("f", ptr->fDunno);
	case 5:
		return Py_BuildValue("f", ptr->throttle);
	case 6:
		return Py_BuildValue("b", ptr->cState);
	}
	Py_RETURN_NONE;
}
// this is a const anyways so useless func
static PyObject* emb_SSPObjUpdateInfo_SetAttr(PyObject *self, PyObject *pArgs)
{
	GET_CAPSULE_DATA(SSPObjUpdateInfo);
	float nf;
	uint ni;
	char nc;
	switch (iFuncID) {
	case 0:
		PyArg_ParseTuple(pFuncArgs, "I", &ni);
		ptr->iShip = ni;
	case 1:
		Py_RETURN_NONE; // Not Supported, ptr->vDir
	case 2:
		Py_RETURN_NONE; // Not Supported, ptr->vPos
	case 3:
		PyArg_ParseTuple(pFuncArgs, "f", &nf);
		ptr->fTimestamp = nf;
	case 4:
		PyArg_ParseTuple(pFuncArgs, "f", &nf);
		ptr->fDunno = nf;
	case 5:
		PyArg_ParseTuple(pFuncArgs, "f", &nf);
		ptr->throttle = nf;
	case 6:
		PyArg_ParseTuple(pFuncArgs, "b", &nc); // not sure about this one
		ptr->cState = nc;
	}
	Py_RETURN_NONE;
}

static PyObject* emb_SSPObjCollisionInfo_GetAttr(PyObject *self, PyObject *pArgs)
{
	GET_CAPSULE_DATA(SSPObjUpdateInfo);
	Py_RETURN_NONE;
}
static PyObject* emb_SSPObjCollisionInfo_SetAttr(PyObject *self, PyObject *pArgs)
{
	GET_CAPSULE_DATA(SSPObjCollisionInfo);
	Py_RETURN_NONE;
}
*/
static PyObject* emb_DamageList_Functions(PyObject *self, PyObject *pArgs)
{
	GET_CAPSULE_DATA(DamageList);
	switch (iFuncID) {
	case 0:
		return Py_BuildValue("s", ptr->DmgCauseToString(ptr->get_cause()));
	case 1:
		Py_RETURN_NONE; // NotImplemented, ->add_damage_entry()
	case 2:
		return Py_BuildValue("I", ptr->get_cause());
	case 3:
		ushort subobj; // funny, this should break according to google, yet compiles on vc14 without { }
		if (!PyArg_ParseTuple(pFuncArgs, "H", &subobj))
			return NULL;
		return Py_BuildValue("f", ptr->get_hit_pts_left(subobj));
	case 4:
		return Py_BuildValue("I", ptr->get_inflictor_id());
	case 5:
		return Py_BuildValue("I", ptr->get_inflictor_owner_player());
	case 6:
		return Py_BuildValue("O", PY_BOOL(ptr->is_destroyed()));
	case 7:
		return Py_BuildValue("O", PY_BOOL(ptr->is_inflictor_a_player()));
	case 8:
		Py_RETURN_NONE; // NotImplemented, ->set_cause()
	case 9:
		Py_RETURN_NONE; // NotImplemented, ->set_destroyed()
	case 10:
		uint obj_id;
		if (!PyArg_ParseTuple(pFuncArgs, "I", &obj_id))
			return NULL;
		ptr->set_inflictor_id(obj_id);
		return Py_BuildValue("O", Py_None);
	case 11:
		uint client_id;
		if (!PyArg_ParseTuple(pFuncArgs, "I", &client_id))
			return NULL;
		ptr->set_inflictor_id(client_id);
		Py_RETURN_NONE;
	default:
		Py_RETURN_NONE;
	}
}
static PyObject* emb_DamageList_GetAttr(PyObject *self, PyObject *pArgs)
{
	GET_CAPSULE_DATA(DamageList);
	switch (iFuncID) {
	case 0:
		return Py_BuildValue("I", ptr->iDunno1);
	case 1:
		return Py_BuildValue("N", ptr->damageentries);
	case 2:
		return Py_BuildValue("O", PY_BOOL(ptr->bDestroyed));
	case 3:
		return Py_BuildValue("I", ptr->iDunno2);
	case 4:
		return Py_BuildValue("I", ptr->iInflictorID);
	case 5:
		return Py_BuildValue("I", ptr->iInflictorPlayerID);
	default:
		Py_RETURN_NONE;
	}
}
static PyObject* emb_ClientInfo_GetAttr(PyObject *self, PyObject *pArgs)
{
	GET_CAPSULE_DATA(CLIENT_INFO);
	switch (iFuncID) {
	case 0:
		return Py_BuildValue("I", ptr->iShip);
	case 1:
		return Py_BuildValue("I", ptr->iShipOld);
	case 2:
		return Py_BuildValue("K", ptr->tmSpawnTime);
	case 3:
		return Py_BuildValue("N", ToPython(&ptr->dmgLast));
	case 4:
		return Py_BuildValue("O", Py_None); // NotImplemented, ->lstMoneyFix
	case 5:
		return Py_BuildValue("I", ptr->iTradePartner);
	case 6:
		return Py_BuildValue("O", PY_BOOL(ptr->bCruiseActivated));
	case 7:
		return Py_BuildValue("O", PY_BOOL(ptr->bThrusterActivated));
	case 8:
		return Py_BuildValue("O", PY_BOOL(ptr->bEngineKilled));
	case 9:
		return Py_BuildValue("O", PY_BOOL(ptr->bTradelane));
	case 10:
		return Py_BuildValue("I", ptr->iBaseEnterTime);
	case 11:
		return Py_BuildValue("I", ptr->iCharMenuEnterTime);
	case 12:
		return Py_BuildValue("K", ptr->tmKickTime);
	case 13:
		return Py_BuildValue("I", ptr->iLastExitedBaseID);
	case 14:
		return Py_BuildValue("O", PY_BOOL(ptr->bDisconnected));
	case 15:
		return Py_BuildValue("O", PY_BOOL(ptr->bCharSelected));
	case 16:
		return Py_BuildValue("K", ptr->tmF1Time); // mstime
	case 17:
		return Py_BuildValue("K", ptr->tmF1TimeDisconnect); // mstime ;
	case 18:
		return Py_BuildValue("O", Py_None); // NotImplemented, list<IGNORE_INFO> lstIgnore;
	case 19:
		return Py_BuildValue("O", Py_None); // NotImplemented, DIEMSGTYPE dieMsg;
	case 20:
		return Py_BuildValue("O", Py_None); // NotImplemented, CHATSIZE dieMsgSize;
	case 21:
		return Py_BuildValue("O", Py_None); // NotImplemented, CHATSTYLE dieMsgStyle;
	case 22:
		return Py_BuildValue("O", Py_None); // NotImplemented, CHATSIZE chatSize;
	case 23:
		return Py_BuildValue("O", Py_None); // NotImplemented, CHATSTYLE chatStyle;
	case 24:
		return Py_BuildValue("O", PY_BOOL(ptr->bAutoBuyMissiles));
	case 25:
		return Py_BuildValue("O", PY_BOOL(ptr->bAutoBuyMines));
	case 26:
		return Py_BuildValue("O", PY_BOOL(ptr->bAutoBuyTorps));
	case 27:
		return Py_BuildValue("O", PY_BOOL(ptr->bAutoBuyCD));
	case 28:
		return Py_BuildValue("O", PY_BOOL(ptr->bAutoBuyCM));
	case 29:
		return Py_BuildValue("O", PY_BOOL(ptr->bAutoBuyReload));
	case 30:
		return Py_BuildValue("I", ptr->iKillsInARow);
	case 31:
		return Py_BuildValue("I", ptr->iConnects); // incremented when player connects
	case 32:
		return Py_BuildValue("N", ToPython(ptr->wscHostname));
	case 33:
		return Py_BuildValue("O", PY_BOOL(ptr->bSpawnProtected));
	case 34:
		return Py_BuildValue("O", PY_BOOL(ptr->bUseServersideHitDetection));
	case 35:
		return Py_BuildValue("O", Py_None); // NotImplemented, byte unused_data[127];
	default:
		Py_RETURN_NONE;
	}
}

static PyMethodDef FLHookClassesMethods[] = {
	{ "Vector_GetAttr", emb_Vector_GetAttr, METH_VARARGS, "" },
	{ "Vector_SetAttr", emb_Vector_SetAttr, METH_VARARGS, "" },
	{ "Quaternion_GetAttr", emb_Quaternion_GetAttr, METH_VARARGS, "" },
	{ "Quaternion_SetAttr", emb_Quaternion_SetAttr, METH_VARARGS, "" },
	//{ "SSPObjUpdateInfo_GetAttr", emb_SSPObjUpdateInfo_GetAttr, METH_VARARGS, "" },
	//{ "SSPObjUpdateInfo_SetAttr", emb_SSPObjUpdateInfo_SetAttr, METH_VARARGS, "" },
	//{ "SSPObjCollisionInfo_GetAttr", emb_SSPObjCollisionInfo_GetAttr, METH_VARARGS, "" },
	//{ "SSPObjCollisionInfo_SetAttr", emb_SSPObjCollisionInfo_SetAttr, METH_VARARGS, "" },

	{ "DamageList_Functions", emb_DamageList_Functions, METH_VARARGS, "" },
	{ "DamageList_GetAttr", emb_DamageList_GetAttr, METH_VARARGS, "" },
	{ "ClientInfo_GetAttr", emb_ClientInfo_GetAttr, METH_VARARGS, "" },
	{ NULL, NULL, 0, NULL }
};





/*
static PyObject* emb_ActivateEquip(PyObject *self, PyObject *pArgs)
{
	uint iClientID;
	PyObject* pActivateEq;
	if (!PyArg_ParseTuple(pArgs, "IO", &iClientID, &pActivateEq))
		return NULL;
	XActivateEquip ActivateEq;
	uint iSpaceID;
	ushort sID;
	int bActivate;
	PyArg_ParseTuple(pActivateEq, "IHi", &iSpaceID, &sID, &bActivate);
	ActivateEq.iSpaceID = iSpaceID;
	ActivateEq.sID = sID;
	ActivateEq.bActivate = bActivate ? true : false;
	Server.ActivateEquip(iClientID, ActivateEq);
	ConPrint(L"Activating....\n");
	Py_RETURN_NONE;
}

static PyMethodDef ServerMethods[] = {
	{ "ActivateEquip", emb_ActivateEquip, METH_VARARGS, "" },
	{ NULL, NULL, 0, NULL }
};
*/

void BuildEmbedded()
{
	// initialize embedded modules
	PyObject *pHook = Py_InitModule("FLHook", FLHookMethods);
	Py_InitModule("FLHookClasses", FLHookClassesMethods);
	//Py_InitModule("Server", ServerMethods);

	// Exception Building
	pException = PyErr_NewException("FLHook.Error", NULL, NULL);
	Py_INCREF(pException); // we need to manually inc the ref count here due to the next line
	PyModule_AddObject(pHook, "Error", pException); // reference stealer
}

