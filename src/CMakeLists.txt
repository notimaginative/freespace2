
# ##############################################################################
#

#
# main code/game library
#

set(code_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/anim/animplay.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/anim/packunpack.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/asteroid/asteroid.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bmpman/bmpman.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cfile/cfile.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cfile/cfilearchive.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cfile/cfilelist.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cfile/cfilesystem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cmdline/cmdline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cmeasure/cmeasure.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/controlconfig/controlsconfig.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/controlconfig/controlsconfigcommon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cutscene/cutscenes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/debris/debris.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/debugconsole/console.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/demo/demo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fireball/fireballs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fireball/warpineffect.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gamehelp/contexthelp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gamehelp/gameplayhelp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gamesequence/gamesequence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gamesnd/eventmusic.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gamesnd/gamesnd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/globalincs/alphacolors.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/globalincs/crypt.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/globalincs/systemvars.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/globalincs/version.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/2d.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/font.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/grgl1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/grgl1render.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/grgl1texture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/gropengl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/grstub.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/grwxgl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hud.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudartillery.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudbrackets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudconfig.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudescort.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudlock.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudmessage.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudobserver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudreticle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudshield.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudsquadmsg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudtarget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudtargetbox.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hud/hudwingmanstatus.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/inetfile/cftp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/inetfile/chttpget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/inetfile/inetgetfile.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/io/joy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/io/joy_ff.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/io/key.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/io/keycontrol.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/io/mouse.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/io/timer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/jumpnode/jumpnode.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lighting/lighting.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/localization/fhash.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/localization/localize.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/localization/strings_tbl_fs1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/math/fix.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/math/floating.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/math/fvi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/math/spline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/math/staticrand.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/math/vecmat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/menuui/barracks.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/menuui/credits.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/menuui/fishtank.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/menuui/mainhallmenu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/menuui/mainhalltemp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/menuui/optionsmenu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/menuui/optionsmenumulti.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/menuui/playermenu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/menuui/readyroom.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/menuui/snazzyui.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/menuui/techmenu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/menuui/trainingmenu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mission/missionbriefcommon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mission/missioncampaign.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mission/missiongoals.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mission/missiongrid.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mission/missionhotkey.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mission/missionload.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mission/missionlog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mission/missionmessage.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mission/missionparse.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mission/missiontraining.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missionui/chatbox.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missionui/missionbrief.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missionui/missioncmdbrief.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missionui/missiondebrief.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missionui/missionloopbrief.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missionui/missionpause.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missionui/missionrecommend.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missionui/missionscreencommon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missionui/missionshipchoice.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missionui/missionstats.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missionui/missionweaponchoice.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/missionui/redalert.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/model/modelcollide.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/model/modelinterp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/model/modeloctant.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/model/modelread.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/movie/decoder16.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/movie/movie.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/movie/mve_audio.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/movie/mvelib.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/movie/mveplayer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/nebula/neb.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/nebula/neblightning.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/chat_api.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/gtrack.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_campaign.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_data.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_dogfight.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_endgame.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_fstracker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_ingame.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_kick.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_obj.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_observer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_oo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_options.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_pause.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_pinfo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_ping.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_pmsg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_pxo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_rate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_respawn.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_sw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_team.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_update.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_voice.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multi_xfer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multilag.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multimsgs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multiteamselect.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multiui.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/multiutil.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/psnet.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/psnet2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/ptrack.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/stand_server.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/network/valid.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/object/collidedebrisship.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/object/collidedebrisweapon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/object/collideshipship.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/object/collideshipweapon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/object/collideweaponweapon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/object/objcollide.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/object/object.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/object/objectsnd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/object/objectsort.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/observer/observer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/osapi/osapi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/osapi/osregistry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/osapi/outwnd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/palman/palman.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/parse/encrypt.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/parse/parselo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/parse/sexp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/particle/particle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pcxutils/pcxutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/physics/physics.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/playerman/managepilot.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/playerman/playercontrol.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/popup/popup.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/popup/popupdead.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/radar/radar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/render/3dclipper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/render/3ddraw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/render/3dlaser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/render/3dmath.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/render/3dsetup.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ship/afterburner.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ship/ai.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ship/aibig.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ship/aicode.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ship/aigoals.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ship/awacs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ship/shield.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ship/ship.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ship/shipcontrails.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ship/shipfx.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ship/shiphit.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sound/acm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sound/audiostr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sound/oal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sound/oal_capture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sound/oal_efx.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sound/rtvoice.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sound/sound.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/starfield/nebula.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/starfield/starfield.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/starfield/supernova.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stats/medals.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stats/scoring.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stats/stats.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tgautils/tgautils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/button.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/checkbox.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/gadget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/icon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/inputbox.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/keytrap.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/listbox.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/radio.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/scroll.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/slider.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/slider2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/uidraw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/uimouse.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/window.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vcodec/codec1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/weapon/beam.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/weapon/corkscrew.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/weapon/emp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/weapon/flak.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/weapon/muzzleflash.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/weapon/shockwave.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/weapon/swarm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/weapon/trails.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/weapon/weapons.cpp
  PARENT_SCOPE
)


