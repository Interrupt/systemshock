/*

Copyright (C) 2020 Shockolate Project

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef EMAIL_H
#define EMAIL_H

#include "mfdint.h"

void add_email_handler(LGRegion *r);
uchar email_color_func(void *dp, int num);
char *email_name_func(void *dp, int num, char *buf);
void read_email(Id new_base, int num);
void select_email(int num, uchar scr);
void set_email_flags(int n);
void update_email_ware();
void email_page_exit(void);
void mfd_emailmug_expose(MFD *mfd, ubyte control);
uchar mfd_emailmug_handler(MFD *m, uiEvent *ev);
errtype mfd_emailware_init(MFD_Func *f);
void mfd_emailware_expose(MFD *mfd, ubyte control);

void email_turnon(uchar visible, uchar real_start);
void email_turnoff(uchar visible, uchar real_stop);

#endif
