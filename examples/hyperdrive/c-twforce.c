/*BHEADER**********************************************************************
 * Copyright (c) 2013, Lawrence Livermore National Security, LLC. 
 * Produced at the Lawrence Livermore National Laboratory. Written by 
 * Jacob Schroder, Rob Falgout, Tzanio Kolev, Ulrike Yang, Veselin 
 * Dobrev, et al. LLNL-CODE-660355. All rights reserved.
 * 
 * This file is part of XBraid. Email xbraid-support@llnl.gov for support.
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License (as published by the Free Software
 * Foundation) version 2.1 dated February 1999.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the terms and conditions of the GNU General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 ***********************************************************************EHEADER*/


#include <stdio.h>
#include <math.h>
#include "advect_data.h"

void
twforce1( grid_fcn *f, double t, advection_setup *kd_ )
{
   int i;
   const double pi=M_PI;
   double x;
   int n = f->n;
   double h = f->h;
   
   if( kd_->pnr == 1 )
   {
      for (i=0; i<=n+1; i++)
      {
         x = (i-1)*h;
/* u-eqn */
         f->sol[i] = -sin(pi*x*x+kd_->ph)*sin(t) + 2.0*pi*x*cos(pi*x*x+kd_->ph)*cos(t);
      }
   }
   else if (kd_->pnr == 2)
   {
      for (i=0; i<=n+1; i++)
      {
         f->sol[i] = 0;
      }
   }
   else
   {
      printf("ERROR: twforce, unknown pnr = %i\n", kd_->pnr);
   }
}

