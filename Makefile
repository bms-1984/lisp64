#    Liz is a custom Lisp implementation.
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

WARNINGS     	?= -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
	           -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations   \
                   -Wredundant-decls -Wnested-externs -Winline -Wno-long-long    \
                   -Wconversion -Wstrict-prototypes
DEBUG		?= -g
CFLAGS		:= $(WARNINGS) $(DEBUG) -MMD -MP
LIBS		:= -lm

VERSION		:= 0.1.0
TARGET		:= liz

SRCFILES	:= $(shell find . -type f -name "*.c")
HDRFILES    	:= $(shell find . -type f -name "*.h")
DEPFILES	:= $(SRCFILES:.c=.d)
AUXFILES	:= LICENSE Makefile lib.lisp
ALLFILES	:= $(SRCFILES) $(AUXFILES) $(HDRFILES)
DISTFILE	:= $(TARGET)-$(VERSION).tar.xz
CLEANFILES	:= $(TARGET) $(DISTFILE) $(DEPFILES)

-include $(DEPFILES)

.PHONY: clean dist

$(TARGET): $(SRCFILES)
	@$(CC) $(CFLAGS) $^ -o $@ -D VERSION=\"$(VERSION)\" $(LIBS)
	$(info All done!)

clean:
	@$(RM) -rf $(wildcard $(CLEANFILES))
	$(info All clean!)

dist:
	@mkdir $(TARGET)-$(VERSION)
	@cp -R $(wildcard $(ALLFILES)) $(TARGET)-$(VERSION)
	@tar cJf $(DISTFILE) $(TARGET)-$(VERSION)
	@$(RM) -rf $(TARGET)-$(VERSION)
	$(info All packed!)
