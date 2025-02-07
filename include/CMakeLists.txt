
# ##############################################################################
#

#
# main code/game library
#

set(code_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/2d.h
  ${CMAKE_CURRENT_SOURCE_DIR}/3d.h
  ${CMAKE_CURRENT_SOURCE_DIR}/3dinternal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/acm.h
  ${CMAKE_CURRENT_SOURCE_DIR}/afterburner.h
  ${CMAKE_CURRENT_SOURCE_DIR}/aibig.h
  ${CMAKE_CURRENT_SOURCE_DIR}/aigoals.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ai.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ailocal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/alphacolors.h
  ${CMAKE_CURRENT_SOURCE_DIR}/animplay.h
  ${CMAKE_CURRENT_SOURCE_DIR}/app_icon.h
  ${CMAKE_CURRENT_SOURCE_DIR}/asteroid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/audiostr.h
  ${CMAKE_CURRENT_SOURCE_DIR}/awacs.h
  ${CMAKE_CURRENT_SOURCE_DIR}/barracks.h
  ${CMAKE_CURRENT_SOURCE_DIR}/beam.h
  ${CMAKE_CURRENT_SOURCE_DIR}/bmpman.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cc_fsdemo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cfilearchive.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cfile.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cfilesystem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cftp.h
  ${CMAKE_CURRENT_SOURCE_DIR}/chat_api.h
  ${CMAKE_CURRENT_SOURCE_DIR}/chatbox.h
  ${CMAKE_CURRENT_SOURCE_DIR}/chttpget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cmdline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cmeasure.h
  ${CMAKE_CURRENT_SOURCE_DIR}/codec1.h
  ${CMAKE_CURRENT_SOURCE_DIR}/contexthelp.h
  ${CMAKE_CURRENT_SOURCE_DIR}/controlsconfig.h
  ${CMAKE_CURRENT_SOURCE_DIR}/corkscrew.h
  ${CMAKE_CURRENT_SOURCE_DIR}/credits.h
  ${CMAKE_CURRENT_SOURCE_DIR}/crypt.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cutscenes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/debris.h
  ${CMAKE_CURRENT_SOURCE_DIR}/demo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/dumpstats.h
  ${CMAKE_CURRENT_SOURCE_DIR}/editor.h
  ${CMAKE_CURRENT_SOURCE_DIR}/emp.h
  ${CMAKE_CURRENT_SOURCE_DIR}/encrypt.h
  ${CMAKE_CURRENT_SOURCE_DIR}/eventmusic.h
  ${CMAKE_CURRENT_SOURCE_DIR}/exceptionhandler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/fhash.h
  ${CMAKE_CURRENT_SOURCE_DIR}/fireballs.h
  ${CMAKE_CURRENT_SOURCE_DIR}/fishtank.h
  ${CMAKE_CURRENT_SOURCE_DIR}/fix.h
  ${CMAKE_CURRENT_SOURCE_DIR}/flak.h
  ${CMAKE_CURRENT_SOURCE_DIR}/floating.h
  ${CMAKE_CURRENT_SOURCE_DIR}/font.h
  ${CMAKE_CURRENT_SOURCE_DIR}/fvi.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gamepad.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gameplayhelp.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gamesequence.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gamesnd.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gitinfo.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/grinternal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/grgl1.h
  ${CMAKE_CURRENT_SOURCE_DIR}/grgl2.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gropengl.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gropenglinternal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/grstub.h
  ${CMAKE_CURRENT_SOURCE_DIR}/grwxgl.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gtrack.h
  ${CMAKE_CURRENT_SOURCE_DIR}/helpeddoc.h
  ${CMAKE_CURRENT_SOURCE_DIR}/helped.h
  ${CMAKE_CURRENT_SOURCE_DIR}/helpedline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/helpedview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudartillery.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudbrackets.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudconfig.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudescort.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudets.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudgauges.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hud.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudlock.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudmessage.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudobserver.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudresource.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudreticle.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudshield.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudsquadmsg.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudtargetbox.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudtarget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hudwingmanstatus.h
  ${CMAKE_CURRENT_SOURCE_DIR}/inetgetfile.h
  ${CMAKE_CURRENT_SOURCE_DIR}/initialships.h
  ${CMAKE_CURRENT_SOURCE_DIR}/initialstatus.h
  ${CMAKE_CURRENT_SOURCE_DIR}/joy_ff.h
  ${CMAKE_CURRENT_SOURCE_DIR}/joy.h
  ${CMAKE_CURRENT_SOURCE_DIR}/jumpnode.h
  ${CMAKE_CURRENT_SOURCE_DIR}/keycontrol.h
  ${CMAKE_CURRENT_SOURCE_DIR}/key.h
  ${CMAKE_CURRENT_SOURCE_DIR}/lighting.h
  ${CMAKE_CURRENT_SOURCE_DIR}/line.h
  ${CMAKE_CURRENT_SOURCE_DIR}/linklist.h
  ${CMAKE_CURRENT_SOURCE_DIR}/localize.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mainhallmenu.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mainhalltemp.h
  ${CMAKE_CURRENT_SOURCE_DIR}/managepilot.h
  ${CMAKE_CURRENT_SOURCE_DIR}/medals.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionbriefcommon.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionbrief.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missioncampaign.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missioncmdbrief.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missiondebrief.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missiongoals.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missiongrid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionhotkey.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionload.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionlog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionloopbrief.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionmessage.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionparse.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionpause.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionrecommend.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionscreencommon.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionshipchoice.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionstats.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missiontraining.h
  ${CMAKE_CURRENT_SOURCE_DIR}/missionweaponchoice.h
  ${CMAKE_CURRENT_SOURCE_DIR}/model.h
  ${CMAKE_CURRENT_SOURCE_DIR}/modelsinc.h
  ${CMAKE_CURRENT_SOURCE_DIR}/monopub.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mouse.h
  ${CMAKE_CURRENT_SOURCE_DIR}/movie.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_campaign.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_data.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_dogfight.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_endgame.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_fstracker.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_ingame.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_kick.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multilag.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_log.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multimsgs.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_obj.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_observer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_oo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_options.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_pause.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_pinfo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_ping.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_pmsg.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_pxo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_rate.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_respawn.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_sw.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_team.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multiteamselect.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multiui.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_update.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multiutil.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_voice.h
  ${CMAKE_CURRENT_SOURCE_DIR}/multi_xfer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/muzzleflash.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mvelib.h
  ${CMAKE_CURRENT_SOURCE_DIR}/neb.h
  ${CMAKE_CURRENT_SOURCE_DIR}/neblightning.h
  ${CMAKE_CURRENT_SOURCE_DIR}/nebula.h
  ${CMAKE_CURRENT_SOURCE_DIR}/oal_capture.h
  ${CMAKE_CURRENT_SOURCE_DIR}/oal_efx.h
  ${CMAKE_CURRENT_SOURCE_DIR}/oal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/objcollide.h
  ${CMAKE_CURRENT_SOURCE_DIR}/object.h
  ${CMAKE_CURRENT_SOURCE_DIR}/objectsnd.h
  ${CMAKE_CURRENT_SOURCE_DIR}/observer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/optionsmenu.h
  ${CMAKE_CURRENT_SOURCE_DIR}/optionsmenumulti.h
  ${CMAKE_CURRENT_SOURCE_DIR}/osapi.h
  ${CMAKE_CURRENT_SOURCE_DIR}/osregistry.h
  ${CMAKE_CURRENT_SOURCE_DIR}/outwnd.h
  ${CMAKE_CURRENT_SOURCE_DIR}/packunpack.h
  ${CMAKE_CURRENT_SOURCE_DIR}/palman.h
  ${CMAKE_CURRENT_SOURCE_DIR}/parselo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/particle.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pcxutils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/physics.h
  ${CMAKE_CURRENT_SOURCE_DIR}/player.h
  ${CMAKE_CURRENT_SOURCE_DIR}/playermenu.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/popupdead.h
  ${CMAKE_CURRENT_SOURCE_DIR}/popup.h
  ${CMAKE_CURRENT_SOURCE_DIR}/psnet2.h
  ${CMAKE_CURRENT_SOURCE_DIR}/psnet.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pstypes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ptrack.h
  ${CMAKE_CURRENT_SOURCE_DIR}/radar.h
  ${CMAKE_CURRENT_SOURCE_DIR}/readyroom.h
  ${CMAKE_CURRENT_SOURCE_DIR}/rect.h
  ${CMAKE_CURRENT_SOURCE_DIR}/redalert.h
  ${CMAKE_CURRENT_SOURCE_DIR}/resource.h
  ${CMAKE_CURRENT_SOURCE_DIR}/rtvoice.h
  ${CMAKE_CURRENT_SOURCE_DIR}/scoring.h
  ${CMAKE_CURRENT_SOURCE_DIR}/sexp.h
  ${CMAKE_CURRENT_SOURCE_DIR}/sexp_tree.h
  ${CMAKE_CURRENT_SOURCE_DIR}/shipcontrails.h
  ${CMAKE_CURRENT_SOURCE_DIR}/shipfx.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ship.h
  ${CMAKE_CURRENT_SOURCE_DIR}/shiphit.h
  ${CMAKE_CURRENT_SOURCE_DIR}/shipspecialdamage.h
  ${CMAKE_CURRENT_SOURCE_DIR}/shockwave.h
  ${CMAKE_CURRENT_SOURCE_DIR}/snazzyui.h
  ${CMAKE_CURRENT_SOURCE_DIR}/sound.h
  ${CMAKE_CURRENT_SOURCE_DIR}/spline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/stand_server.h
  ${CMAKE_CURRENT_SOURCE_DIR}/standalone_html.h
  ${CMAKE_CURRENT_SOURCE_DIR}/starfield.h
  ${CMAKE_CURRENT_SOURCE_DIR}/staticrand.h
  ${CMAKE_CURRENT_SOURCE_DIR}/stats.h
  ${CMAKE_CURRENT_SOURCE_DIR}/subsysdamage.h
  ${CMAKE_CURRENT_SOURCE_DIR}/supernova.h
  ${CMAKE_CURRENT_SOURCE_DIR}/swarm.h
  ${CMAKE_CURRENT_SOURCE_DIR}/systemvars.h
  ${CMAKE_CURRENT_SOURCE_DIR}/techmenu.h
  ${CMAKE_CURRENT_SOURCE_DIR}/tga.h
  ${CMAKE_CURRENT_SOURCE_DIR}/tgautils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/timer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/tmapper.h
  ${CMAKE_CURRENT_SOURCE_DIR}/trails.h
  ${CMAKE_CURRENT_SOURCE_DIR}/trainingmenu.h
  ${CMAKE_CURRENT_SOURCE_DIR}/uidefs.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ui.h
  ${CMAKE_CURRENT_SOURCE_DIR}/valid.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vecmat.h
  ${CMAKE_CURRENT_SOURCE_DIR}/version.h
  ${CMAKE_CURRENT_SOURCE_DIR}/weapon.h
  PARENT_SCOPE
)


