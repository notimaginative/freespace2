#ifndef CC_FSDEMO_H
#define CC_FSDEMO_H

// --------------------------------------------------
// Keyboard #defines for the actions
// This is the value of the id field in config_item
// --------------------------------------------------

// targeting a ship

#define TARGET_NEXT									0
#define TARGET_PREV									1
#define TARGET_NEXT_CLOSEST_HOSTILE					2
#define TARGET_PREV_CLOSEST_HOSTILE					3
#define TOGGLE_AUTO_TARGETING						4
#define TARGET_NEXT_CLOSEST_FRIENDLY				5
#define TARGET_PREV_CLOSEST_FRIENDLY				6
#define TARGET_SHIP_IN_RETICLE						7
#define TARGET_CLOSEST_SHIP_ATTACKING_TARGET		8
#define STOP_TARGETING_SHIP							9

// targeting a ship's subsystem
#define TARGET_SUBOBJECT_IN_RETICLE					10
#define TARGET_NEXT_SUBOBJECT						11
#define TARGET_PREV_SUBOBJECT						12
#define STOP_TARGETING_SUBSYSTEM					13

// speed matching
#define MATCH_TARGET_SPEED							14
#define TOGGLE_AUTO_MATCH_TARGET_SPEED				15

// weapons
#define FIRE_PRIMARY								16
#define FIRE_SECONDARY								17
#define CYCLE_NEXT_PRIMARY							18
#define CYCLE_PREV_PRIMARY							19
#define CYCLE_SECONDARY								20
#define CYCLE_NUM_MISSLES							21
#define LAUNCH_COUNTERMEASURE						22

// controls
#define FORWARD_THRUST								23
#define REVERSE_THRUST								24
#define BANK_LEFT									25
#define BANK_RIGHT									26
#define PITCH_FORWARD								27
#define PITCH_BACK									28
#define YAW_LEFT									29
#define YAW_RIGHT									30

// throttle control
#define ZERO_THROTTLE								31
#define MAX_THROTTLE								32
#define ONE_THIRD_THROTTLE							33
#define TWO_THIRDS_THROTTLE							34
#define PLUS_5_PERCENT_THROTTLE						35
#define MINUS_5_PERCENT_THROTTLE					36

// squadmate messaging keys
#define ATTACK_MESSAGE								37
#define DISARM_MESSAGE								38
#define DISABLE_MESSAGE								39
#define ATTACK_SUBSYSTEM_MESSAGE					40
#define CAPTURE_MESSAGE								41
#define ENGAGE_MESSAGE								42
#define FORM_MESSAGE								43
#define IGNORE_MESSAGE								44
#define PROTECT_MESSAGE								45
#define COVER_MESSAGE								46
#define WARP_MESSAGE								47
#define REARM_MESSAGE								48

#define TARGET_CLOSEST_SHIP_ATTACKING_SELF			49

// Views
#define VIEW_CHASE									50
#define VIEW_EXTERNAL								51
#define VIEW_EXTERNAL_TOGGLE_CAMERA_LOCK			52
#define VIEW_SLEW									53
#define VIEW_OTHER_SHIP								54
#define VIEW_DIST_INCREASE							55
#define VIEW_DIST_DECREASE							56
#define VIEW_CENTER									57

#define RADAR_RANGE_CYCLE							58
#define SQUADMSG_MENU								59
#define SHOW_GOALS									60
#define END_MISSION									61
#define TARGET_TARGETS_TARGET						62
#define AFTERBURNER									63

#define INCREASE_WEAPON								64
#define DECREASE_WEAPON								65
#define INCREASE_SHIELD								66
#define DECREASE_SHIELD								67
#define INCREASE_ENGINE								68
#define DECREASE_ENGINE								69
#define SHIELD_EQUALIZE								70
#define SHIELD_XFER_TOP								71
#define SHIELD_XFER_BOTTOM							72
#define SHIELD_XFER_LEFT							73
#define SHIELD_XFER_RIGHT							74

#define XFER_SHIELD									75
#define XFER_LASER									76
#define SHOW_DAMAGE_POPUP							77

#define BANK_WHEN_PRESSED							78
#define SHOW_NAVMAP									79
#define ADD_REMOVE_ESCORT							80
#define ESCORT_CLEAR								81
#define TARGET_NEXT_ESCORT_SHIP						82

#define TARGET_CLOSEST_REPAIR_SHIP					83
#define TARGET_NEXT_UNINSPECTED_CARGO				84
#define TARGET_PREV_UNINSPECTED_CARGO				85
#define TARGET_NEWEST_SHIP							86
#define TARGET_NEXT_LIVE_TURRET						87
#define TARGET_PREV_LIVE_TURRET						88

#define TARGET_NEXT_BOMB							89
#define TARGET_PREV_BOMB							90

// multiplayer messaging keys
#define MULTI_MESSAGE_ALL							91
#define MULTI_MESSAGE_FRIENDLY						92
#define MULTI_MESSAGE_HOSTILE						93
#define MULTI_MESSAGE_TARGET						94

// multiplayer misc keys
#define MULTI_OBSERVER_ZOOM_TO						95

#define TIME_SPEED_UP								96
#define TIME_SLOW_DOWN								97

#define PADLOCK_UP									98
#define PADLOCK_DOWN								99
#define PADLOCK_LEFT								100
#define PADLOCK_RIGHT								101

// this should be the total number of control action defines above (or last define + 1)
#define CCFG_MAX 102

#endif // CC_FSDEMO_H
