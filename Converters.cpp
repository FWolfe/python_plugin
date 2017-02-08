#include "headers.h"
//#include <Python.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
pyConstConverter - takes a struct's name, and a python tuple of that structs data and converts it to a
collections.namedtuple python object.
*/
PyObject* pyConstConverter(string scStructName, PyObject *pData)
{
	PyObject *pResult;
	try {
		PyObject *pName = PyString_FromString(scStructName.c_str());
		PyObject *pArgs = Py_BuildValue("(NN)", pName, pData);

		pResult = PyObject_CallObject(pConstConverter, pArgs);
		Py_XDECREF(pArgs);
		if (pResult == NULL) { // function call error?
			ERRMSG(L"ERROR convertConst returned NULL: " + stows(scStructName));
		}
	}
	catch (...) { AddLog("Exception in pyConverter"); }
	return pResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
pyClassConverter
*/
PyObject* pyClassConverter(string scClassName, void *ptr)
{
	PyObject *pResult;
	try {
		PyObject *pName = PyString_FromString(scClassName.c_str());
		PyObject *pCapsule = PyCapsule_New(ptr, NULL, NULL);
		PyObject *pArgs = Py_BuildValue("(NN)", pName, pCapsule);
		pResult = PyObject_CallObject(pClassConverter, pArgs);
		Py_XDECREF(pArgs);
		if (pResult == NULL) { // function call error?
			ERRMSG(L"ERROR convertClass returned NULL: " + stows(scClassName));
		}
	}
	catch (...) { AddLog("Exception in pyClassConverter"); }
	return pResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
pytows - python string to wide string. this works on any python object (not just strings) by getting a string
	repersentation of it. Basically calling python's str() function
*/
wstring pytows(PyObject *pObj)
{
	PyObject* pString = PyObject_Str(pObj);
	const char* cString = PyString_AsString(pString);
	Py_XDECREF(pString);
	string scString = string(cString);
	return stows(scString);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
pytos - python string to string. this works on any python object (not just strings) by getting a string
repersentation of it. Basically calling python's str() function
*/
string pytos(PyObject *pObj)
{
	PyObject* pString = PyObject_Str(pObj);
	const char* cString = PyString_AsString(pString);
	Py_XDECREF(pString);
	return string(cString);
}


PyObject* ToPython(wstring wscString)
{
	// when passing our return to Py_BuildValue, use N (not O). Here our reference count is 1
	// by using N we dont have to call Py_DECREF() since N steals the reference. If we're keeping this object
	// for other reasons we'll have to manually decrease the count.
	PyObject *pString = PyString_FromString(wstos(wscString).c_str());
	return pString;
}

PyObject* ToPython(Vector hkInfo)
{
	return pyConstConverter("Vector", 
		Py_BuildValue("(fff)", 
			hkInfo.x, 
			hkInfo.y, 
			hkInfo.z));
}
PyObject* ToPython(Vector* hkInfo)
{
	return pyClassConverter("Vector", hkInfo);
};

PyObject* ToPython(Quaternion hkInfo)
{
	return pyConstConverter("Quaternion", 
		Py_BuildValue("(ffff)", 
			hkInfo.w, 
			hkInfo.x, 
			hkInfo.y, 
			hkInfo.z));
}
PyObject* ToPython(Quaternion* hkInfo)
{
	return pyClassConverter("Quaternion", hkInfo);
};


PyObject* ToPython(SSPObjUpdateInfo hkInfo)
{
	return pyConstConverter("SSPObjUpdateInfo", 
		Py_BuildValue("(INNfffb)", 
			hkInfo.iShip,
			ToPython(hkInfo.vDir), 
			ToPython(hkInfo.vPos), 
			hkInfo.fTimestamp, 
			hkInfo.fDunno, 
			hkInfo.throttle, 
			hkInfo.cState
		));
}

PyObject* ToPython(SSPObjCollisionInfo hkInfo)
{
	return pyConstConverter("SSPObjCollisionInfo", 
		Py_BuildValue("(IIIIf)", 
			hkInfo.iColliderObjectID,
			hkInfo.iColliderSubObjID, 
			hkInfo.iDamagedObjectID, 
			hkInfo.iDamagedSubObjID, 
			hkInfo.fDamage
		));
}


PyObject* ToPython(SStartupInfo hkInfo)
{
	return pyConstConverter("SStartupInfo", 
		Py_BuildValue("(OI)",
			Py_None, //uint iDunno[130],
			hkInfo.iMaxPlayers
		));
}


PyObject* ToPython(SLoginInfo hkInfo)
{
	return pyConstConverter("SLoginInfo", 
		Py_BuildValue("(s)", 
			wstos(hkInfo.wszAccount)
		));
}


PyObject* ToPython(SCreateCharacterInfo hkInfo)
{
	// From [Faction] section of newcharacter.ini
	return pyConstConverter("SCreateCharacterInfo", 
		Py_BuildValue("(sIII)",
			wstos(hkInfo.wszCharname), 
			hkInfo.iNickName, 
			hkInfo.iBase, 
			hkInfo.iPackage, 
			hkInfo.iPilot,
			Py_None // uint iDunno[96];
		));
}


PyObject* ToPython(XFireWeaponInfo hkInfo)
{ // these arrays need conversion
	return pyConstConverter("XFireWeaponInfo", 
		//Py_BuildValue("(INIHHH)",
		Py_BuildValue("(INIHHH)",
			hkInfo.iDunno1,
			ToPython(hkInfo.vDirection), 
			hkInfo.iDunno2, 
			hkInfo.sArray1, 
			hkInfo.sArray2, 
			hkInfo.s3)
	);
}


PyObject* ToPython(XActivateEquip hkInfo)
{
	return pyConstConverter("XActivateEquip", 
		Py_BuildValue("(IHO)", 
			hkInfo.iSpaceID, 
			hkInfo.sID, 
			PY_BOOL(hkInfo.bActivate)
		));
}


PyObject* ToPython(XActivateCruise hkInfo)
{
	return pyConstConverter("XActivateCruise", 
		Py_BuildValue("(IO)", 
			hkInfo.iShip, 
			PY_BOOL(hkInfo.bActivate)
		));
}


PyObject* ToPython(XActivateThrusters hkInfo)
{
	return pyConstConverter("XActivateThrusters", 
		Py_BuildValue("(IO)", 
			hkInfo.iShip, 
			PY_BOOL(hkInfo.bActivate)
		));
}


PyObject* ToPython(XSetTarget hkInfo)
{
	return pyConstConverter("XSetTarget", 
		Py_BuildValue("(IIII)", 
			hkInfo.iShip, 
			hkInfo.iSlot, 
			hkInfo.iSpaceID, 
			hkInfo.iSubObjID
		));
}


PyObject* ToPython(XGoTradelane hkInfo)
{
	return pyConstConverter("XGoTradelane", 
		Py_BuildValue("(III)", 
			hkInfo.iShip, 
			hkInfo.iTradelaneSpaceObj1, 
			hkInfo.iTradelaneSpaceObj2
		));
}


PyObject* ToPython(XJettisonCargo hkInfo)
{
	return pyConstConverter("XJettisonCargo", 
		Py_BuildValue("(III)", 
			hkInfo.iShip, 
			hkInfo.iSlot, 
			hkInfo.iCount
		));
}


PyObject* ToPython(XSetManeuver hkInfo)
{
	return pyConstConverter("XSetManeuver", 
		Py_BuildValue("(III)", 
			hkInfo.iShipFrom, 
			hkInfo.IShipTo, 
			hkInfo.iFlag
		));
}


PyObject* ToPython(SGFGoodSellInfo hkInfo)
{
	return pyConstConverter("SGFGoodSellInfo", 
		Py_BuildValue("(lIi)", 
			hkInfo.l1, 
			hkInfo.iArchID, 
			hkInfo.iCount
		));
}


PyObject* ToPython(SGFGoodBuyInfo hkInfo)
{
	return pyConstConverter("SGFGoodBuyInfo", 
		Py_BuildValue("(IkIi)", 
			hkInfo.iBaseID, 
			hkInfo.lNull, 
			hkInfo.iGoodID, 
			hkInfo.iCount
		));
}


PyObject* ToPython(SSPMunitionCollisionInfo hkInfo)
{
	return pyConstConverter("SSPMunitionCollisionInfo", 
		Py_BuildValue("(IkkH)", 
			hkInfo.iProjectileArchID, 
			hkInfo.dw2, 
			hkInfo.dwTargetShip, 
			hkInfo.s1
		));
}

// need to convert this one to a class object
PyObject* ToPython(EquipDesc hkInfo)
{
	return pyConstConverter("EquipDesc", 
		Py_BuildValue("(HHIOfIOI)", 
			hkInfo.iDunno, 
			hkInfo.sID,
			hkInfo.iArchID, 
			PY_BOOL(hkInfo.bMounted), 
			hkInfo.fHealth, 
			hkInfo.iCount, 
			PY_BOOL(hkInfo.bMission), 
			hkInfo.iOwner
			//CacheString hkInfo.szHardPoint not mapped
		));
}


PyObject* ToPython(CHARACTER_ID hkInfo)
{
	return pyConstConverter("CHARACTER_ID", 
		Py_BuildValue("(s)", 
			hkInfo.szCharFilename
		));
}


PyObject* ToPython(CARGO_INFO hkInfo)
{
	return pyConstConverter("CARGO_INFO", 
		Py_BuildValue("(IiIfOO)", 
			hkInfo.iID, 
			hkInfo.iCount, 
			hkInfo.iArchID, 
			hkInfo.fStatus, 
			PY_BOOL(hkInfo.bMission), 
			PY_BOOL(hkInfo.bMounted)
			//CacheString hkInfo.HardPoint not mapped
		));
}


PyObject* ToPython(MONEY_FIX hkInfo)
{
	return pyConstConverter("MONEY_FIX", 
		Py_BuildValue("(si)", 
			wstos(hkInfo.wscCharname), 
			hkInfo.iAmount
		));
}


PyObject* ToPython(IGNORE_INFO hkInfo)
{
	return pyConstConverter("IGNORE_INFO", 
		Py_BuildValue("(ss)", 
			wstos(hkInfo.wscCharname), 
			wstos(hkInfo.wscFlags)
		));
}


PyObject* ToPython(RESOLVE_IP hkInfo)
{
	return pyConstConverter("RESOLVE_IP", 
		Py_BuildValue("(IIss)", 
			hkInfo.iClientID, 
			hkInfo.iConnects, 
			wstos(hkInfo.wscIP), 
			wstos(hkInfo.wscHostname)
		));
}


PyObject* ToPython(HKPLAYERINFO hkInfo)
{
	return pyConstConverter("HKPLAYERINFO", 
		Py_BuildValue("(IsssIIss)",
			hkInfo.iClientID, 
			wstos(hkInfo.wscCharname), 
			wstos(hkInfo.wscBase), 
			wstos(hkInfo.wscSystem),
			hkInfo.iSystem,
			hkInfo.iShip,
			wstos(hkInfo.wscIP),
			wstos(hkInfo.wscHostname)
			// DPN_CONNECTION_INFO ci;
		));
};


PyObject* ToPython(DamageEntry hkInfo)
{
	return pyConstConverter("DamageEntry", 
		Py_BuildValue("(HfI)", 
			hkInfo.subobj, 
			hkInfo.health, 
			hkInfo.fate
		));
};


PyObject* ToPython(list<CARGO_INFO> &lstCargo)
{
	uint size = lstCargo.size();
	uint i = 0;
	PyObject *pList;
	pList = PyTuple_New(size);
	foreach(lstCargo, CARGO_INFO, it)
	{
		PyTuple_SetItem(pList, i, ToPython(*it));
		++i;
	}
	return pList;
}


PyObject* ToPython(list<uint> &lst)
{
	uint size = lst.size();
	uint i = 0;
	PyObject *pList;
	pList = PyTuple_New(size);
	foreach(lst, uint, it)
	{
		PyTuple_SetItem(pList, i, Py_BuildValue("I", *it));
		++i;
	}
	return pList;
}


PyObject* ToPython(list<DamageEntry> &lstDmg)
{
	uint size = lstDmg.size();
	uint i = 0;
	PyObject *pList;
	pList = PyTuple_New(size);
	foreach(lstDmg, DamageEntry, it)
	{
		PyTuple_SetItem(pList, i, ToPython(*it));
		++i;
	}
	return pList;
}

/////////////////////////////////////////////////////////////////////////////////////

PyObject* ToPython(DamageList* hkInfo)
{
	return pyClassConverter("DamageList", hkInfo);
};


PyObject* ToPython(CLIENT_INFO* hkInfo)
{
	return pyClassConverter("ClientInfo", hkInfo);
};


struct SSPUseItem
{
	uint object;    // ship id, I think
	short hpid;     // hardpoint identifier
	short quantity; // seems to be number you have, not to use
};


struct SGFGoodVaporizedInfo
{
	uint object;
	uint nickname;
	uint quantity;
};


PyObject* ToPython(SSPUseItem hkInfo)
{
	return pyConstConverter("SSPUseItem", 
		Py_BuildValue("(Ihh)", 
			hkInfo.object, 
			hkInfo.hpid, 
			hkInfo.quantity
		));
};


