#    Lisp64 is a custom Lisp implementation.
#    Copyright (C) 2020 Ben M. Sutter
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.

CFLAGS		?= -g -Wall
LIBS		:= -lm
TARGET		:= liz
SRCFILES	:= $(shell find . -type f -name "*.c")
AUXFILES	:= LICENSE Makefile lib.lisp mpc.h
ALLFILES	:= $(SRCFILES) $(ALLFILES)
VERSION		:= 0.1.0
DISTFILE	:= $(TARGET)-$(VERSION).tar.gz
CLEANFILES	:= $(TARGET) $(DISTFILE)

.PHONY: clean dist

$(TARGET): $(SRCFILES)
	@$(CC) $(CFLAGS) $^ -o $@ -D VERSION=\"$(VERSION)\" $(LIBS)
	@echo All done!

clean:
	@$(RM) -rf $(wildcard $(CLEANFILES))
	@echo All clean!

dist:
	@tar cJf $(DISTFILE) $(ALLFILES)
	@echo All packed!
