/*BHEADER**********************************************************************
 * Copyright (c) 2013,  Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * This file is part of XBraid.  See file COPYRIGHT for details.
 *
 * XBraid is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 ***********************************************************************EHEADER*/

/** \file braid.c
 * \brief Source code for user interface routines.  See braid.h for more information.
 *
 */

#include "_braid.h"
#include "util.h"

#ifndef DEBUG
#define DEBUG 0
#endif

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_Init(MPI_Comm               comm_world,
           MPI_Comm               comm,
           braid_Real             tstart,
           braid_Real             tstop,
           braid_Int              ntime,
           braid_App              app,
           braid_PtFcnPhi         phi,
           braid_PtFcnInit        init,
           braid_PtFcnClone       clone,
           braid_PtFcnFree        free,
           braid_PtFcnSum         sum,
           braid_PtFcnResidDot    residdot,
           braid_PtFcnAccess      access,
           braid_PtFcnBufSize     bufsize,
           braid_PtFcnBufPack     bufpack,
           braid_PtFcnBufUnpack   bufunpack,
           braid_Core            *core_ptr)
{
   _braid_Core           *core;
   braid_Int             *nrels;
   braid_Int              level;
   _braid_AccuracyHandle *accuracy;

   /* Braid default values */
   braid_Int              cfdefault = 2;         /* Default coarsening factor */
   braid_Int              nrdefault = 1;         /* Default number of FC sweeps on each level */
   braid_Int              fmg = 0;               /* Default fmg (0 is off) */
   braid_Int              nfmg_Vcyc = 1;         /* Default num V-cycles at each fmg level is 1 */
   braid_Int              max_iter = 100;        /* Default max_iter */
   braid_Int              max_levels = 30;       /* Default max_levels */
   braid_Int              max_coarse = 1;        /* Default max_coarse (in terms of number of C-points) */
   braid_Int              print_level = 1;       /* Default print level */
   braid_Int              access_level = 1;      /* Default access level */
   braid_Real             tol = 1.0e-09;         /* Default absolute tolerance */
   braid_Real             rtol = 1.0e-09;        /* Default relative tolerance */

   core = _braid_CTAlloc(_braid_Core, 1);

   _braid_CoreElt(core, comm_world)    = comm_world;
   _braid_CoreElt(core, comm)          = comm;
   _braid_CoreElt(core, tstart)        = tstart;
   _braid_CoreElt(core, tstop)         = tstop;
   _braid_CoreElt(core, ntime)         = ntime;
   _braid_CoreElt(core, app)           = app;

   _braid_CoreElt(core, phi)           = phi;
   _braid_CoreElt(core, init)          = init;
   _braid_CoreElt(core, clone)         = clone;
   _braid_CoreElt(core, free)          = free;
   _braid_CoreElt(core, sum)           = sum;
   _braid_CoreElt(core, residdot)      = residdot;
   _braid_CoreElt(core, access)        = access;
   _braid_CoreElt(core, bufsize)       = bufsize;
   _braid_CoreElt(core, bufpack)       = bufpack;
   _braid_CoreElt(core, bufunpack)     = bufunpack;
   _braid_CoreElt(core, coarsen)       = NULL;
   _braid_CoreElt(core, refine)        = NULL;

   _braid_CoreElt(core, access_level)  = access_level;
   _braid_CoreElt(core, print_level)   = print_level;
   _braid_CoreElt(core, max_levels)    = max_levels;
   _braid_CoreElt(core, max_coarse)    = max_coarse;
   _braid_CoreElt(core, tol)           = tol;
   _braid_CoreElt(core, rtol)          = rtol;

   nrels = _braid_TAlloc(braid_Int, max_levels);
   for (level = 0; level < max_levels; level++)
   {
      nrels[level] = -1;
   }
   _braid_CoreElt(core, nrels)      = nrels;
   _braid_CoreElt(core, nrdefault)  = nrdefault;

   _braid_CoreElt(core, cfactors)   = _braid_CTAlloc(braid_Int, max_levels);
   _braid_CoreElt(core, cfdefault)  = cfdefault;

   _braid_CoreElt(core, max_iter)   = max_iter;
   _braid_CoreElt(core, niter)      = 0;
   _braid_CoreElt(core, rnorm)      = 0.0;
   _braid_CoreElt(core, fmg)        = fmg;
   _braid_CoreElt(core, nfmg_Vcyc)  = nfmg_Vcyc;

   /* Accuracy for spatial solves for using implicit schemes
    *  - accuracy[0] refers to accuracy on level 0
    *  - accuracy[1] refers to accuracy on all levels > 0 */
   accuracy                         = _braid_TAlloc(_braid_AccuracyHandle, 2);
   accuracy[0].matchF               = 0;
   accuracy[0].value                = 1.0e-02;
   accuracy[0].old_value            = 1.0e-02;
   accuracy[0].loose                = 1.0e-02;
   accuracy[0].tight                = 1.0e-02;
   accuracy[0].tight_used           = 0;

   accuracy[1].matchF               = 0;
   accuracy[1].value                = 1.0e-02;
   accuracy[1].old_value            = 1.0e-02;
   accuracy[1].loose                = 1.0e-02;
   accuracy[1].tight                = 1.0e-02;
   accuracy[1].tight_used           = 0;
   _braid_CoreElt(core, accuracy)   = accuracy;

   _braid_CoreElt(core, gupper)     = ntime;

   _braid_CoreElt(core, rfactors)   = NULL;

   _braid_CoreElt(core, nlevels)    = 0;
   _braid_CoreElt(core, grids)      = _braid_CTAlloc(_braid_Grid *, max_levels);

   *core_ptr = core;

   return _braid_error_flag;
}


