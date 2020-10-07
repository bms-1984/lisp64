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

lisp64: lisp64.c mpc.c
	$(CC) -g -Wall $^ -lm -o $@

.PHONY: clean

clean:
	rm -f lisp64 *~
