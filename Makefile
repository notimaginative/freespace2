# Makefile for code module
# for that freespace 2 thing

CC=g++-3.0
CODE_BINARY=code.so
FS_BINARY=freespace2
LDFLAGS=$(shell sdl-config --libs)
CFLAGS=-Wall -g -DPLAT_UNIX -O2 $(shell sdl-config --cflags) -Iinclude/


%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)


CODE_SOURCES =./src/anim/animplay.cpp \
	./src/anim/packunpack.cpp \
	./src/asteroid/asteroid.cpp \
	./src/bmpman/bmpman.cpp \
	./src/cfile/cfile.cpp \
	./src/cfile/cfilearchive.cpp \
	./src/cfile/cfilelist.cpp \
	./src/cfile/cfilesystem.cpp \
	./src/cmdline/cmdline.cpp \
	./src/cmeasure/cmeasure.cpp \
	./src/controlconfig/controlsconfig.cpp \
	./src/controlconfig/controlsconfigcommon.cpp \
	./src/cutscene/cutscenes.cpp \
	./src/debris/debris.cpp \
	./src/debugconsole/console.cpp \
	./src/fireball/fireballs.cpp \
	./src/fireball/warpineffect.cpp \
	./src/gamehelp/contexthelp.cpp \
	./src/gamehelp/gameplayhelp.cpp \
	./src/gamesequence/gamesequence.cpp \
	./src/gamesnd/eventmusic.cpp \
	./src/gamesnd/gamesnd.cpp \
	./src/globalincs/alphacolors.cpp \
	./src/globalincs/crypt.cpp \
	./src/globalincs/systemvars.cpp \
	./src/globalincs/version.cpp \
	./src/graphics/2d.cpp \
	./src/graphics/aaline.cpp \
	./src/graphics/bitblt.cpp \
	./src/graphics/circle.cpp \
	./src/graphics/colors.cpp \
	./src/graphics/font.cpp \
	./src/graphics/gradient.cpp \
	./src/graphics/gropengl.cpp \
	./src/graphics/grzbuffer.cpp \
	./src/graphics/line.cpp \
	./src/graphics/pixel.cpp \
	./src/graphics/rect.cpp \
	./src/graphics/scaler.cpp \
	./src/graphics/shade.cpp \
	./src/graphics/tmapper.cpp \
	./src/graphics/tmapscanline.cpp \
	./src/graphics/tmapscantiled128x128.cpp \
	./src/graphics/tmapscantiled16x16.cpp \
	./src/graphics/tmapscantiled256x256.cpp \
	./src/graphics/tmapscantiled32x32.cpp \
	./src/graphics/tmapscantiled64x64.cpp \
	./src/hud/hud.cpp \
	./src/hud/hudartillery.cpp \
	./src/hud/hudbrackets.cpp \
	./src/hud/hudconfig.cpp \
	./src/hud/hudescort.cpp \
	./src/hud/hudets.cpp \
	./src/hud/hudlock.cpp \
	./src/hud/hudmessage.cpp \
	./src/hud/hudobserver.cpp \
	./src/hud/hudreticle.cpp \
	./src/hud/hudshield.cpp \
	./src/hud/hudsquadmsg.cpp \
	./src/hud/hudtarget.cpp \
	./src/hud/hudtargetbox.cpp \
	./src/hud/hudwingmanstatus.cpp \
	./src/io/key.cpp \
	./src/io/keycontrol.cpp \
	./src/io/mouse.cpp \
	./src/io/timer.cpp \
	./src/jumpnode/jumpnode.cpp \
	./src/lighting/lighting.cpp \
	./src/math/fix.cpp \
	./src/math/floating.cpp \
	./src/math/fvi.cpp \
	./src/math/spline.cpp \
	./src/math/staticrand.cpp \
	./src/math/vecmat.cpp \
	./src/menuui/barracks.cpp \
	./src/menuui/credits.cpp \
	./src/menuui/fishtank.cpp \
	./src/menuui/mainhallmenu.cpp \
	./src/menuui/mainhalltemp.cpp \
	./src/menuui/optionsmenu.cpp \
	./src/menuui/optionsmenumulti.cpp \
	./src/menuui/playermenu.cpp \
	./src/menuui/readyroom.cpp \
	./src/menuui/snazzyui.cpp \
	./src/menuui/techmenu.cpp \
	./src/menuui/trainingmenu.cpp \
	./src/mission/missionbriefcommon.cpp \
	./src/mission/missioncampaign.cpp \
	./src/mission/missiongoals.cpp \
	./src/mission/missiongrid.cpp \
	./src/mission/missionhotkey.cpp \
	./src/mission/missionload.cpp \
	./src/mission/missionlog.cpp \
	./src/mission/missionmessage.cpp \
	./src/mission/missionparse.cpp \
	./src/mission/missiontraining.cpp \
	./src/missionui/chatbox.cpp \
	./src/missionui/missionbrief.cpp \
	./src/missionui/missioncmdbrief.cpp \
	./src/missionui/missiondebrief.cpp \
	./src/missionui/missionloopbrief.cpp \
	./src/missionui/missionpause.cpp \
	./src/missionui/missionrecommend.cpp \
	./src/missionui/missionscreencommon.cpp \
	./src/missionui/missionshipchoice.cpp \
	./src/missionui/missionstats.cpp \
	./src/missionui/missionweaponchoice.cpp \
	./src/missionui/redalert.cpp \
	./src/model/modelcollide.cpp \
	./src/model/modelinterp.cpp \
	./src/model/modeloctant.cpp \
	./src/model/modelread.cpp \
	./src/object/collidedebrisship.cpp \
	./src/object/collidedebrisweapon.cpp \
	./src/object/collideshipship.cpp \
	./src/object/collideshipweapon.cpp \
	./src/object/collideweaponweapon.cpp \
	./src/object/objcollide.cpp \
	./src/object/object.cpp \
	./src/object/objectsnd.cpp \
	./src/object/objectsort.cpp \
	./src/observer/observer.cpp \
	./src/osapi/os_unix.cpp \
	./src/palman/palman.cpp \
	./src/parse/encrypt.cpp \
	./src/parse/parselo.cpp \
	./src/parse/sexp.cpp \
	./src/particle/particle.cpp \
	./src/pcxutils/pcxutils.cpp \
	./src/physics/physics.cpp \
	./src/playerman/managepilot.cpp \
	./src/playerman/playercontrol.cpp \
	./src/popup/popup.cpp \
	./src/popup/popupdead.cpp \
	./src/radar/radar.cpp \
	./src/render/3dclipper.cpp \
	./src/render/3ddraw.cpp \
	./src/render/3dlaser.cpp \
	./src/render/3dmath.cpp \
	./src/render/3dsetup.cpp \
	./src/ship/afterburner.cpp \
	./src/ship/ai.cpp \
	./src/ship/aibig.cpp \
	./src/ship/aicode.cpp \
	./src/ship/aigoals.cpp \
	./src/ship/awacs.cpp \
	./src/ship/shield.cpp \
	./src/ship/ship.cpp \
	./src/ship/shipcontrails.cpp \
	./src/ship/shipfx.cpp \
	./src/ship/shiphit.cpp \
	./src/starfield/nebula.cpp \
	./src/starfield/starfield.cpp \
	./src/starfield/supernova.cpp \
	./src/stats/medals.cpp \
	./src/stats/scoring.cpp \
	./src/stats/stats.cpp \
	./src/ui/button.cpp \
	./src/ui/checkbox.cpp \
	./src/ui/gadget.cpp \
	./src/ui/icon.cpp \
	./src/ui/inputbox.cpp \
	./src/ui/keytrap.cpp \
	./src/ui/listbox.cpp \
	./src/ui/radio.cpp \
	./src/ui/scroll.cpp \
	./src/ui/slider.cpp \
	./src/ui/slider2.cpp \
	./src/ui/uidraw.cpp \
	./src/ui/uimouse.cpp \
	./src/ui/window.cpp \
	./src/weapon/beam.cpp \
	./src/weapon/corkscrew.cpp \
	./src/weapon/emp.cpp \
	./src/weapon/flak.cpp \
	./src/weapon/muzzleflash.cpp \
	./src/weapon/shockwave.cpp \
	./src/weapon/swarm.cpp \
	./src/weapon/trails.cpp \
	./src/weapon/weapons.cpp \
	./src/nebula/neb.cpp \
	./src/nebula/neblightning.cpp \
	./src/localization/fhash.cpp \
	./src/localization/localize.cpp \
	./src/tgautils/tgautils.cpp \
	./src/demo/demo.cpp \
	./src/inetfile/cftp.cpp \
	./src/inetfile/chttpget.cpp \
	./src/inetfile/inetgetfile.cpp \
	./src/exceptionhandler/exceptionhandler.cpp \
	./src/network/multi.cpp \
	./src/network/multi_campaign.cpp \
	./src/network/multi_data.cpp \
	./src/network/multi_dogfight.cpp \
	./src/network/multi_endgame.cpp \
	./src/network/multi_ingame.cpp \
	./src/network/multi_kick.cpp \
	./src/network/multi_log.cpp \
	./src/network/multi_obj.cpp \
	./src/network/multi_observer.cpp \
	./src/network/multi_oo.cpp \
	./src/network/multi_options.cpp \
	./src/network/multi_pause.cpp \
	./src/network/multi_pinfo.cpp \
	./src/network/multi_ping.cpp \
	./src/network/multi_pmsg.cpp \
	./src/network/multi_rate.cpp \
	./src/network/multi_respawn.cpp \
	./src/network/multi_team.cpp \
	./src/network/multi_update.cpp \
	./src/network/multi_voice.cpp \
	./src/network/multi_xfer.cpp \
	./src/network/multilag.cpp \
	./src/network/multimsgs.cpp \
	./src/network/multiteamselect.cpp \
	./src/network/multiui.cpp \
	./src/network/multiutil.cpp \
	./src/network/psnet.cpp \
	./src/network/psnet2.cpp \
	./src/platform/unix.cpp
#	./src/network/stand_gui.cpp 

FS_SOURCES=./src/freespace2/freespace.cpp \
	./src/freespace2/levelpaging.cpp


CODE_OBJECTS=$(CODE_SOURCES:.cpp=.o)
FS_OBJECTS=$(FS_SOURCES:.cpp=.o)


all: code fs2

code: $(CODE_OBJECTS)
	$(CC) -shared -o $(CODE_BINARY) $(LDFLAGS) $(CODE_OBJECTS)

fs2: $(FS_OBJECTS)
	$(CC) -o $(FS_BINARY) $(LDFLAGS) $(FS_OJBECTS) $(CODE_BINARY)

clean:
	rm -rf $(BINARY) $(OBJECTS)