/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_Drive(braid_Core  core)
{
   braid_Real     tstart      = _braid_CoreElt(core, tstart);
   braid_Real     tstop       = _braid_CoreElt(core, tstop);
   braid_Int      ntime       = _braid_CoreElt(core, ntime);
   braid_Real     tol         = _braid_CoreElt(core, tol);
   braid_Int      rtol        = _braid_CoreElt(core, rtol);
   braid_Int      fmg         = _braid_CoreElt(core, fmg);
   braid_Int      max_iter    = _braid_CoreElt(core, max_iter);
   braid_Int      print_level = _braid_CoreElt(core, print_level);
   braid_Int      access_level= _braid_CoreElt(core, access_level);
   braid_Int      nfmg_Vcyc   = _braid_CoreElt(core, nfmg_Vcyc); 

   braid_Int      nlevels, iter;
   braid_Real     rnorm, old_rnorm;
   braid_Real     accuracy;
   braid_Int      ilower, iupper;
   braid_Real    *ta;
   braid_Int      level, fmglevel, fmg_Vcyc, down, done, i, refined;
   _braid_Grid   *grid;
   braid_Real     localtime, globaltime;

   MPI_Comm    comm_world = _braid_CoreElt(core, comm_world);
   braid_Int      myid;

   /* Start timer */
   localtime = MPI_Wtime();

   MPI_Comm_rank(comm_world, &myid);

   level = 0;
   rnorm = -1.0;

   /* Create fine grid */
   _braid_GetDistribution(core, &ilower, &iupper);
   _braid_GridInit(core, 0, ilower, iupper, &grid);

   /* Set t values */
   ta = _braid_GridElt(grid, ta);
   for (i = ilower; i <= iupper; i++)
   {
      ta[i-ilower] = tstart + (((braid_Real)i)/ntime)*(tstop-tstart);
   }

   /* Create a grid hierarchy */
   _braid_InitHierarchy(core, grid);
   nlevels = _braid_CoreElt(core, nlevels);

   /* Set initial values at C-points */
   _braid_InitGuess(core, 0);

   /* Set cycling variables */
   fmglevel = 0;
   fmg_Vcyc = 0;
   if (fmg)
   {
      fmglevel = nlevels-1;
   }
   down = 1;
   done = 0;
   if ((nlevels <= 1) || (tol <= 0.0))
   {
      /* Just do sequential time marching */
      done = 1;
   }

   iter = 0;
   while (!done)
   {
      /* Down cycle */

      if (down)
      {
         if (level < (nlevels-1))
         {
            /* CF-relaxation */
            _braid_CFRelax(core, level);

            /* F-relax then restrict */
            if( level == 0)
            {
               old_rnorm = rnorm;
            }
            _braid_FRestrict(core, level, iter, &rnorm);
            /* Set initial guess on next coarser level */
            _braid_InitGuess(core, level+1);

            /* Adjust tolerance */
            if ((level == 0) && (iter == 0))
            {
               if (rtol){
                  tol *= rnorm;
               }
            }

            if (level == 0)
            {
               /* Adjust accuracy of spatial solves for level 0 */
               _braid_SetAccuracy(rnorm, _braid_CoreElt(core, accuracy[0].loose), 
                               _braid_CoreElt(core, accuracy[0].tight),
                               _braid_CoreElt(core, accuracy[0].value), tol, &accuracy);
               _braid_CoreElt(core, accuracy[0].old_value) = _braid_CoreElt(core, accuracy[0].value);
               _braid_CoreElt(core, accuracy[0].value)     = accuracy;
               _braid_CoreElt(core, accuracy[0].matchF)    = 1;
               
               /* Debug printing only if the accuracy value has changed */
               if( (print_level >= 2) && (myid == 0) && 
                   ( _braid_CoreElt(core, accuracy[0].old_value) != _braid_CoreElt(core, accuracy[0].value)) )
               {
                  _braid_printf("  Braid:  Accuracy changed to %.2e \n", accuracy);
               }
            }

            level++;
         }
         else
         {
            /* Coarsest grid - solve on the up-cycle */
            down = 0;
         }
      }

      /* Up cycle */

      if (!down)
      {
         if (level > 0)
         {
            if (level >= fmglevel)
            {
               /* F-relax then interpolate */
               _braid_FInterp(core, level, iter+1, rnorm);
               
               level--;
            }
            else
            {  
               // Do nfmg_Vcyc number of V-cycles at each level in FMG
               fmg_Vcyc += 1;
               if ( fmg_Vcyc == nfmg_Vcyc )
               {
                  fmg_Vcyc = 0;
                  fmglevel--;
               }
               
               down = 1;
            }
         }
         else
         {
            /* Finest grid - refine grid if desired, else check convergence */
            _braid_FRefine(core, &refined);

            if (refined)
            {
               nlevels = _braid_CoreElt(core, nlevels);
            }
            else
            {
               /* Note that this residual is based on an earlier iterate */
               if( (print_level >= 1) && (myid == 0) )
               {
                  if (iter == 0)
                     _braid_printf("  Braid:  || r_%d || = %e\n", iter, rnorm);
                  else
                     _braid_printf("  Braid:  || r_%d || = %e,  conv. factor = %e\n", iter, rnorm,
                            rnorm/old_rnorm);
               }

               if ( ((rnorm < tol) && (_braid_CoreElt(core, accuracy[0].tight_used) == 1)) || 
                    (rnorm == 0.0) ||
                    (iter == max_iter-1) )
               {
                  done = 1;
               }
            }

            iter++;

            if (fmg)
            {
               fmglevel = nlevels-1;
            }
            down = 1;
         }
      }

   }

   /* All final access to Braid by carrying out an F-relax to generate all points */

   if( access_level >= 1)
   {
      _braid_FAccess(core, rnorm, iter, 0, 1);
   }

   _braid_CoreElt(core, niter) = iter;
   _braid_CoreElt(core, rnorm) = rnorm;

   /* Stop timer */
   localtime = MPI_Wtime() - localtime;
   MPI_Allreduce(&localtime, &globaltime, 1, MPI_DOUBLE, MPI_MAX, comm_world);
   _braid_CoreElt(core, localtime)  = localtime;
   _braid_CoreElt(core, globaltime) = globaltime;

   /* Print statistics for this run */
   if( (print_level >= 1) && (myid == 0) )
   {
      braid_PrintStats(core);
   }

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_Destroy(braid_Core  core)
{
   if (core)
   {
      braid_Int      nlevels    = _braid_CoreElt(core, nlevels);
      _braid_Grid  **grids      = _braid_CoreElt(core, grids);
      braid_Int      level;

      _braid_TFree(_braid_CoreElt(core, nrels));
      _braid_TFree(_braid_CoreElt(core, cfactors));
      _braid_TFree(_braid_CoreElt(core, accuracy));
      _braid_TFree(_braid_CoreElt(core, rfactors));
      for (level = 0; level < nlevels; level++)
      {
         _braid_GridDestroy(core, grids[level]);
      }
      _braid_TFree(grids);

      _braid_TFree(core);
   }

   if (_braid_printfile != NULL)
   {
      fclose(_braid_printfile);
   }

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_PrintStats(braid_Core  core)
{
   MPI_Comm      comm_world = _braid_CoreElt(core, comm_world);
   braid_Real    tstart     = _braid_CoreElt(core, tstart);
   braid_Real    tstop      = _braid_CoreElt(core, tstop);
   braid_Int     ntime      = _braid_CoreElt(core, ntime);
   braid_Int     max_levels = _braid_CoreElt(core, max_levels);
   braid_Int     max_coarse = _braid_CoreElt(core, max_coarse);
   braid_Real    tol        = _braid_CoreElt(core, tol);
   braid_Int     rtol       = _braid_CoreElt(core, rtol);
   braid_Int    *nrels      = _braid_CoreElt(core, nrels);
   /*braid_Int    *cfactors   = _braid_CoreElt(core, cfactors);*/
   braid_Int     max_iter   = _braid_CoreElt(core, max_iter);
   braid_Int     niter      = _braid_CoreElt(core, niter);
   braid_Real    rnorm      = _braid_CoreElt(core, rnorm);
   braid_Int     nlevels    = _braid_CoreElt(core, nlevels);
   _braid_Grid **grids      = _braid_CoreElt(core, grids);

   braid_Real    globaltime = _braid_CoreElt(core, globaltime);

   braid_Int     myid, level;

   MPI_Comm_rank(comm_world, &myid);
   if ( myid == 0 )
   {
      _braid_printf("\n");
      _braid_printf("  start time = %e\n", tstart);
      _braid_printf("  stop time  = %e\n", tstop);
      _braid_printf("  time steps = %d\n", ntime);
      _braid_printf("\n");
      _braid_printf("  max number of levels = %d\n", max_levels);
      _braid_printf("  max coarse           = %d\n", max_coarse);
      _braid_printf("  number of levels     = %d\n", nlevels);
      _braid_printf("  stopping tolerance   = %e\n", tol);
      _braid_printf("  relative tolerance?  = %d\n", rtol);
      _braid_printf("  max iterations       = %d\n", max_iter);
      _braid_printf("  iterations           = %d\n", niter);
      _braid_printf("  residual norm        = %e\n", rnorm);
      _braid_printf("\n");
      _braid_printf("  level   cfactor   nrelax\n", globaltime);
      for (level = 0; level < nlevels-1; level++)
      {
         _braid_printf("  % 5d   % 7d   % 6d\n",
                      level, _braid_GridElt(grids[level], cfactor), nrels[level]);
      }
      /* Print out blank information on coarsest grid */
      _braid_printf("  % 5d   % 7c   % 6c\n", level, ' ', ' ');
      _braid_printf("\n");
      _braid_printf("  wall time = %f\n", globaltime);
      _braid_printf("\n");
   }

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetLoosexTol(braid_Core  core,
                   braid_Int   level,
                   braid_Real  loose_tol)
{
   if (level < 0)
   {
      /* Set the loose tolerance on all levels. 
       * Index 0 corresponds to level 0, index 1 to all levels > 0. */
      _braid_CoreElt(core, accuracy[0].loose)     = loose_tol;
      _braid_CoreElt(core, accuracy[1].loose)     = loose_tol;

      /* Initialize the current and old value with loose_tol. */
      _braid_CoreElt(core, accuracy[0].value)     = loose_tol;
      _braid_CoreElt(core, accuracy[0].old_value) = loose_tol;
      
      _braid_CoreElt(core, accuracy[1].value)     = loose_tol;
      _braid_CoreElt(core, accuracy[1].old_value) = loose_tol;
   }
   else
   {  
      /* Set the loose tolerance on level level and initialize
       * the current and old value for that level with loose_tol. */
      _braid_CoreElt(core, accuracy[level].loose)     = loose_tol;
      _braid_CoreElt(core, accuracy[level].value)     = loose_tol;
      _braid_CoreElt(core, accuracy[level].old_value) = loose_tol;
   }

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetTightxTol(braid_Core  core,
                   braid_Int   level,
                   braid_Real  tight_tol)
{
   if (level < 0)
   {
      /* Set tight tolerance on all levels. */
      _braid_CoreElt(core, accuracy[0].tight)     = tight_tol;
      _braid_CoreElt(core, accuracy[1].tight)     = tight_tol;
   }
   else
   {  
      /* Set tight tolerance on level level. */
      _braid_CoreElt(core, accuracy[level].tight) = tight_tol;
   }

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetMaxLevels(braid_Core  core,
                   braid_Int   max_levels)
{
   _braid_CoreElt(core, max_levels) = max_levels;

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetMaxCoarse(braid_Core  core,
                braid_Int   max_coarse)
{
   _braid_CoreElt(core, max_coarse) = max_coarse;

   return _braid_error_flag;
}


/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetPrintLevel(braid_Core  core,
                    braid_Int   print_level)
{
   _braid_CoreElt(core, print_level) = print_level;

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetPrintFile(braid_Core     core,
                   const char    *printfile_name)
{
   MPI_Comm   comm_world = _braid_CoreElt(core, comm_world);
   braid_Int     myid;
   
   MPI_Comm_rank(comm_world, &myid);
   
   if(myid == 0)
   {
      if ((_braid_printfile = fopen(printfile_name, "w")) == NULL)
      {
         printf("Error: can't open output file %s\n", printfile_name);
         exit(1);
      }
   }

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetAccessLevel(braid_Core  core,
                     braid_Int   access_level)
{
   _braid_CoreElt(core, access_level) = access_level;

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SplitCommworld(const MPI_Comm  *comm_world,
                     braid_Int       px,
                     MPI_Comm        *comm_x,
                     MPI_Comm        *comm_t)
{
   braid_Int myid, xcolor, tcolor;

   /* Create communicators for the time and space dimensions */
   /* The communicators are based on colors and keys (= myid) */
   MPI_Comm_rank( *comm_world, &myid );
   xcolor = myid / px;
   tcolor = myid % px;

   MPI_Comm_split( *comm_world, xcolor, myid, comm_x );
   MPI_Comm_split( *comm_world, tcolor, myid, comm_t );

   return _braid_error_flag;
}


/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetAbsTol(braid_Core  core,
                braid_Real  tol)
{
   _braid_CoreElt(core, tol) = tol;

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetRelTol(braid_Core  core,
                braid_Real  tol)
{
   _braid_CoreElt(core, tol)  = tol;
   _braid_CoreElt(core, rtol) = 1;

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetNRelax(braid_Core  core,
                braid_Int   level,
                braid_Int   nrelax)
{
   braid_Int  *nrels = _braid_CoreElt(core, nrels);

   if (level < 0)
   {
      /* Set default value */
      _braid_CoreElt(core, nrdefault) = nrelax;
   }
   else
   {
      /* Set factor on specified level */
      nrels[level] = nrelax;
   }

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetCFactor(braid_Core  core,
                 braid_Int   level,
                 braid_Int   cfactor)
{
   braid_Int  *cfactors = _braid_CoreElt(core, cfactors);

   if (level < 0)
   {
      /* Set default value */
      _braid_CoreElt(core, cfdefault) = cfactor;
   }
   else
   {
      /* Set factor on specified level */
      cfactors[level] = cfactor;
   }

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetMaxIter(braid_Core  core,
                 braid_Int   max_iter)
{
   _braid_CoreElt(core, max_iter) = max_iter;

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetFMG(braid_Core  core)
{
   _braid_CoreElt(core, fmg) = 1;

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetNFMGVcyc(braid_Core  core,
                  braid_Int   nfmg_Vcyc)
{
   _braid_CoreElt(core, nfmg_Vcyc) = nfmg_Vcyc;

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetSpatialCoarsen(braid_Core  core, 
                        braid_PtFcnCoarsen coarsen)
{
   _braid_CoreElt(core, coarsen) = coarsen;

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_SetSpatialRefine(braid_Core  core,
                    braid_PtFcnRefine refine)
{
   _braid_CoreElt(core, refine) = refine;

   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_GetStatusResidual(braid_Status  status,
                        braid_Real   *rnorm_ptr)
{
   *rnorm_ptr = _braid_StatusElt(status, rnorm);
   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_GetStatusIter(braid_Status  status,
                    braid_Int    *iter_ptr)
{
   *iter_ptr = _braid_StatusElt(status, iter);
   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_GetStatusLevel(braid_Status  status,
                     braid_Int    *level_ptr)
{
   *level_ptr = _braid_StatusElt(status, level);
   return _braid_error_flag;
}


/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_GetStatusDone(braid_Status  status,
                    braid_Int    *done_ptr)
{
   *done_ptr = _braid_StatusElt(status, done);
   return _braid_error_flag;
}

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_GetNumIter(braid_Core  core,
              braid_Int   *niter_ptr)
{
   *niter_ptr =  _braid_CoreElt(core, niter);
   return _braid_error_flag;
} 

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

braid_Int
braid_GetRNorm(braid_Core  core,
               braid_Real  *rnorm_ptr)

{
   *rnorm_ptr = _braid_CoreElt(core, rnorm);
   return _braid_error_flag;
} 


