
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include "ViennaRNA/fold_vars.h"
#include "ViennaRNA/utils.h"
#include "ViennaRNA/constraints.h"
#include "ViennaRNA/exterior_loops.h"
#include "ViennaRNA/gquad.h"
#include "ViennaRNA/interior_loops.h"

/*
#################################
# BEGIN OF FUNCTION DEFINITIONS #
#################################
*/


PUBLIC int
vrna_E_int_loop(vrna_fold_compound_t *vc,
                int i,
                int j){

  int q, p, j_q, p_i, pq, *c_pq, max_q, max_p, tmp, *rtype, noGUclosure, no_close, energy;
  short *S_p1, *S_q1;
  char              *ptype_pq;
  unsigned char     type, type_2;
  char              *hc_pq;
  int               cp            = vc->cutpoint;
  char              *ptype        = vc->ptype;
  short             *S            = vc->sequence_encoding;
  short             S_i1          = S[i+1];
  short             S_j1          = S[j-1];
  int               *indx         = vc->jindx;
  char              *hc           = vc->hc->matrix;
  int               *hc_up        = vc->hc->up_int;
  vrna_sc_t         *sc           = vc->sc; 
  vrna_param_t      *P            = vc->params;
  int               ij            = indx[j] + i;
  int               hc_decompose  = hc[ij];
  int               e             = INF;
  int               *c            = vc->matrices->c;
  int               *ggg          = vc->matrices->ggg;
  vrna_md_t         *md           = &(P->model_details);
  int               with_gquad    = md->gquad;
  int               turn          = md->min_loop_size;
  vrna_callback_hc_evaluate *f    = vc->hc->f;
  char              eval_loop;

  /* CONSTRAINED INTERIOR LOOP start */
  if(hc_decompose & VRNA_CONSTRAINT_CONTEXT_INT_LOOP){

    type        = (unsigned char)ptype[ij];
    rtype       = &(md->rtype[0]);
    noGUclosure = md->noGUclosure;
    no_close    = (((type==3)||(type==4))&&noGUclosure);
    max_q       = i+turn+2;
    max_q       = MAX2(max_q, j - MAXLOOP - 1);
    for(q = j - 1; q >= max_q; q--){
      j_q = j - q - 1;

      if(hc_up[q+1] < j_q) break;

      pq        = indx[q] + i + 1;
      p_i       = 0;
      max_p     = i + 1;
      tmp       = i + 1 + MAXLOOP - j_q;
      max_p     = MAX2(max_p, tmp);
      tmp       = q - turn;
      max_p     = MIN2(max_p, tmp);
      tmp       = i + 1 + hc_up[i + 1];
      max_p     = MIN2(max_p, tmp);
      hc_pq     = hc + pq;
      c_pq      = c + pq;
      ptype_pq  = ptype + pq;

      S_p1      = S + i;
      S_q1      = S + q + 1;
      for(p = i+1; p <= max_p; p++){
        eval_loop = *hc_pq & VRNA_CONSTRAINT_CONTEXT_INT_LOOP_ENC;
#ifdef WITH_GEN_HC
        if(f)
          eval_loop = (f(i, j, p, q, VRNA_DECOMP_PAIR_IL, vc->hc->data)) ? eval_loop : (char)0;
#endif
        /* discard this configuration if (p,q) is not allowed to be enclosed pair of an interior loop */
        if(eval_loop){
          energy = *c_pq;
          if(energy != INF){
            type_2 = rtype[(unsigned char)*ptype_pq];

            if (noGUclosure)
              if (no_close||(type_2==3)||(type_2==4))
                if ((p>i+1)||(q<j-1)) continue;  /* continue unless stack */

            energy += ubf_eval_int_loop( i, j, p, q,
                                        p_i, j_q,
                                        S_i1, S_j1, *S_p1, *S_q1,
                                        type, type_2, rtype,
                                        ij, cp,
                                        P, sc);
            e = MIN2(e, energy);
          }

        }
        hc_pq++;    /* get hc[pq + 1] */
        c_pq++;     /* get c[pq + 1] */
        p_i++;      /* increase unpaired region [i+1...p-1] */
        ptype_pq++; /* get ptype[pq + 1] */
        S_p1++;
        pq++;
      } /* end q-loop */
    } /* end p-loop */

    if(with_gquad){
      /* include all cases where a g-quadruplex may be enclosed by base pair (i,j) */
      if ((!no_close) && ((cp < 0) || ON_SAME_STRAND(i, j, cp))) {
        energy = E_GQuad_IntLoop(i, j, type, S, ggg, indx, P);
        e = MIN2(e, energy);
      }
    }

  }
  return e;
}