set(platform_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/platform.h
  ${CMAKE_CURRENT_SOURCE_DIR}/unix.h
  ${CMAKE_CURRENT_SOURCE_DIR}/win.h
  PARENT_SCOPE
)

#
# the game itself
#

set(freespace_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/freespace.h
  ${CMAKE_CURRENT_SOURCE_DIR}/levelpaging.h
  PARENT_SCOPE
)

#
# launcher
#

set(launcher_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher.h
  ${CMAKE_CURRENT_SOURCE_DIR}/launchersetup.h
  PARENT_SCOPE
)

#
# standalone server GUI
#

set(standalone_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/stand_gui.h
  PARENT_SCOPE
)

#
# ##############################################################################

# ##############################################################################
#
# toolset
#

#
# AC: anim converter
#

set(ac_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/ac.h
  ${CMAKE_CURRENT_SOURCE_DIR}/convert.h
  PARENT_SCOPE
)

#
# POFVIEW: model viewer
#

set(pofview_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/pofview.h
  PARENT_SCOPE
)

#
# SCRAMBLE: file-based encryption for TBLs
#

set(scramble_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/scramble.h
  PARENT_SCOPE
)

#
# FONTTOOL: create font files / edit kerning data
#

set(fonttool_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/fonttool.h
  PARENT_SCOPE
)

#
# ##############################################################################
