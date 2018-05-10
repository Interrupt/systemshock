/*

Copyright (C) 2015-2018 Night Dive Studios, LLC.

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
#include "lg.h"
#include "error.h"
#include "mouse.h"
#include "vmouse.h"
//#include <mprintf.h>

void (*ui_mouse_convert)(short *px, short *py, uchar down) = NULL;
void (*ui_mouse_convert_round)(short *px, short *py, uchar down) = NULL;

errtype ui_mouse_do_conversion(short *pmx, short *pmy, uchar down)
{
   if (ui_mouse_convert != NULL)
      ui_mouse_convert(pmx,pmy,down);
   return(OK);
}

errtype ui_mouse_get_xy(short *pmx, short *pmy)
{
   errtype retval;
   retval = mouse_get_xy(pmx,pmy);
   ui_mouse_do_conversion(pmx,pmy,true);
   return(retval);
}

errtype ui_mouse_put_xy(short pmx, short pmy)
{
   errtype retval;
   ui_mouse_do_conversion(&pmx,&pmy,false);
   retval = mouse_put_xy(pmx,pmy);
   return(retval);
}

errtype ui_mouse_constrain_xy(short xl, short yl, short xh, short yh)
{
   if (ui_mouse_convert == NULL)
      return(mouse_constrain_xy(xl,yl,xh,yh));
   else
   {
      short uxl,uyl,uxh,uyh;
      uxl=xl;
      uyl=yl;
      uxh=xh;
      uyh=yh;
      ui_mouse_convert_round(&uxl,&uyl,false);
      ui_mouse_convert_round(&uxh,&uyh,false);
      return(mouse_constrain_xy(uxl,uyl,uxh,uyh));
   }
}