PUBLIC FLT_OR_DBL
vrna_exp_E_int_loop(vrna_fold_compound_t *vc,
                int i,
                int j){

  int k,l, u1, u2, kl, *qb_kl, maxk, minl, tmp, *rtype, noGUclosure, no_close, energy;
  FLT_OR_DBL    qbt1, q_temp, *qb, *G, *scale;
  char              *ptype_pq;
  unsigned char     type, type_2;
  char              *hc_pq;
  int               cp            = vc->cutpoint;
  char              *ptype        = vc->ptype;
  short             *S1           = vc->sequence_encoding;
  short             S_i1          = S1[i+1];
  short             S_j1          = S1[j-1];
  int               *my_iindx     = vc->iindx;
  int               *jindx        = vc->jindx;
  char              *hc           = vc->hc->matrix;
  int               *hc_up        = vc->hc->up_int;
  vrna_sc_t         *sc           = vc->sc; 
  vrna_exp_param_t  *pf_params    = vc->exp_params;
  int               ij            = jindx[j] + i;
  vrna_md_t         *md           = &(pf_params->model_details);
  int               with_gquad    = md->gquad;
  int               turn          = md->min_loop_size;

  qb    = vc->exp_matrices->qb;
  G     = vc->exp_matrices->G;
  scale = vc->exp_matrices->scale;
  qbt1  = 0.;

  /* CONSTRAINED INTERIOR LOOP start */
  if(hc[ij] & VRNA_CONSTRAINT_CONTEXT_INT_LOOP){

    type        = (unsigned char)ptype[ij];
    rtype       = &(md->rtype[0]);
    noGUclosure = md->noGUclosure;
    no_close    = (((type==3)||(type==4))&&noGUclosure);
    maxk = i + MAXLOOP + 1;
    maxk = MIN2(maxk, j - turn - 2);
    maxk = MIN2(maxk, i + 1 + hc_up[i+1]);

    for (k = i + 1; k <= maxk; k++) {
      if(!ON_SAME_STRAND(i, k, cp)) break;
      u1    = k-i-1;

      minl  = MAX2(k + turn + 1, j - 1 - MAXLOOP + u1);
      kl    = my_iindx[k] - j + 1;

      for (u2 = 0, l=j-1; l>=minl; l--, kl++, u2++){
        if(hc_up[l+1] < u2) break;
        if(hc[jindx[l] + k] & VRNA_CONSTRAINT_CONTEXT_INT_LOOP_ENC){
          if(!ON_SAME_STRAND(l, j, cp)) break;
          type_2 = rtype[(unsigned char)ptype[jindx[l] + k]];
          q_temp = qb[kl]
                  * scale[u1+u2+2]
                  * exp_E_IntLoop(u1, u2, type, type_2, S_i1, S_j1, S1[k-1], S1[l+1], pf_params);

          if(sc){
            if(sc->exp_energy_up)
              q_temp *= sc->exp_energy_up[i+1][u1]
                        * sc->exp_energy_up[l+1][u2];

            if(sc->exp_f)
              q_temp *= sc->exp_f(i, j, k, l, VRNA_DECOMP_PAIR_IL, sc->data);

            if(sc->exp_energy_stack)
              if((i+1 == k) && (j-1 == l)){
                q_temp *=   sc->exp_energy_stack[i]
                          * sc->exp_energy_stack[k]
                          * sc->exp_energy_stack[l]
                          * sc->exp_energy_stack[j];
              }
          }

          qbt1 += q_temp;
        }
      }
    }

    if(with_gquad){
      /* include all cases where a g-quadruplex may be enclosed by base pair (i,j) */
      if ((!no_close) && ((cp < 0) || ON_SAME_STRAND(i, j, cp))) {
        qbt1 += exp_E_GQuad_IntLoop(i, j, type, S1, G, my_iindx, pf_params)
                * scale[2];
      }
    }

    if(sc && sc->exp_energy_bp){
      qbt1 *= sc->exp_energy_bp[my_iindx[i] - j];
    }


  }
  return qbt1;
}

