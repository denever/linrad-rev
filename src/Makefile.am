# Copyright (C) 2005, 2006, 2007 Giuseppe Martino <denever@users.sf.net>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = xlinrad
xlinrad_SOURCES = adtest.c calsub.c cwspeed.c fft2var.c keyboard_var.c meter_graph.c perseus.c sdr14.c timing.c users.c wse_sdrxx.c afcsub.c\
	 calsub2.c eme.c fft3.c llsq.c mix1.c pol_graph.c sdrvar.c tune.c users_tr.c wsys.c afc_graph.c calvar.c extra.c fft3var.c llsqvar.c\
	 mix2.c powtim.c sellim.c tx.c users_w3sz.c wxsys.c baseb_graph.c cfft.c fft0.c fonts.c lmain.c modesub.c powtvar.c selvar.c txssb.c\
	 vgatest.c xmain.c blank1.c coherent.c fft1.c freq_control.c lsetad.c morse.c radar.c sigvar.c txtest.c wcw.c xsys.c blnkvar.c cohsub.c\
	 fft1var.c gifsave.c lsys.c mouse.c rxin.c spur.c txvar.c wide_graph.c xvar.c buf.c coh_osc.c fft1_complex.c help.c lvar.c mousetest.c\
	 screen.c spursub.c tx_graph.c winvar.c calibrate.c configure.c fft1_re.c hires_graph.c lxsys.c network.c screensub.c thrvar.c ui.c\
	 wmain.c caliq.c cwdetect.c fft2.c hwaredriver.c menu.c palette.c screenvar.c timf2.c uivar.c wsetad.c
xlinrad_CFLAGS = -ansi -Wall -pedantic -g -ggdb
xlinrad_LDADD = $(ALSA_LIBS) $(X11_LIBS) $(XEXT_LIBS) $(XPROTO_LIBS)
INCLUDES = -I$(top_srcdir)/include $(ALSA_CFLAGS) $(X11_CFLAGS) $(XEXT_CFLAGS) $(XPROTO_CFLAGS)


