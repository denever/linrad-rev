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
xlinrad_SOURCES = xmain.c ui.c uivar.c fft1var.c lsetad.c rxin.c wide_graph.c fft0.c fft1.c adtest.c screenvar.c sigvar.c\
	 baseb_graph.c afc_graph.c hires_graph.c palette.c gifsave.c calibrate.c calsub.c calsub2.c llsqvar.c\
	 caliq.c hwaredriver.c llsq.c sellim.c selvar.c mix1.c fft2var.c fft3var.c spursub.c network.c buf.c\
	 thrvar.c radar.c fft3.c timf2.c fft1_re.c fft1_complex.c powtvar.c wcw.c blank1.c calvar.c blnkvar.c\
	 fft2.c pol_graph.c modesub.c help.c afcsub.c timing.c mix2.c spur.c coherent.c cohsub.c eme.c tune.c\
	 txtest.c mouse.c freq_control.c morse.c lxsys.c lvar.c powtim.c cwspeed.c coh_osc.c cwdetect.c txvar.c\
	 tx.c menu.c screen.c sdrvar.c xsys.c sdr14.c keyboard_var.c xvar.c meter_graph.c fonts.c screensub.c\
	 tx_graph.c txssb.c wxsys.c 
xlinrad_CFLAGS = -g -ggdb
xlinrad_OBJASM = hware.os split.ol getiq.ol fftasm.ol fft2mmxb.ol fft2mmxa.ol fft2mmxc.ol simdasm.ol
xlinrad_LDADD = $(xlinrad_OBJASM) $(ALSA_LIBS) $(X11_LIBS) $(XEXT_LIBS) $(XPROTO_LIBS)
INCLUDES = -I$(top_srcdir)/include $(ALSA_CFLAGS) $(X11_CFLAGS) $(XEXT_CFLAGS) $(XPROTO_CFLAGS)

%.os : %.s
	$(NASM) -w-orphan-labels -felf -s $< -o $@

%.ol : %.sl %.s
	$(NASM) -w-orphan-labels -felf -s $< -o $@