PUBLIC int
vrna_E_ext_int_loop(vrna_fold_compound_t *vc,
                    int i,
                    int j,
                    int *ip,
                    int *iq){

  int                       ij, q, p, e, u1, u2, qmin, energy, *rtype, length, *indx, *hc_up, *c, turn;
  unsigned char             type, type_2;
  vrna_md_t                 *md;
  char                      *ptype, *hc;
  vrna_param_t              *P;
  short                     *S;
  vrna_sc_t                 *sc;
  vrna_callback_hc_evaluate *f;
  char                      eval_loop;

  length  = vc->length;
  indx    = vc->jindx;
  ptype   = vc->ptype;
  c       = vc->matrices->c;
  hc      = vc->hc->matrix;
  hc_up   = vc->hc->up_int;
  f       = vc->hc->f;
  P       = vc->params;
  md      = &(P->model_details);
  turn    = md->min_loop_size;
  S       = vc->sequence_encoding;
  sc      = vc->sc;

  ij      = indx[j] + i;
  rtype   = &(md->rtype[0]);
  type    = rtype[(unsigned char)ptype[ij]];
  e       = INF;

  /* CONSTRAINED INTERIOR LOOP start */
  if(hc[ij] & VRNA_CONSTRAINT_CONTEXT_INT_LOOP){

    for (p = j+1; p < length ; p++) {
      u1 = p-j-1;
      if (u1+i-1>MAXLOOP) break;
      if (hc_up[j+1] < u1) break;

      qmin = u1+i-1+length-MAXLOOP;
      if(qmin < p + turn + 1)
        qmin = p+turn+1;
      for (q = length; q >= qmin; q--) {
        u2 = i-1 + length-q;
        if(hc_up[q+1] < u2) break;

        eval_loop = hc[indx[q] + p] & VRNA_CONSTRAINT_CONTEXT_INT_LOOP;

#ifdef WITH_GEN_HC
        if(f)
          eval_loop = (f(i, j, p, q, VRNA_DECOMP_PAIR_IL, vc->hc->data)) ? eval_loop : (char)0;
#endif

        if(eval_loop){
          type_2 = rtype[(unsigned char)ptype[indx[q]+p]];
          if (u1+u2>MAXLOOP) continue;

          energy = ubf_eval_ext_int_loop( i, j, p, q,
                                          u1, u2,
                                          S[j+1], S[i-1], S[p-1], S[q+1],
                                          type, type_2,
                                          length,
                                          P, sc);
          energy += c[indx[q]+p];

          if (energy < e){
            e = energy;
            if((ip != NULL) && (iq != NULL)){
              *ip = p;
              *iq = q;
            }
          }
        }
      }
    }
  }

  return e;
}


