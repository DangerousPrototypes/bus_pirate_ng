##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

GIT_VERSION := $(shell git describe --dirty --always --tags)

CFLAGS +=-DFWVER=\"$(GIT_VERSION)\" 

OBJS += debug.o cdcacm.o UI.o protocols.o dummy1.o dummy2.o HiZ.o HWSPI.o 
OBJS += AUXpin.o ADC.o HWUSART.o bpflash.o HWI2C.o LA.o SW2W.o SW3W.o sump.o
OBJS += DIO.o LCDSPI.o HD44780.o ST7735.o 1WIRE.o

BINARY = buspirateNG

include ../Makefile.include



