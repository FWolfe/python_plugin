import FLHook # embedded module in FLHook's C++


# C++ structs are converted to collections.namedtuple objects. This keeps our C++ syntax for accessing attributes like:
# Vector.x, Vector.y, etc, and ensures a 'read-only' mentality on the struct data
from freelancer.embedded.classes import convertConst, convertClass

#==============================================================================
# FLHook Constants
#==============================================================================
HKE_OK = 0
HKE_PLAYER_NOT_LOGGED_IN = 1
HKE_CHAR_DOES_NOT_EXIST = 2
HKE_COULD_NOT_DECODE_CHARFILE = 3
HKE_COULD_NOT_ENCODE_CHARFILE = 4
HKE_INVALID_BASENAME = 5
HKE_UNKNOWN_ERROR = 6
HKE_INVALID_CLIENT_ID = 7
HKE_INVALID_GROUP_ID = 8
HKE_INVALID_ID_STRING = 9
HKE_INVALID_SYSTEM = 10
HKE_PLAYER_NOT_IN_SPACE = 11
HKE_PLAYER_NOT_DOCKED = 12
HKE_PLAYER_NO_ADMIN = 13
HKE_WRONG_XML_SYNTAX = 14
HKE_INVALID_GOOD = 15
HKE_NO_CHAR_SELECTED = 16
HKE_CHARNAME_ALREADY_EXISTS = 17
HKE_CHARNAME_TOO_LONG = 18
HKE_CHARNAME_TOO_SHORT = 19
HKE_AMBIGUOUS_SHORTCUT = 20
HKE_NO_MATCHING_PLAYER = 21
HKE_INVALID_SHORTCUT_STRING = 22
HKE_MPNEWCHARACTERFILE_NOT_FOUND_OR_INVALID = 23
HKE_INVALID_REP_GROUP = 24
HKE_COULD_NOT_GET_PATH = 25
HKE_NO_TASKKILL = 26
HKE_SYSTEM_NO_MATCH = 27
HKE_OBJECT_NO_DOCK = 28
HKE_CARGO_WONT_FIT = 29
HKE_NAME_NOT_MATCH_RESTRICTION = 30


#==============================================================================
# PLUGIN_RETURNCODE
DEFAULT_RETURNCODE = 0
SKIPPLUGINS = 1
SKIPPLUGINS_NOFUNCTIONCALL = 2
NOFUNCTIONCALL = 3


#==============================================================================
# ENGINE_STATE
ES_CRUISE = 0
ES_THRUSTER = 1
ES_ENGINE = 2
ES_KILLED = 3
ES_TRADELANE = 4

#==============================================================================
# EQ_TYPE
ET_GUN = 0
ET_TORPEDO = 1
ET_CD = 2
ET_MISSILE = 3
ET_MINE = 4
ET_CM = 5
ET_SHIELDGEN = 6
ET_THRUSTER = 7
ET_SHIELDBAT = 8
ET_NANOBOT = 9
ET_MUNITION = 10
ET_ENGINE = 11
ET_OTHER = 12
ET_SCANNER = 13
ET_TRACTOR = 14
ET_LIGHT = 15

#==============================================================================
# Some Callbacks we shouldnt log (called too often)

logtypes = {
    'HkCbIServerImpl_Update' : False,
    'HkCbIServerImpl_SPObjUpdate' : False,
    'HkCbIServerImpl_SPObjUpdate_AFTER' : False,
    'HkCbIServerImpl_ActivateThrusters' : False,
    'HkCbIServerImpl_ActivateThrusters_AFTER' : False,
    'HkCbIServerImpl_ActivateEquip' : False,
    'HkCbIServerImpl_ActivateEquip_AFTER' : False,
    'HkCbIServerImpl_FireWeapon' : False,
    'HkCbIServerImpl_FireWeapon_AFTER' : False,
    'HkCbIServerImpl_SPMunitionCollision' : False,
    'HkCbIServerImpl_SPMunitionCollision_AFTER' : False,
    'ShipDestroyed' : False, # causes buffer overflow when logged?
    'HkTimerCheckKick' : False,
    'HkCb_Elapse_Time' : False, 
    'HkCb_Elapse_Time_AFTER' : False, 
    'HkCb_Update_Time' : False, 
    'HkCb_Update_Time_AFTER' : False, 
    'HkCb_AddDmgEntry' : False,
    'HkCb_AddDmgEntry_AFTER' : False,
    
    'HkCbIServerImpl_SetWeaponGroup' : False,
    'HkCbIServerImpl_SetWeaponGroup_AFTER' : False,
}

def logger(text):
    """logger(text)
    basic logging function"""
    FLHook.ConPrint("%s\n" % text)
    #FLHook.AddLog(text)

def _init():
    """_init()
    internal function called from C++, when python is starting up.
    """
    logger('Python Loaded OK!')


def _shutdown():
    """_shutdown()
    internal function called from C++, when python is shutting down.
    """
    pass


def _callback(event, data):
    """_callback(event, data)
    internal function called from C++, pyCallback() function. This is our main callback handler
    """
    # your code goes here, this line is simply a debugger
    if logtypes.get(event, True):
        logger('Python Got: %s : %s' % (event, str(data)))

    return DEFAULT_RETURNCODE