PUBLIC int
vrna_E_stack( vrna_fold_compound_t *vc,
              int i,
              int j){

  int e, ij, pq, p, q;
  unsigned char type, type_2;

  int               cp                = vc->cutpoint;
  short             *S                = vc->sequence_encoding;
  char              *ptype            = vc->ptype;
  vrna_param_t      *P                = vc->params;
  vrna_md_t         *md               = &(P->model_details);
  int               *rtype            = &(md->rtype[0]);
  int               *indx             = vc->jindx;
  char              *hard_constraints = vc->hc->matrix;
  vrna_sc_t         *sc               = vc->sc;
  vrna_callback_hc_evaluate *f;
  char              eval_loop;

  e         = INF;
  p         = i + 1;
  q         = j - 1;
  ij        = indx[j] + i;
  pq        = indx[q] + p;
  type      = (unsigned char)ptype[ij];
  type_2    = rtype[(unsigned char)ptype[pq]];
  f         = vc->hc->f;
  eval_loop = (hard_constraints[pq] & VRNA_CONSTRAINT_CONTEXT_INT_LOOP_ENC) && (hard_constraints[ij] & VRNA_CONSTRAINT_CONTEXT_INT_LOOP);

#ifdef WITH_GEN_HC
  if(f)
    eval_loop = (f(i, j, i+1, j-1, VRNA_DECOMP_PAIR_IL, vc->hc->data)) ? eval_loop : (char)0;
#endif

  if(eval_loop){
    if ((cp < 0) || (ON_SAME_STRAND(i, p, cp) && ON_SAME_STRAND(q, j, cp))){ /* regular stack */
      e = P->stack[type][type_2];
    } else {  /* stack like cofold structure */
      short si, sj;
      si  = ON_SAME_STRAND(i, i + 1, cp) ? S[i+1] : -1;
      sj  = ON_SAME_STRAND(j - 1, j, cp) ? S[j-1] : -1;
      e   = E_IntLoop_Co(rtype[type], rtype[type_2],
                                    i, j, p, q,
                                    cp,
                                    si, sj,
                                    S[p-1], S[q+1],
                                    md->dangles,
                                    P);
    }

    /* add soft constraints */
    if(sc){
      if(sc->energy_bp)
        e += sc->energy_bp[ij];

      if(sc->energy_stack)
        if((p==i+1) && (q == j-1))
          e +=  sc->energy_stack[i]
                + sc->energy_stack[p]
                + sc->energy_stack[q]
                + sc->energy_stack[j];

      if(sc->f)
        e += sc->f(i, j, p, q, VRNA_DECOMP_PAIR_IL, sc->data);
    }
  }

  return e;
}



PUBLIC int
vrna_BT_stack(vrna_fold_compound_t *vc,
              int *i,
              int *j,
              int *en,
              vrna_bp_stack_t *bp_stack,
              int *stack_count){

  int           ij, *idx, *my_c, *rtype;
  char          *ptype;
  unsigned char type, type_2;

  vrna_param_t  *P;
  vrna_md_t     *md;
  vrna_hc_t     *hc;
  vrna_sc_t     *sc;
  vrna_callback_hc_evaluate *f;
  char          eval_loop;

  idx         = vc->jindx;
  P           = vc->params;
  md          = &(P->model_details);
  hc          = vc->hc;
  f           = vc->hc->f;
  sc          = vc->sc;
  my_c        = vc->matrices->c;
  ij          = idx[*j] + *i;
  ptype       = vc->ptype;
  type        = (unsigned char)ptype[ij];
  rtype       = &(md->rtype[0]);

  if(my_c[ij] == *en){ /*  always true, if (i.j) closes canonical structure,
                          thus (i+1.j-1) must be a pair
                      */
    eval_loop =     (hc->matrix[ij] & VRNA_CONSTRAINT_CONTEXT_INT_LOOP)
                &&  (hc->matrix[idx[*j - 1] + *i + 1] & VRNA_CONSTRAINT_CONTEXT_INT_LOOP_ENC);

    if(f)
      eval_loop = (f(*i, *j, *i+1, *j-1, VRNA_DECOMP_PAIR_IL, hc->data)) ? eval_loop : (char)0;

    if(eval_loop){
      type_2 = ptype[idx[*j - 1] + *i + 1];
      type_2 = rtype[type_2];
      *en -= P->stack[type][type_2];
      if(sc){
        if(sc->energy_bp)
          *en -= sc->energy_bp[ij];
        if(sc->energy_stack)
          *en -=    sc->energy_stack[*i]
                  + sc->energy_stack[*i + 1]
                  + sc->energy_stack[*j - 1]
                  + sc->energy_stack[*j];
        if(sc->f)
          *en -= sc->f(*i, *j, *i + 1, *j - 1, VRNA_DECOMP_PAIR_IL, sc->data);
      }
      bp_stack[++(*stack_count)].i = *i + 1;
      bp_stack[(*stack_count)].j   = *j - 1;
      (*i)++;
      (*j)--;
      return 1;
    }
  }

  return 0;
}