set(platform_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/unix.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/win.cpp
  PARENT_SCOPE
)


#
# the game itself
#

set(freespace_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/freespace2/freespace.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/freespace2/levelpaging.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/freespace2/main.cpp
  PARENT_SCOPE
)

#
# launcher
#

set(launcher_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/launcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/launcherini.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/launchersetup.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/freespace_img.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_background.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_help-click.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_help-hover.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_help.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_play-click.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_play-hover.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_play.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_pxo-click.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_pxo-hover.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_pxo.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_quit-click.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_quit-hover.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_quit.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_readme-click.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_readme-hover.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_readme.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_setup-click.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_setup-hover.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_setup.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_uninstall-click.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_uninstall-hover.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_uninstall.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_update-click.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_update-hover.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_update.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_volition-click.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_volition-hover.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_btn_volition.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_help_txt.h
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_snd_click_wav.h
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/fs2_snd_hover_wav.h
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher/res/volition_img.xpm
  PARENT_SCOPE
)

#
# ##############################################################################

# ##############################################################################
#
# toolset: targets for creating/modifying game assets
#

#
# AC: anim converter
#

set(ac_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/ac/ac.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ac/ac_stubs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ac/convert.cpp
  PARENT_SCOPE
)

#
# CFILEARCHIVER: to create VP file archives
#

set(cfilearchiver_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/cfilearchiver/cfilearchiver.cpp
  PARENT_SCOPE
)

#
# CRYPTSTRING: string encryption (for embedded cheat codes)
#

set(cryptstring_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/cryptstring/cryptstring.cpp
  PARENT_SCOPE
)

#
# NEBEDIT: FS1 style nebula editor/creator
#

set(nebedit_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/nebedit/nebedit.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/nebedit/nebstubs.cpp
  PARENT_SCOPE
)

#
# POFVIEW: model viewer
#

set(pofview_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/pofview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/pofviewdialogs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/pofviewcanvas.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/pofviewstubs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/res/pofview_ico.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/res/tool_about.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/res/tool_damaged.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/res/tool_debris.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/res/tool_detail1.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/res/tool_detail2.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/res/tool_detail3.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/res/tool_detail4.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/res/tool_detail5.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/res/tool_detail6.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/res/tool_lights.xpm
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview/res/tool_tree.xpm
  PARENT_SCOPE
)

#
# SCRAMBLE: file-based encryption for TBLs
#

set(scramble_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/parse/encrypt.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/scramble/scramble.cpp
  PARENT_SCOPE
)

#
# FONTTOOL: create font files / edit kerning data
#

set(fonttool_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/fonttool/fontstubs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fonttool/fontcreate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fonttool/fontkern.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fonttool/fontkerncopy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fonttool/fonttool.cpp
  PARENT_SCOPE
)
