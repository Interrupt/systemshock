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
#include <iostream>

#define FIXDEBUG

#include "fixpp.h"

int main( void )
{
   int                c =  1;
   signed int        sc =  2;
   unsigned int      uc =  3;

   int               s =  4;
   signed int        ss =  5;
   unsigned int      us =  6;

   int                i =  7;
   signed int        si =  8;
   unsigned int      ui =  9;

   long               l = 10;
   signed long       sl = 11;
   unsigned long int ul = 12;

   float              f = 13.3;
   double             d = 14.4;

#  define CONST 15.5





   // Init: fp = other
   // ================

   Fixpoint fa =  c;
   Fixpoint fb = sc;
   Fixpoint fc = uc;

   Fixpoint fd =  s;
   Fixpoint fe = ss;
   Fixpoint ff = us;

   Fixpoint fg =  i;
   Fixpoint fh = si;
   Fixpoint fi = ui;

   Fixpoint fj =  l;
   Fixpoint fk = sl;
   Fixpoint fl = ul;

   Fixpoint fm =  f;
   Fixpoint fn =  d;

   Fixpoint fo = CONST;

   std::cout << fa << "\n" << fb << "\n" << fc << "\n" << fd << "\n" << fe
        << "\n" << ff << "\n" << fg << "\n" << fh << "\n" << fi << "\n"
        << fj << "\n" << fk << "\n" << fl << "\n" << fm << "\n" << fn
        << "\n" << fo << "\n--------------------\n" ;








   // Init: other = fp;
   // ================

   {
      int               c =                fa.to_lint();
      signed int       sc =        fb.to_lint();
      unsigned int     uc =       fc.to_lint();

      int              s =           fd.to_lint();
      signed int      ss =    fe.to_lint();
      unsigned int    us =  ff.to_lint();

      int                i = fg.to_lint();
      signed int        si = fh.to_lint();
      unsigned int      ui = fi.to_lint();

      long               l = fj.to_lint();
      signed long       sl = fk.to_lint();
      unsigned long int ul = fl.to_lint();

      float              f = fm.to_double();
      double             d = fn.to_double();

      std::cout << c << "\n" << sc << "\n" << uc << "\n" <<  s << "\n" << ss
           << "\n" << us << "\n" <<  i << "\n" << si << "\n" << ui << "\n"
           <<  l   << "\n" << sl << "\n" << ul << "\n" <<  f << "\n"
           << d << "\n--------------------\n" ;
   }






   // Make 'em all the same for ease of checking math by lazy eye.
   // ============================================================

   {
      int i=10;
      fa = fb = fc = fd = fe = ff = fg = fh = fi = fj = fk = fl = fm = fn = i;
   }






   // Homo math.
   // ==========

   std::cout << (fc = fa * fb) << "\n";
   std::cout << (fd = fc / fa) << "\n";
   std::cout << (fe = fd + fa) << "\n";
   std::cout << (ff = fe - fb) << "\n";

   std::cout << (fa += fb) << "\n";
   std::cout << (fa -= fc) << "\n";
   std::cout << (fa *= fd) << "\n";
   std::cout << (fa /= fe) << "\n";

   std::cout << (fa = -fa) << "\n";
   std::cout << (fa = +fa) << "\n";

   std::cout << (fa  < fb) << "\n";
   std::cout << (fa  > fb) << "\n";
   std::cout << (fa <= fb) << "\n";
   std::cout << (fa <= fb) << "\n";
   std::cout << (fa == fb) << "\n";
   std::cout << (fa != fd) << "\n";


   std::cout << "--------\n" ;


   {
      int i=10;
      fa = fb = fc = fd = fe = ff = fg = fh = fi = fj = fk = fl = fm = fn = i;
   }



   // Fp = other
   // ==========


   std::cout << (fa =  c) << "\n";
   std::cout << (fb = sc) << "\n";
   std::cout << (fc = uc) << "\n";

   std::cout << (fd =  s) << "\n";
   std::cout << (fe = ss) << "\n";
   std::cout << (ff = us) << "\n";

   std::cout << (fg =  i) << "\n";
   std::cout << (fh = si) << "\n";
   std::cout << (fi = ui) << "\n";

   std::cout << (fj =  l) << "\n";
   std::cout << (fk = sl) << "\n";
   std::cout << (fl = ul) << "\n";

   std::cout << (fm =  f) << "\n";
   std::cout << (fn =  d) << "\n";
   std::cout << (fo = CONST) << "\n";


   std::cout << "--------\n" ;


   // other = Fp
   // ==========


   std::cout << ( c = (char)               fa.to_lint()) << "\n";
   std::cout << (sc = (signed char)        fb.to_lint()) << "\n";
   std::cout << (uc = (unsigned char)      fc.to_lint()) << "\n";

   std::cout << ( s = (short int)          fd.to_lint()) << "\n";
   std::cout << (ss = (signed short int)   fe.to_lint()) << "\n";
   std::cout << (us = (unsigned short int) ff.to_lint()) << "\n";

   std::cout << ( i = fg.to_lint()) << "\n";
   std::cout << (si = fh.to_lint()) << "\n";
   std::cout << (ui = fi.to_lint()) << "\n";

   std::cout << ( l = fj.to_lint()) << "\n";
   std::cout << (sl = fk.to_lint()) << "\n";
   std::cout << (ul = fl.to_lint()) << "\n";

   std::cout << ( f = fm.to_double()) << "\n";
   std::cout << ( d = fn.to_double()) << "\n";


   std::cout << "--------\n" ;




   // Fp < other
   // ==========


   std::cout << (fa <  c) << "\n" ;
   std::cout << (fb < sc) << "\n" ;
   std::cout << (fc < uc) << "\n" ;

   std::cout << (fd <  s) << "\n" ;
   std::cout << (fe < ss) << "\n" ;
   std::cout << (ff < us) << "\n" ;

   std::cout << (fg <  i) << "\n" ;
   std::cout << (fh < si) << "\n" ;
   std::cout << (fi < ui) << "\n" ;

   std::cout << (fj <  l) << "\n" ;
   std::cout << (fk < sl) << "\n" ;
   std::cout << (fl < ul) << "\n" ;

   std::cout << (fm <  f) << "\n" ;
   std::cout << (fn <  d) << "\n" ;
   std::cout << (fn < CONST) << "\n";


   std::cout << "--------\n" ;


   std::cout << (fa >  c) << "\n" ;
   std::cout << (fb > sc) << "\n" ;
   std::cout << (fc > uc) << "\n" ;

   std::cout << (fd >  s) << "\n" ;
   std::cout << (fe > ss) << "\n" ;
   std::cout << (ff > us) << "\n" ;

   std::cout << (fg >  i) << "\n" ;
   std::cout << (fh > si) << "\n" ;
   std::cout << (fi > ui) << "\n" ;

   std::cout << (fj >  l) << "\n" ;
   std::cout << (fk > sl) << "\n" ;
   std::cout << (fl > ul) << "\n" ;

   std::cout << (fm >  f) << "\n" ;
   std::cout << (fn >  d) << "\n" ;
   std::cout << (fn > CONST) << "\n";


   std::cout << "--------\n" ;


   std::cout << (fa <=  c) << "\n" ;
   std::cout << (fb <= sc) << "\n" ;
   std::cout << (fc <= uc) << "\n" ;

   std::cout << (fd <=  s) << "\n" ;
   std::cout << (fe <= ss) << "\n" ;
   std::cout << (ff <= us) << "\n" ;

   std::cout << (fg <=  i) << "\n" ;
   std::cout << (fh <= si) << "\n" ;
   std::cout << (fi <= ui) << "\n" ;

   std::cout << (fj <=  l) << "\n" ;
   std::cout << (fk <= sl) << "\n" ;
   std::cout << (fl <= ul) << "\n" ;

   std::cout << (fm <=  f) << "\n" ;
   std::cout << (fn <=  d) << "\n" ;
   std::cout << (fn <= CONST) << "\n";


   std::cout << "--------\n" ;


   std::cout << (fa >=  c) << "\n" ;
   std::cout << (fb >= sc) << "\n" ;
   std::cout << (fc >= uc) << "\n" ;

   std::cout << (fd >=  s) << "\n" ;
   std::cout << (fe >= ss) << "\n" ;
   std::cout << (ff >= us) << "\n" ;

   std::cout << (fg >=  i) << "\n" ;
   std::cout << (fh >= si) << "\n" ;
   std::cout << (fi >= ui) << "\n" ;

   std::cout << (fj >=  l) << "\n" ;
   std::cout << (fk >= sl) << "\n" ;
   std::cout << (fl >= ul) << "\n" ;

   std::cout << (fm >=  f) << "\n" ;
   std::cout << (fn >=  d) << "\n" ;
   std::cout << (fn >= CONST) << "\n";


   std::cout << "--------\n" ;


   std::cout << (fa ==  c) << "\n" ;
   std::cout << (fb == sc) << "\n" ;
   std::cout << (fc == uc) << "\n" ;

   std::cout << (fd ==  s) << "\n" ;
   std::cout << (fe == ss) << "\n" ;
   std::cout << (ff == us) << "\n" ;

   std::cout << (fg ==  i) << "\n" ;
   std::cout << (fh == si) << "\n" ;
   std::cout << (fi == ui) << "\n" ;

   std::cout << (fj ==  l) << "\n" ;
   std::cout << (fk == sl) << "\n" ;
   std::cout << (fl == ul) << "\n" ;

   std::cout << (fm ==  f) << "\n" ;
   std::cout << (fn ==  d) << "\n" ;
   std::cout << (fn == CONST) << "\n";

   std::cout << "--------\n" ;

   std::cout << (fa !=  c) << "\n" ;
   std::cout << (fb != sc) << "\n" ;
   std::cout << (fc != uc) << "\n" ;

   std::cout << (fd !=  s) << "\n" ;
   std::cout << (fe != ss) << "\n" ;
   std::cout << (ff != us) << "\n" ;

   std::cout << (fg !=  i) << "\n" ;
   std::cout << (fh != si) << "\n" ;
   std::cout << (fi != ui) << "\n" ;

   std::cout << (fj !=  l) << "\n" ;
   std::cout << (fk != sl) << "\n" ;
   std::cout << (fl != ul) << "\n" ;

   std::cout << (fm !=  f) << "\n" ;
   std::cout << (fn !=  d) << "\n" ;
   std::cout << (fn != CONST) << "\n";


   std::cout << "--------\n" ;
   std::cout << "--------\n" ;







#ifdef BAD_MIX


   // other < Fp
   //
   // BAD!!!!
   // ==========

   std::cout << (  c<fa ) << "\n" ;
   std::cout << ( sc<fb ) << "\n" ;
   std::cout << ( uc<fc ) << "\n" ;

   std::cout << (  s<fd ) << "\n" ;
   std::cout << ( ss<fe ) << "\n" ;
   std::cout << ( us<ff ) << "\n" ;

   std::cout << (  i<fg ) << "\n" ;
   std::cout << ( si<fh ) << "\n" ;
   std::cout << ( ui<fi ) << "\n" ;

   std::cout << (  l<fj ) << "\n" ;
   std::cout << ( sl<fk ) << "\n" ;
   std::cout << ( ul<fl ) << "\n" ;

   std::cout << (  f<fm ) << "\n" ;
   std::cout << (  d<fn ) << "\n" ;

   std::cout << (CONST<fo) << "\n";


   std::cout << "--------\n" ;


   std::cout << (  c>fa ) << "\n" ;
   std::cout << ( sc>fb ) << "\n" ;
   std::cout << ( uc>fc ) << "\n" ;

   std::cout << (  s>fd ) << "\n" ;
   std::cout << ( ss>fe ) << "\n" ;
   std::cout << ( us>ff ) << "\n" ;

   std::cout << (  i>fg ) << "\n" ;
   std::cout << ( si>fh ) << "\n" ;
   std::cout << ( ui>fi ) << "\n" ;

   std::cout << (  l>fj ) << "\n" ;
   std::cout << ( sl>fk ) << "\n" ;
   std::cout << ( ul>fl ) << "\n" ;

   std::cout << (  f>fm ) << "\n" ;
   std::cout << (  d>fn ) << "\n" ;

   std::cout << (CONST>fo) << "\n";


   std::cout << "--------\n" ;


   std::cout << (  c<=fa ) << "\n" ;
   std::cout << ( sc<=fb ) << "\n" ;
   std::cout << ( uc<=fc ) << "\n" ;

   std::cout << (  s<=fd ) << "\n" ;
   std::cout << ( ss<=fe ) << "\n" ;
   std::cout << ( us<=ff ) << "\n" ;

   std::cout << (  i<=fg ) << "\n" ;
   std::cout << ( si<=fh ) << "\n" ;
   std::cout << ( ui<=fi ) << "\n" ;

   std::cout << (  l<=fj ) << "\n" ;
   std::cout << ( sl<=fk ) << "\n" ;
   std::cout << ( ul<=fl ) << "\n" ;

   std::cout << (  f<=fm ) << "\n" ;
   std::cout << (  d<=fn ) << "\n" ;

   std::cout << (CONST<=fo) << "\n";

   std::cout << "--------\n" ;


   std::cout << (  c>=fa ) << "\n" ;
   std::cout << ( sc>=fb ) << "\n" ;
   std::cout << ( uc>=fc ) << "\n" ;

   std::cout << (  s>=fd ) << "\n" ;
   std::cout << ( ss>=fe ) << "\n" ;
   std::cout << ( us>=ff ) << "\n" ;

   std::cout << (  i>=fg ) << "\n" ;
   std::cout << ( si>=fh ) << "\n" ;
   std::cout << ( ui>=fi ) << "\n" ;

   std::cout << (  l>=fj ) << "\n" ;
   std::cout << ( sl>=fk ) << "\n" ;
   std::cout << ( ul>=fl ) << "\n" ;

   std::cout << (  f>=fm ) << "\n" ;
   std::cout << (  d>=fn ) << "\n" ;

   std::cout << (CONST>=fo) << "\n";

   std::cout << "--------\n" ;


   std::cout << (  c==fa ) << "\n" ;
   std::cout << ( sc==fb ) << "\n" ;
   std::cout << ( uc==fc ) << "\n" ;

   std::cout << (  s==fd ) << "\n" ;
   std::cout << ( ss==fe ) << "\n" ;
   std::cout << ( us==ff ) << "\n" ;

   std::cout << (  i==fg ) << "\n" ;
   std::cout << ( si==fh ) << "\n" ;
   std::cout << ( ui==fi ) << "\n" ;

   std::cout << (  l==fj ) << "\n" ;
   std::cout << ( sl==fk ) << "\n" ;
   std::cout << ( ul==fl ) << "\n" ;

   std::cout << (  f==fm ) << "\n" ;
   std::cout << (  d==fn ) << "\n" ;

   std::cout << (CONST==fo) << "\n";

   std::cout << "--------\n" ;

   std::cout << (  c!=fa ) << "\n" ;
   std::cout << ( sc!=fb ) << "\n" ;
   std::cout << ( uc!=fc ) << "\n" ;

   std::cout << (  s!=fd ) << "\n" ;
   std::cout << ( ss!=fe ) << "\n" ;
   std::cout << ( us!=ff ) << "\n" ;

   std::cout << (  i!=fg ) << "\n" ;
   std::cout << ( si!=fh ) << "\n" ;
   std::cout << ( ui!=fi ) << "\n" ;

   std::cout << (  l!=fj ) << "\n" ;
   std::cout << ( sl!=fk ) << "\n" ;
   std::cout << ( ul!=fl ) << "\n" ;

   std::cout << (  f!=fm ) << "\n" ;
   std::cout << (  d!=fn ) << "\n" ;

   std::cout << (CONST!=fo) << "\n";

   std::cout << "--------\n" ;
   std::cout << "--------\n" ;




   // other += Fp
   //
   // BAD!!!!!!!!!
   // ============






   std::cout << (  c*=fa ) << "\n" ;
   std::cout << ( sc*=fb ) << "\n" ;
   std::cout << ( uc*=fc ) << "\n" ;

   std::cout << (  s*=fd ) << "\n" ;
   std::cout << ( ss*=fe ) << "\n" ;
   std::cout << ( us*=ff ) << "\n" ;

   std::cout << (  i*=fg ) << "\n" ;
   std::cout << ( si*=fh ) << "\n" ;
   std::cout << ( ui*=fi ) << "\n" ;

   std::cout << (  l*=fj ) << "\n" ;
   std::cout << ( sl*=fk ) << "\n" ;
   std::cout << ( ul*=fl ) << "\n" ;

   std::cout << (  f*=fm ) << "\n" ;
   std::cout << (  d*=fn ) << "\n" ;


   std::cout << "--------\n" ;

   std::cout << (  c+=fa ) << "\n" ;
   std::cout << ( sc+=fb ) << "\n" ;
   std::cout << ( uc+=fc ) << "\n" ;

   std::cout << (  s+=fd ) << "\n" ;
   std::cout << ( ss+=fe ) << "\n" ;
   std::cout << ( us+=ff ) << "\n" ;

   std::cout << (  i+=fg ) << "\n" ;
   std::cout << ( si+=fh ) << "\n" ;
   std::cout << ( ui+=fi ) << "\n" ;

   std::cout << (  l+=fj ) << "\n" ;
   std::cout << ( sl+=fk ) << "\n" ;
   std::cout << ( ul+=fl ) << "\n" ;

   std::cout << (  f+=fm ) << "\n" ;
   std::cout << (  d+=fn ) << "\n" ;


   std::cout << "--------\n" ;

   std::cout << (  c-=fa ) << "\n" ;
   std::cout << ( sc-=fb ) << "\n" ;
   std::cout << ( uc-=fc ) << "\n" ;

   std::cout << (  s-=fd ) << "\n" ;
   std::cout << ( ss-=fe ) << "\n" ;
   std::cout << ( us-=ff ) << "\n" ;

   std::cout << (  i-=fg ) << "\n" ;
   std::cout << ( si-=fh ) << "\n" ;
   std::cout << ( ui-=fi ) << "\n" ;

   std::cout << (  l-=fj ) << "\n" ;
   std::cout << ( sl-=fk ) << "\n" ;
   std::cout << ( ul-=fl ) << "\n" ;

   std::cout << (  f-=fm ) << "\n" ;
   std::cout << (  d-=fn ) << "\n" ;


   std::cout << "--------\n" ;

   std::cout << (  c/=fa ) << "\n" ;
   std::cout << ( sc/=fb ) << "\n" ;
   std::cout << ( uc/=fc ) << "\n" ;

   std::cout << (  s/=fd ) << "\n" ;
   std::cout << ( ss/=fe ) << "\n" ;
   std::cout << ( us/=ff ) << "\n" ;

   std::cout << (  i/=fg ) << "\n" ;
   std::cout << ( si/=fh ) << "\n" ;
   std::cout << ( ui/=fi ) << "\n" ;

   std::cout << (  l/=fj ) << "\n" ;
   std::cout << ( sl/=fk ) << "\n" ;
   std::cout << ( ul/=fl ) << "\n" ;

   std::cout << (  f/=fm ) << "\n" ;
   std::cout << (  d/=fn ) << "\n" ;


   std::cout << "--------\n" ;


#endif /* BADMIX */




   // Fp += other
   // ===========


   std::cout << (fa *=  c) << "\n" ;
   std::cout << (fb *= sc) << "\n" ;
   std::cout << (fc *= uc) << "\n" ;

   std::cout << (fd *=  s) << "\n" ;
   std::cout << (fe *= ss) << "\n" ;
   std::cout << (ff *= us) << "\n" ;

   std::cout << (fg *=  i) << "\n" ;
   std::cout << (fh *= si) << "\n" ;
   std::cout << (fi *= ui) << "\n" ;

   std::cout << (fj *=  l) << "\n" ;
   std::cout << (fk *= sl) << "\n" ;
   std::cout << (fl *= ul) << "\n" ;

   std::cout << (fm *=  f) << "\n" ;
   std::cout << (fn *=  d) << "\n" ;

   std::cout << (fo *= CONST) << "\n";


   std::cout << "--------\n" ;

   std::cout << (fa +=  c) << "\n" ;
   std::cout << (fb += sc) << "\n" ;
   std::cout << (fc += uc) << "\n" ;

   std::cout << (fd +=  s) << "\n" ;
   std::cout << (fe += ss) << "\n" ;
   std::cout << (ff += us) << "\n" ;

   std::cout << (fg +=  i) << "\n" ;
   std::cout << (fh += si) << "\n" ;
   std::cout << (fi += ui) << "\n" ;

   std::cout << (fj +=  l) << "\n" ;
   std::cout << (fk += sl) << "\n" ;
   std::cout << (fl += ul) << "\n" ;

   std::cout << (fm +=  f) << "\n" ;
   std::cout << (fn +=  d) << "\n" ;

   std::cout << (fo += CONST) << "\n";

   std::cout << "--------\n" ;

   std::cout << (fa -=  c) << "\n" ;
   std::cout << (fb -= sc) << "\n" ;
   std::cout << (fc -= uc) << "\n" ;

   std::cout << (fd -=  s) << "\n" ;
   std::cout << (fe -= ss) << "\n" ;
   std::cout << (ff -= us) << "\n" ;

   std::cout << (fg -=  i) << "\n" ;
   std::cout << (fh -= si) << "\n" ;
   std::cout << (fi -= ui) << "\n" ;

   std::cout << (fj -=  l) << "\n" ;
   std::cout << (fk -= sl) << "\n" ;
   std::cout << (fl -= ul) << "\n" ;

   std::cout << (fm -=  f) << "\n" ;
   std::cout << (fn -=  d) << "\n" ;

   std::cout << (fo -= CONST) << "\n";

   std::cout << "--------\n" ;

   std::cout << (fa /=  c) << "\n" ;
   std::cout << (fb /= sc) << "\n" ;
   std::cout << (fc /= uc) << "\n" ;

   std::cout << (fd /=  s) << "\n" ;
   std::cout << (fe /= ss) << "\n" ;
   std::cout << (ff /= us) << "\n" ;

   std::cout << (fg /=  i) << "\n" ;
   std::cout << (fh /= si) << "\n" ;
   std::cout << (fi /= ui) << "\n" ;

   std::cout << (fj /=  l) << "\n" ;
   std::cout << (fk /= sl) << "\n" ;
   std::cout << (fl /= ul) << "\n" ;

   std::cout << (fm /=  f) << "\n" ;
   std::cout << (fn /=  d) << "\n" ;

   std::cout << (fo /= CONST) << "\n";

   std::cout << "--------\n" ;




   // Fp + other
   // ==========


   std::cout << fa *  c << "\n" ;
   std::cout << fb * sc << "\n" ;
   std::cout << fc * uc << "\n" ;

   std::cout << fd *  s << "\n" ;
   std::cout << fe * ss << "\n" ;
   std::cout << ff * us << "\n" ;

   std::cout << fg *  i << "\n" ;
   std::cout << fh * si << "\n" ;
   std::cout << fi * ui << "\n" ;

   std::cout << fj *  l << "\n" ;
   std::cout << fk * sl << "\n" ;
   std::cout << fl * ul << "\n" ;

   std::cout << fm *  f << "\n" ;
   std::cout << fn *  d << "\n" ;

   std::cout << fo * CONST << "\n";


   std::cout << "--------\n" ;



   std::cout << fa -  c << "\n" ;
   std::cout << fb - sc << "\n" ;
   std::cout << fc - uc << "\n" ;

   std::cout << fd -  s << "\n" ;
   std::cout << fe - ss << "\n" ;
   std::cout << ff - us << "\n" ;

   std::cout << fg -  i << "\n" ;
   std::cout << fh - si << "\n" ;
   std::cout << fi - ui << "\n" ;

   std::cout << fj -  l << "\n" ;
   std::cout << fk - sl << "\n" ;
   std::cout << fl - ul << "\n" ;

   std::cout << fm -  f << "\n" ;
   std::cout << fn -  d << "\n" ;

   std::cout << fo - CONST << "\n";


   std::cout << "--------\n" ;



   std::cout << fa +  c << "\n" ;
   std::cout << fb + sc << "\n" ;
   std::cout << fc + uc << "\n" ;

   std::cout << fd +  s << "\n" ;
   std::cout << fe + ss << "\n" ;
   std::cout << ff + us << "\n" ;

   std::cout << fg +  i << "\n" ;
   std::cout << fh + si << "\n" ;
   std::cout << fi + ui << "\n" ;

   std::cout << fj +  l << "\n" ;
   std::cout << fk + sl << "\n" ;
   std::cout << fl + ul << "\n" ;

   std::cout << fm +  f << "\n" ;
   std::cout << fn +  d << "\n" ;

   std::cout << fo + CONST << "\n";

   std::cout << "--------\n" ;


   std::cout << fa /  c << "\n" ;
   std::cout << fb / sc << "\n" ;
   std::cout << fc / uc << "\n" ;

   std::cout << fd /  s << "\n" ;
   std::cout << fe / ss << "\n" ;
   std::cout << ff / us << "\n" ;

   std::cout << fg /  i << "\n" ;
   std::cout << fh / si << "\n" ;
   std::cout << fi / ui << "\n" ;

   std::cout << fj /  l << "\n" ;
   std::cout << fk / sl << "\n" ;
   std::cout << fl / ul << "\n" ;

   std::cout << fm /  f << "\n" ;
   std::cout << fn /  d << "\n" ;

   std::cout << fo / CONST << "\n";

   std::cout << "--------\n" ;
   std::cout << "--------\n" ;





   // other + Fp
   // ==========


   std::cout <<  c / fa << "\n" ;
   std::cout << sc / fb << "\n" ;
   std::cout << uc / fc << "\n" ;

   std::cout <<  s / fd << "\n" ;
   std::cout << ss / fe << "\n" ;
   std::cout << us / ff << "\n" ;

   std::cout <<  i / fg << "\n" ;
   std::cout << si / fh << "\n" ;
   std::cout << ui / fi << "\n" ;

   std::cout <<  l / fj << "\n" ;
   std::cout << sl / fk << "\n" ;
   std::cout << ul / fl << "\n" ;

   std::cout <<  f / fm << "\n" ;
   std::cout <<  d / fn << "\n" ;

   std::cout << CONST / fo << "\n";

   std::cout << "--------\n" ;

   std::cout <<  c * fa << "\n" ;
   std::cout << sc * fb << "\n" ;
   std::cout << uc * fc << "\n" ;

   std::cout <<  s * fd << "\n" ;
   std::cout << ss * fe << "\n" ;
   std::cout << us * ff << "\n" ;

   std::cout <<  i * fg << "\n" ;
   std::cout << si * fh << "\n" ;
   std::cout << ui * fi << "\n" ;

   std::cout <<  l * fj << "\n" ;
   std::cout << sl * fk << "\n" ;
   std::cout << ul * fl << "\n" ;

   std::cout <<  f * fm << "\n" ;
   std::cout <<  d * fn << "\n" ;

   std::cout << CONST * fo << "\n";

   std::cout << "--------\n" ;


   std::cout <<  c - fa << "\n" ;
   std::cout << sc - fb << "\n" ;
   std::cout << uc - fc << "\n" ;

   std::cout <<  s - fd << "\n" ;
   std::cout << ss - fe << "\n" ;
   std::cout << us - ff << "\n" ;

   std::cout <<  i - fg << "\n" ;
   std::cout << si - fh << "\n" ;
   std::cout << ui - fi << "\n" ;

   std::cout <<  l - fj << "\n" ;
   std::cout << sl - fk << "\n" ;
   std::cout << ul - fl << "\n" ;

   std::cout <<  f - fm << "\n" ;
   std::cout <<  d - fn << "\n" ;

   std::cout << CONST - fo << "\n";

   std::cout << "--------\n" ;

   std::cout <<  c + fa << "\n" ;
   std::cout << sc + fb << "\n" ;
   std::cout << uc + fc << "\n" ;

   std::cout <<  s + fd << "\n" ;
   std::cout << ss + fe << "\n" ;
   std::cout << us + ff << "\n" ;

   std::cout <<  i + fg << "\n" ;
   std::cout << si + fh << "\n" ;
   std::cout << ui + fi << "\n" ;

   std::cout <<  l + fj << "\n" ;
   std::cout << sl + fk << "\n" ;
   std::cout << ul + fl << "\n" ;

   std::cout <<  f + fm << "\n" ;
   std::cout <<  d + fn << "\n" ;

   std::cout << CONST + fo << "\n\n";


// Trig things
// ===========

   Fixpoint Zero = 0,
            PI = 3.141592653589793,
            PI_over_2 = PI/2,
            PI_over_4 = PI/4,
            three_PI_over_2 = (3*PI)/2;

   std::cout << Zero.to_fixang() << "\n"
        << PI.to_fixang() << "\n"
        << PI_over_2.to_fixang() << "\n"
        << PI_over_4.to_fixang() << "\n"
        << three_PI_over_2.to_fixang() << "\n\n" ;

   Fixpoint zero, pi, pi_over_2, pi_over_4, three_pi_over_2;
   zero.fixang_to( 0U );
   pi.fixang_to( 0x8000 );
   pi_over_2.fixang_to( 0x4000 );
   pi_over_4.fixang_to( 0x2000 );
   three_pi_over_2.fixang_to( 0xC000 );

   std::cout << zero << "\n"
        << pi << "\n"
        << pi_over_2 << "\n" 
        << pi_over_4 << "\n"
        << three_pi_over_2 << "\n\n" ;

   std::cout
        << sin( 0 ) << "\n"
        << sin( PI ) << "\n"
        << sin( PI_over_2 ) << "\n"
        << sin( PI_over_4 ) << "\n"
        << sin( three_PI_over_2 ) << "\n"
        << sin( -PI ) << "\n"
        << sin( -PI_over_2 ) << "\n"
        << sin( -PI_over_4 ) << "\n"
        << sin( -three_PI_over_2 ) << "\n"

        << cos( 0 ) << "\n"
        << cos( PI ) << "\n"
        << cos( PI_over_2 ) << "\n"
        << cos( PI_over_4 ) << "\n"
        << cos( three_PI_over_2 ) << "\n"
        << cos( -PI ) << "\n"
        << cos( -PI_over_2 ) << "\n"
        << cos( -PI_over_4 ) << "\n"
        << cos( -three_PI_over_2 ) << "\n\n" ;


   Fixpoint one_over_root_two = 1./sqrt(2.);

   std::cout << one_over_root_two << "\n\n" ;

   Fixpoint One = 1.;


   std::cout
        << asin( Zero ) << "\n"
        << asin( one_over_root_two ) << "\n"
        << asin( One ) << "\n"
        << asin( -one_over_root_two ) << "\n"
        << asin( -One ) << "\n\n"

        << acos( Zero ) << "\n"
        << acos( one_over_root_two ) << "\n"
        << acos( One ) << "\n"
        << acos( -one_over_root_two ) << "\n"
        << acos( -One ) << "\n\n" ;

   std::cout << "Atan2 test\n\n"
        << atan2( 1, 2 ) << "\n"
        << atan2( 1, -2 ) << "\n"
        << atan2( -1, -2 ) << "\n"
        << atan2( -1, 2 ) << "\n"
        << atan2( 0, 1 ) << "\n"
        << atan2( 1, 1 ) << "\n"
        << atan2( 1, 0 ) << "\n"
        << atan2( 1, -1 ) << "\n"
        << atan2( 0, -1 ) << "\n"
        << atan2( -1, -1 ) << "\n"
        << atan2( -1, 0 ) << "\n"
        << atan2( -1, 1 ) << "\n\n" ;

   Fixpoint sn, cs;

   std::cout << "Sincos test\n\n" ;
   sincos( 0, &sn, &cs );
   std::cout << sn << " " << cs << "\n" ;
   sincos( PI, &sn, &cs );
   std::cout << sn << " " << cs << "\n" ;
   sincos( PI_over_2, &sn, &cs );
   std::cout << sn << " " << cs << "\n" ;
   sincos( PI_over_4, &sn, &cs );
   std::cout << sn << " " << cs << "\n" ;
   sincos( three_PI_over_2, &sn, &cs );
   std::cout << sn << " " << cs << "\n" ;
   sincos( -PI, &sn, &cs );
   std::cout << sn << " " << cs << "\n" ;
   sincos( -PI_over_2, &sn, &cs );
   std::cout << sn << " " << cs << "\n" ;
   sincos( -PI_over_4, &sn, &cs );
   std::cout << sn << " " << cs << "\n" ;
   sincos( -three_PI_over_2, &sn, &cs );
   std::cout << sn << " " << cs << "\n\n\n" ;

   long lang;
   fixang fang;
   Fixpoint Fang;
   std::cout << "Tan test\n\n" ;
   for (lang = 0; lang < 0x10000; lang += 0x2000)
   {
      fang = (fixang)lang;
      char str[20];
      sprintf( str, "%x ", fang );
      Fang.fixang_to( fang );
      std::cout << str << tan( Fang ) << "\n" ;
   }
   std::cout << "\n\n" ;

   Fixpoint :: report();
   

   return(0);
}