PUBLIC int
vrna_BT_int_loop( vrna_fold_compound_t *vc,
                  int *i,
                  int *j,
                  int en,
                  vrna_bp_stack_t *bp_stack,
                  int *stack_count){

  int           cp, ij, p, q, minq, turn, *idx, noGUclosure, no_close, energy, new, *my_c, *rtype;
  unsigned char type, type_2;
  char          *ptype;
  short         *S1, *S;

  vrna_param_t  *P;
  vrna_md_t     *md;
  vrna_hc_t     *hc;
  vrna_sc_t     *sc;
  vrna_callback_hc_evaluate *f;
  char          eval_loop;

  cp          = vc->cutpoint;
  idx         = vc->jindx;
  P           = vc->params;
  md          = &(P->model_details);
  hc          = vc->hc;
  f           = vc->hc->f;
  sc          = vc->sc;
  my_c        = vc->matrices->c;
  turn        = md->min_loop_size;
  ij          = idx[*j] + *i;
  ptype       = vc->ptype;
  type        = (unsigned char)ptype[ij];
  rtype       = &(md->rtype[0]);
  S1          = vc->sequence_encoding;
  S           = vc->sequence_encoding2;
  noGUclosure = md->noGUclosure;
  no_close    = (((type==3)||(type==4))&&noGUclosure);

  if(hc->matrix[ij] & VRNA_CONSTRAINT_CONTEXT_INT_LOOP)
    for (p = *i+1; p <= MIN2(*j-2-turn,*i+MAXLOOP+1); p++) {
      minq = *j-*i+p-MAXLOOP-2;
      if (minq<p+1+turn)
        minq = p+1+turn;

      if(hc->up_int[*i+1] < (p - *i - 1))
        break;

      for (q = *j-1; q >= minq; q--) {
        if(hc->up_int[q+1] < (*j - q - 1))
          break;

        type_2 = (unsigned char)ptype[idx[q]+p];

        eval_loop = hc->matrix[idx[q]+p] & VRNA_CONSTRAINT_CONTEXT_INT_LOOP_ENC;

        if(f)
          eval_loop = (f(*i, *j, p, q, VRNA_DECOMP_PAIR_IL, hc->data)) ? eval_loop : (char)0;

        if(!eval_loop)
          continue;

        type_2 = rtype[type_2];

        if(noGUclosure)
          if(no_close||(type_2==3)||(type_2==4))
            if((p>*i+1)||(q<*j-1))
              continue;  /* continue unless stack */

        energy = ubf_eval_int_loop( *i, *j, p, q,
                                    p-*i-1, *j-q-1,
                                    S1[*i+1], S1[*j-1], S1[p-1], S1[q+1],
                                    type, type_2,
                                    rtype,
                                    ij,
                                    -1,
                                    P,
                                    sc);
        new = energy + my_c[idx[q]+p];

        if(new == en){
          bp_stack[++(*stack_count)].i = p;
          bp_stack[(*stack_count)].j   = q;
          if(sc)
            if(sc->bt){
              vrna_basepair_t *ptr, *aux_bps;
              aux_bps = sc->bt(*i, *j, p, q, VRNA_DECOMP_PAIR_IL, sc->data);
              for(ptr = aux_bps; ptr && ptr->i != 0; ptr++){
                bp_stack[++(*stack_count)].i = ptr->i;
                bp_stack[(*stack_count)].j   = ptr->j;
              }
              free(aux_bps);
            }
          *i = p, *j = q;
          return 1; /* success */
        }
      }
    }

  /* is it a g-quadruplex? */
  if(md->gquad){
    /*
      The case that is handled here actually resembles something like
      an interior loop where the enclosing base pair is of regular
      kind and the enclosed pair is not a canonical one but a g-quadruplex
      that should then be decomposed further...
    */
    if(ON_SAME_STRAND(*i, *j, cp))
      if(vrna_BT_gquad_int(vc, *i, *j, en, bp_stack, stack_count)){
        *i = *j = -1; /* tell the calling block to continue backtracking with next block */
        return 1;
      }
  }
  
  return 0; /* unsuccessful */
}
