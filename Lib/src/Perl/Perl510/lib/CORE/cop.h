/*    cop.h
 *
 *    Copyright (C) 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
 *    2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009 by Larry Wall and others
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 * Control ops (cops) are one of the two ops OP_NEXTSTATE and OP_DBSTATE,
 * that (loosely speaking) are separate statements.
 * They hold information important for lexical state and error reporting.
 * At run time, PL_curcop is set to point to the most recently executed cop,
 * and thus can be used to determine our current state.
 */

/* A jmpenv packages the state required to perform a proper non-local jump.
 * Note that there is a start_env initialized when perl starts, and top_env
 * points to this initially, so top_env should always be non-null.
 *
 * Existence of a non-null top_env->je_prev implies it is valid to call
 * longjmp() at that runlevel (we make sure start_env.je_prev is always
 * null to ensure this).
 *
 * je_mustcatch, when set at any runlevel to TRUE, means eval ops must
 * establish a local jmpenv to handle exception traps.  Care must be taken
 * to restore the previous value of je_mustcatch before exiting the
 * stack frame iff JMPENV_PUSH was not called in that stack frame.
 * GSAR 97-03-27
 */

struct jmpenv {
	struct jmpenv *	je_prev;
	Sigjmp_buf		je_buf;		/* only for use if !je_throw */
	int			je_ret;		/* last exception thrown */
	bool		je_mustcatch;	/* need to call longjmp()? */
};

typedef struct jmpenv JMPENV;

#ifdef OP_IN_REGISTER
#define OP_REG_TO_MEM	PL_opsave = op
#define OP_MEM_TO_REG	op = PL_opsave
#else
#define OP_REG_TO_MEM	NOOP
#define OP_MEM_TO_REG	NOOP
#endif

/*
 * How to build the first jmpenv.
 *
 * top_env needs to be non-zero. It points to an area
 * in which longjmp() stuff is stored, as C callstack
 * info there at least is thread specific this has to
 * be per-thread. Otherwise a 'die' in a thread gives
 * that thread the C stack of last thread to do an eval {}!
 */

#define JMPENV_BOOTSTRAP \
	STMT_START {				\
	Zero(&PL_start_env, 1, JMPENV);		\
	PL_start_env.je_ret = -1;		\
	PL_start_env.je_mustcatch = TRUE;	\
	PL_top_env = &PL_start_env;		\
	} STMT_END

/*
 *   PERL_FLEXIBLE_EXCEPTIONS
 *
 * All the flexible exceptions code has been removed.
 * See the following threads for details:
 *
 *   http://www.xray.mpe.mpg.de/mailing-lists/perl5-porters/2004-07/msg00378.html
 *
 * Joshua's original patches (which weren't applied) and discussion:
 *
 *   http://www.xray.mpe.mpg.de/mailing-lists/perl5-porters/1998-02/msg01396.html
 *   http://www.xray.mpe.mpg.de/mailing-lists/perl5-porters/1998-02/msg01489.html
 *   http://www.xray.mpe.mpg.de/mailing-lists/perl5-porters/1998-02/msg01491.html
 *   http://www.xray.mpe.mpg.de/mailing-lists/perl5-porters/1998-02/msg01608.html
 *   http://www.xray.mpe.mpg.de/mailing-lists/perl5-porters/1998-02/msg02144.html
 *   http://www.xray.mpe.mpg.de/mailing-lists/perl5-porters/1998-02/msg02998.html
 *
 * Chip's reworked patch and discussion:
 *
 *   http://www.xray.mpe.mpg.de/mailing-lists/perl5-porters/1999-03/msg00520.html
 *
 * The flaw in these patches (which went unnoticed at the time) was
 * that they moved some code that could potentially die() out of the
 * region protected by the setjmp()s.  This caused exceptions within
 * END blocks and such to not be handled by the correct setjmp().
 *
 * The original patches that introduces flexible exceptions were:
 *
 *   http://public.activestate.com/cgi-bin/perlbrowse?patch=3386
 *   http://public.activestate.com/cgi-bin/perlbrowse?patch=5162
 */

#define dJMPENV		JMPENV cur_env

#define JMPENV_PUSH(v) \
	STMT_START {							\
	DEBUG_l(Perl_deb(aTHX_ "Setting up jumplevel %p, was %p at %s:%d\n",	\
				 (void*)&cur_env, (void*)PL_top_env,			\
				 __FILE__, __LINE__));					\
	cur_env.je_prev = PL_top_env;					\
	OP_REG_TO_MEM;							\
	cur_env.je_ret = PerlProc_setjmp(cur_env.je_buf, SCOPE_SAVES_SIGNAL_MASK);		\
	OP_MEM_TO_REG;							\
	PL_top_env = &cur_env;						\
	cur_env.je_mustcatch = FALSE;					\
	(v) = cur_env.je_ret;						\
	} STMT_END

#define JMPENV_POP \
	STMT_START {							\
	DEBUG_l(Perl_deb(aTHX_ "popping jumplevel was %p, now %p at %s:%d\n",	\
				 (void*)PL_top_env, (void*)cur_env.je_prev,		\
				 __FILE__, __LINE__));					\
	assert(PL_top_env == &cur_env);					\
	PL_top_env = cur_env.je_prev;					\
	} STMT_END

#define JMPENV_JUMP(v) \
	STMT_START {						\
	OP_REG_TO_MEM;						\
	if (PL_top_env->je_prev)				\
		PerlProc_longjmp(PL_top_env->je_buf, (v));		\
	if ((v) == 2)						\
		PerlProc_exit(STATUS_EXIT);		                \
	PerlIO_printf(PerlIO_stderr(), "panic: top_env\n");	\
	PerlProc_exit(1);					\
	} STMT_END

#define CATCH_GET		(PL_top_env->je_mustcatch)
#define CATCH_SET(v)		(PL_top_env->je_mustcatch = (v))


#include "mydtrace.h"

struct cop {
	BASEOP
	/* On LP64 putting this here takes advantage of the fact that BASEOP isn't
	   an exact multiple of 8 bytes to save structure padding.  */
	line_t      cop_line;       /* line # of this command */
	/* label for this construct is now stored in cop_hints_hash */
#ifdef USE_ITHREADS
	char *	cop_stashpv;	/* package line was compiled in */
	char *	cop_file;	/* file name the following line # is from */
#else
	HV *	cop_stash;	/* package line was compiled in */
	GV *	cop_filegv;	/* file the following line # is from */
#endif
	U32		cop_hints;	/* hints bits from pragmata */
	U32		cop_seq;	/* parse sequence number */
	/* Beware. mg.c and warnings.pl assume the type of this is STRLEN *:  */
	STRLEN *	cop_warnings;	/* lexical warnings bitmask */
	/* compile time state of %^H.  See the comment in op.c for how this is
	   used to recreate a hash to return from caller.  */
	struct refcounted_he * cop_hints_hash;
};

#ifdef USE_ITHREADS
#  define CopFILE(c)		((c)->cop_file)
#  define CopFILEGV(c)		(CopFILE(c) \
				 ? gv_fetchfile(CopFILE(c)) : NULL)

#  ifdef NETWARE
#    define CopFILE_set(c,pv)	((c)->cop_file = savepv(pv))
#    define CopFILE_setn(c,pv,l)  ((c)->cop_file = savepv((pv),(l)))
#  else
#    define CopFILE_set(c,pv)	((c)->cop_file = savesharedpv(pv))
#    define CopFILE_setn(c,pv,l)  ((c)->cop_file = savesharedpvn((pv),(l)))
#  endif

#  define CopFILESV(c)		(CopFILE(c) \
				 ? GvSV(gv_fetchfile(CopFILE(c))) : NULL)
#  define CopFILEAV(c)		(CopFILE(c) \
				 ? GvAV(gv_fetchfile(CopFILE(c))) : NULL)
#  ifdef DEBUGGING
#    define CopFILEAVx(c)	(assert(CopFILE(c)), \
				   GvAV(gv_fetchfile(CopFILE(c))))
#  else
#    define CopFILEAVx(c)	(GvAV(gv_fetchfile(CopFILE(c))))
#  endif
#  define CopSTASHPV(c)		((c)->cop_stashpv)

#  ifdef NETWARE
#    define CopSTASHPV_set(c,pv)	((c)->cop_stashpv = ((pv) ? savepv(pv) : NULL))
#  else
#    define CopSTASHPV_set(c,pv)	((c)->cop_stashpv = savesharedpv(pv))
#  endif

#  define CopSTASH(c)		(CopSTASHPV(c) \
				 ? gv_stashpv(CopSTASHPV(c),GV_ADD) : NULL)
#  define CopSTASH_set(c,hv)	CopSTASHPV_set(c, (hv) ? HvNAME_get(hv) : NULL)
#  define CopSTASH_eq(c,hv)	((hv) && stashpv_hvname_match(c,hv))
#  ifdef NETWARE
#    define CopSTASH_free(c) SAVECOPSTASH_FREE(c)
#    define CopFILE_free(c) SAVECOPFILE_FREE(c)
#  else
#    define CopSTASH_free(c)	PerlMemShared_free(CopSTASHPV(c))
#    define CopFILE_free(c)	(PerlMemShared_free(CopFILE(c)),(CopFILE(c) = NULL))
#  endif
#else
#  define CopFILEGV(c)		((c)->cop_filegv)
#  define CopFILEGV_set(c,gv)	((c)->cop_filegv = (GV*)SvREFCNT_inc(gv))
#  define CopFILE_set(c,pv)	CopFILEGV_set((c), gv_fetchfile(pv))
#  define CopFILE_setn(c,pv,l)	CopFILEGV_set((c), gv_fetchfile_flags((pv),(l),0))
#  define CopFILESV(c)		(CopFILEGV(c) ? GvSV(CopFILEGV(c)) : NULL)
#  define CopFILEAV(c)		(CopFILEGV(c) ? GvAV(CopFILEGV(c)) : NULL)
#  ifdef DEBUGGING
#    define CopFILEAVx(c)	(assert(CopFILEGV(c)), GvAV(CopFILEGV(c)))
#  else
#    define CopFILEAVx(c)	(GvAV(CopFILEGV(c)))
# endif
#  define CopFILE(c)		(CopFILEGV(c) && GvSV(CopFILEGV(c)) \
					? SvPVX(GvSV(CopFILEGV(c))) : NULL)
#  define CopSTASH(c)		((c)->cop_stash)
#  define CopSTASH_set(c,hv)	((c)->cop_stash = (hv))
#  define CopSTASHPV(c)		(CopSTASH(c) ? HvNAME_get(CopSTASH(c)) : NULL)
   /* cop_stash is not refcounted */
#  define CopSTASHPV_set(c,pv)	CopSTASH_set((c), gv_stashpv(pv,GV_ADD))
#  define CopSTASH_eq(c,hv)	(CopSTASH(c) == (hv))
#  define CopSTASH_free(c)
#  define CopFILE_free(c)	(SvREFCNT_dec(CopFILEGV(c)),(CopFILEGV(c) = NULL))

#endif /* USE_ITHREADS */
#define CopLABEL(c)  Perl_fetch_cop_label(aTHX_ (c)->cop_hints_hash, NULL, NULL)
#define CopLABEL_alloc(pv)	((pv)?savepv(pv):NULL)

#define CopSTASH_ne(c,hv)	(!CopSTASH_eq(c,hv))
#define CopLINE(c)		((c)->cop_line)
#define CopLINE_inc(c)		(++CopLINE(c))
#define CopLINE_dec(c)		(--CopLINE(c))
#define CopLINE_set(c,l)	(CopLINE(c) = (l))

/* OutCopFILE() is CopFILE for output (caller, die, warn, etc.) */
#define OutCopFILE(c) CopFILE(c)

/* If $[ is non-zero, it's stored in cop_hints under the key "$[", and
   HINT_ARYBASE is set to indicate this.
   Setting it is ineficient due to the need to create 2 mortal SVs, but as
   using $[ is highly discouraged, no sane Perl code will be using it.  */
#define CopARYBASE_get(c)	\
	((CopHINTS_get(c) & HINT_ARYBASE)				\
	 ? SvIV(Perl_refcounted_he_fetch(aTHX_ (c)->cop_hints_hash, 0,	\
					 "$[", 2, 0, 0))		\
	 : 0)
#define CopARYBASE_set(c, b) STMT_START { \
	if (b || ((c)->cop_hints & HINT_ARYBASE)) {			\
		(c)->cop_hints |= HINT_ARYBASE;				\
		if ((c) == &PL_compiling) {					\
		SV *val = newSViv(b);					\
		(void)hv_stores(GvHV(PL_hintgv), "$[", val);		\
		mg_set(val);						\
		PL_hints |= HINT_ARYBASE;				\
		} else {							\
		(c)->cop_hints_hash					\
		   = Perl_refcounted_he_new(aTHX_ (c)->cop_hints_hash,	\
					newSVpvs_flags("$[", SVs_TEMP),	\
					sv_2mortal(newSViv(b)));	\
		}								\
	}								\
	} STMT_END

/* FIXME NATIVE_HINTS if this is changed from op_private (see perl.h)  */
#define CopHINTS_get(c)		((c)->cop_hints + 0)
#define CopHINTS_set(c, h)	STMT_START {				\
					(c)->cop_hints = (h);		\
				} STMT_END

/*
 * Here we have some enormously heavy (or at least ponderous) wizardry.
 */

/* subroutine context */
struct block_sub {
	OP *	retop;	/* op to execute on exit from sub */
	/* Above here is the same for sub, format and eval.  */
	CV *	cv;
	/* Above here is the same for sub and format.  */
	AV *	savearray;
	AV *	argarray;
	I32		olddepth;
	PAD		*oldcomppad;
};


/* format context */
struct block_format {
	OP *	retop;	/* op to execute on exit from sub */
	/* Above here is the same for sub, format and eval.  */
	CV *	cv;
	/* Above here is the same for sub and format.  */
	GV *	gv;
	GV *	dfoutgv;
};

/* base for the next two macros. Don't use directly.
 * Note that the refcnt of the cv is incremented twice;  The CX one is
 * decremented by LEAVESUB, the other by LEAVE. */

#define PUSHSUB_BASE(cx)						\
	ENTRY_PROBE(GvENAME(CvGV(cv)),		       			\
		CopFILE((const COP *)CvSTART(cv)),			\
		CopLINE((const COP *)CvSTART(cv)));			\
									\
	cx->blk_sub.cv = cv;						\
	cx->blk_sub.olddepth = CvDEPTH(cv);				\
	cx->cx_type |= (hasargs) ? CXp_HASARGS : 0;			\
	cx->blk_sub.retop = NULL;					\
	if (!CvDEPTH(cv)) {						\
		SvREFCNT_inc_simple_void_NN(cv);				\
		SvREFCNT_inc_simple_void_NN(cv);				\
		SAVEFREESV(cv);						\
	}


#define PUSHSUB(cx)							\
	PUSHSUB_BASE(cx)						\
	cx->blk_u16 = PL_op->op_private &				\
						  (OPpLVAL_INTRO|OPpENTERSUB_INARGS);

/* variant for use by OP_DBSTATE, where op_private holds hint bits */
#define PUSHSUB_DB(cx)							\
	PUSHSUB_BASE(cx)						\
	cx->blk_u16 = 0;


#define PUSHFORMAT(cx, retop)						\
	cx->blk_format.cv = cv;						\
	cx->blk_format.gv = gv;						\
	cx->blk_format.retop = (retop);					\
	cx->blk_format.dfoutgv = PL_defoutgv;				\
	SvREFCNT_inc_void(cx->blk_format.dfoutgv)

#define POP_SAVEARRAY()						\
	STMT_START {							\
	SvREFCNT_dec(GvAV(PL_defgv));					\
	GvAV(PL_defgv) = cx->blk_sub.savearray;				\
	} STMT_END

/* junk in @_ spells trouble when cloning CVs and in pp_caller(), so don't
 * leave any (a fast av_clear(ary), basically) */
#define CLEAR_ARGARRAY(ary) \
	STMT_START {							\
	AvMAX(ary) += AvARRAY(ary) - AvALLOC(ary);			\
	AvARRAY(ary) = AvALLOC(ary);					\
	AvFILLp(ary) = -1;						\
	} STMT_END

#define POPSUB(cx,sv)							\
	STMT_START {							\
	RETURN_PROBE(GvENAME(CvGV((const CV*)cx->blk_sub.cv)),		\
		CopFILE((COP*)CvSTART((const CV*)cx->blk_sub.cv)),	\
		CopLINE((COP*)CvSTART((const CV*)cx->blk_sub.cv)));	\
									\
	if (CxHASARGS(cx)) {						\
		POP_SAVEARRAY();						\
		/* abandon @_ if it got reified */				\
		if (AvREAL(cx->blk_sub.argarray)) {				\
		const SSize_t fill = AvFILLp(cx->blk_sub.argarray);	\
		SvREFCNT_dec(cx->blk_sub.argarray);			\
		cx->blk_sub.argarray = newAV();				\
		av_extend(cx->blk_sub.argarray, fill);			\
		AvREIFY_only(cx->blk_sub.argarray);			\
		CX_CURPAD_SV(cx->blk_sub, 0) = MUTABLE_SV(cx->blk_sub.argarray); \
		}								\
		else {							\
		CLEAR_ARGARRAY(cx->blk_sub.argarray);			\
		}								\
	}								\
	sv = MUTABLE_SV(cx->blk_sub.cv);				\
	if (sv && (CvDEPTH((const CV*)sv) = cx->blk_sub.olddepth))	\
		sv = NULL;						\
	} STMT_END

#define LEAVESUB(sv)							\
	STMT_START {							\
	if (sv)								\
		SvREFCNT_dec(sv);						\
	} STMT_END

#define POPFORMAT(cx)							\
	setdefout(cx->blk_format.dfoutgv);				\
	SvREFCNT_dec(cx->blk_format.dfoutgv);

/* eval context */
struct block_eval {
	OP *	retop;	/* op to execute on exit from eval */
	/* Above here is the same for sub, format and eval.  */
	SV *	old_namesv;
	OP *	old_eval_root;
	SV *	cur_text;
	CV *	cv;
	JMPENV *	cur_top_env; /* value of PL_top_env when eval CX created */
};

/* If we ever need more than 512 op types, change the shift from 7.
   blku_gimme is actually also only 2 bits, so could be merged with something.
*/

#define CxOLD_IN_EVAL(cx)	(((cx)->blk_u16) & 0x7F)
#define CxOLD_OP_TYPE(cx)	(((cx)->blk_u16) >> 7)

#define PUSHEVAL(cx,n)							\
	STMT_START {							\
	assert(!(PL_in_eval & ~0x7F));					\
	assert(!(PL_op->op_type & ~0x1FF));				\
	cx->blk_u16 = (PL_in_eval & 0x7F) | ((U16)PL_op->op_type << 7);	\
	cx->blk_eval.old_namesv = (n ? newSVpv(n,0) : NULL);		\
	cx->blk_eval.old_eval_root = PL_eval_root;			\
	cx->blk_eval.cur_text = PL_parser ? PL_parser->linestr : NULL;	\
	cx->blk_eval.cv = NULL; /* set by doeval(), as applicable */	\
	cx->blk_eval.retop = NULL;					\
	cx->blk_eval.cur_top_env = PL_top_env; 				\
	} STMT_END

#define POPEVAL(cx)							\
	STMT_START {							\
	PL_in_eval = CxOLD_IN_EVAL(cx);					\
	optype = CxOLD_OP_TYPE(cx);					\
	PL_eval_root = cx->blk_eval.old_eval_root;			\
	if (cx->blk_eval.old_namesv)					\
		sv_2mortal(cx->blk_eval.old_namesv);			\
	} STMT_END

/* loop context */
struct block_loop {
	I32		resetsp;
	LOOP *	my_op;	/* My op, that contains redo, next and last ops.  */
	/* (except for non_ithreads we need to modify next_op in pp_ctl.c, hence
	why next_op is conditionally defined below.)  */
#ifdef USE_ITHREADS
	PAD		*oldcomppad; /* Also used for the GV, if targoffset is 0 */
	/* This is also accesible via cx->blk_loop.my_op->op_targ */
	PADOFFSET	targoffset;
#else
	OP *	next_op;
	SV **	itervar;
#endif
	union {
	struct { /* valid if type is LOOP_FOR or LOOP_PLAIN (but {NULL,0})*/
		AV * ary; /* use the stack if this is NULL */
		IV ix;
	} ary;
	struct { /* valid if type is LOOP_LAZYIV */
		IV cur;
		IV end;
	} lazyiv;
	struct { /* valid if type if LOOP_LAZYSV */
		SV * cur;
		SV * end; /* maxiumum value (or minimum in reverse) */
	} lazysv;
	} state_u;
};

#ifdef USE_ITHREADS
#  define CxITERVAR(c)							\
	((c)->blk_loop.oldcomppad					\
	 ? (CxPADLOOP(c) 						\
		? &CX_CURPAD_SV( (c)->blk_loop, (c)->blk_loop.targoffset )	\
		: &GvSV((GV*)(c)->blk_loop.oldcomppad))			\
	 : (SV**)NULL)
#  define CX_ITERDATA_SET(cx,idata,o)					\
	if ((cx->blk_loop.targoffset = (o)))				\
		CX_CURPAD_SAVE(cx->blk_loop);				\
	else								\
		cx->blk_loop.oldcomppad = (idata);
#else
#  define CxITERVAR(c)		((c)->blk_loop.itervar)
#  define CX_ITERDATA_SET(cx,ivar,o)					\
	cx->blk_loop.itervar = (SV**)(ivar);
#endif
#define CxLABEL(c)	(0 + CopLABEL((c)->blk_oldcop))
#define CxHASARGS(c)	(((c)->cx_type & CXp_HASARGS) == CXp_HASARGS)
#define CxLVAL(c)	(0 + (c)->blk_u16)

#ifdef USE_ITHREADS
#  define PUSHLOOP_OP_NEXT		/* No need to do anything.  */
#  define CX_LOOP_NEXTOP_GET(cx)	((cx)->blk_loop.my_op->op_nextop + 0)
#else
#  define PUSHLOOP_OP_NEXT		cx->blk_loop.next_op = cLOOP->op_nextop
#  define CX_LOOP_NEXTOP_GET(cx)	((cx)->blk_loop.next_op + 0)
#endif

#define PUSHLOOP_PLAIN(cx, s)						\
	cx->blk_loop.resetsp = s - PL_stack_base;			\
	cx->blk_loop.my_op = cLOOP;					\
	PUSHLOOP_OP_NEXT;						\
	cx->blk_loop.state_u.ary.ary = NULL;				\
	cx->blk_loop.state_u.ary.ix = 0;				\
	CX_ITERDATA_SET(cx, NULL, 0);

#define PUSHLOOP_FOR(cx, dat, s, offset)				\
	cx->blk_loop.resetsp = s - PL_stack_base;			\
	cx->blk_loop.my_op = cLOOP;					\
	PUSHLOOP_OP_NEXT;						\
	cx->blk_loop.state_u.ary.ary = NULL;				\
	cx->blk_loop.state_u.ary.ix = 0;				\
	CX_ITERDATA_SET(cx, dat, offset);

#define POPLOOP(cx)							\
	if (CxTYPE(cx) == CXt_LOOP_LAZYSV) {				\
		SvREFCNT_dec(cx->blk_loop.state_u.lazysv.cur);		\
		SvREFCNT_dec(cx->blk_loop.state_u.lazysv.end);		\
	}								\
	if (CxTYPE(cx) == CXt_LOOP_FOR)					\
		SvREFCNT_dec(cx->blk_loop.state_u.ary.ary);

/* given/when context */
struct block_givwhen {
	OP *leave_op;
};

#define PUSHGIVEN(cx)							\
	cx->blk_givwhen.leave_op = cLOGOP->op_other;

#define PUSHWHEN PUSHGIVEN

/* context common to subroutines, evals and loops */
struct block {
	U8		blku_type;	/* what kind of context this is */
	U8		blku_gimme;	/* is this block running in list context? */
	U16		blku_u16;	/* used by block_sub and block_eval (so far) */
	I32		blku_oldsp;	/* stack pointer to copy stuff down to */
	COP *	blku_oldcop;	/* old curcop pointer */
	I32		blku_oldmarksp;	/* mark stack index */
	I32		blku_oldscopesp;	/* scope stack index */
	PMOP *	blku_oldpm;	/* values of pattern match vars */

	union {
	struct block_sub	blku_sub;
	struct block_format	blku_format;
	struct block_eval	blku_eval;
	struct block_loop	blku_loop;
	struct block_givwhen	blku_givwhen;
	} blk_u;
};
#define blk_oldsp	cx_u.cx_blk.blku_oldsp
#define blk_oldcop	cx_u.cx_blk.blku_oldcop
#define blk_oldmarksp	cx_u.cx_blk.blku_oldmarksp
#define blk_oldscopesp	cx_u.cx_blk.blku_oldscopesp
#define blk_oldpm	cx_u.cx_blk.blku_oldpm
#define blk_gimme	cx_u.cx_blk.blku_gimme
#define blk_u16		cx_u.cx_blk.blku_u16
#define blk_sub		cx_u.cx_blk.blk_u.blku_sub
#define blk_format	cx_u.cx_blk.blk_u.blku_format
#define blk_eval	cx_u.cx_blk.blk_u.blku_eval
#define blk_loop	cx_u.cx_blk.blk_u.blku_loop
#define blk_givwhen	cx_u.cx_blk.blk_u.blku_givwhen

/* Enter a block. */
#define PUSHBLOCK(cx,t,sp) CXINC, cx = &cxstack[cxstack_ix],		\
	cx->cx_type		= t,					\
	cx->blk_oldsp		= sp - PL_stack_base,			\
	cx->blk_oldcop		= PL_curcop,				\
	cx->blk_oldmarksp	= PL_markstack_ptr - PL_markstack,	\
	cx->blk_oldscopesp	= PL_scopestack_ix,			\
	cx->blk_oldpm		= PL_curpm,				\
	cx->blk_gimme		= (U8)gimme;				\
	DEBUG_l( PerlIO_printf(Perl_debug_log, "Entering block %ld, type %s\n",	\
			(long)cxstack_ix, PL_block_type[CxTYPE(cx)]); )

/* Exit a block (RETURN and LAST). */
#define POPBLOCK(cx,pm) cx = &cxstack[cxstack_ix--],			\
	newsp		 = PL_stack_base + cx->blk_oldsp,		\
	PL_curcop	 = cx->blk_oldcop,				\
	PL_markstack_ptr = PL_markstack + cx->blk_oldmarksp,		\
	PL_scopestack_ix = cx->blk_oldscopesp,				\
	pm		 = cx->blk_oldpm,				\
	gimme		 = cx->blk_gimme;				\
	DEBUG_SCOPE("POPBLOCK");					\
	DEBUG_l( PerlIO_printf(Perl_debug_log, "Leaving block %ld, type %s\n",		\
			(long)cxstack_ix+1,PL_block_type[CxTYPE(cx)]); )

/* Continue a block elsewhere (NEXT and REDO). */
#define TOPBLOCK(cx) cx  = &cxstack[cxstack_ix],			\
	PL_stack_sp	 = PL_stack_base + cx->blk_oldsp,		\
	PL_markstack_ptr = PL_markstack + cx->blk_oldmarksp,		\
	PL_scopestack_ix = cx->blk_oldscopesp,				\
	PL_curpm         = cx->blk_oldpm;				\
	DEBUG_SCOPE("TOPBLOCK");

/* substitution context */
struct subst {
	U8		sbu_type;	/* what kind of context this is */
	U8		sbu_rflags;
	U16		sbu_rxtainted;	/* matches struct block */
	I32		sbu_iters;
	I32		sbu_maxiters;
	I32		sbu_oldsave;
	char *	sbu_orig;
	SV *	sbu_dstr;
	SV *	sbu_targ;
	char *	sbu_s;
	char *	sbu_m;
	char *	sbu_strend;
	void *	sbu_rxres;
	REGEXP *	sbu_rx;
};
#define sb_iters	cx_u.cx_subst.sbu_iters
#define sb_maxiters	cx_u.cx_subst.sbu_maxiters
#define sb_rflags	cx_u.cx_subst.sbu_rflags
#define sb_oldsave	cx_u.cx_subst.sbu_oldsave
#define sb_once		cx_u.cx_subst.sbu_once
#define sb_rxtainted	cx_u.cx_subst.sbu_rxtainted
#define sb_orig		cx_u.cx_subst.sbu_orig
#define sb_dstr		cx_u.cx_subst.sbu_dstr
#define sb_targ		cx_u.cx_subst.sbu_targ
#define sb_s		cx_u.cx_subst.sbu_s
#define sb_m		cx_u.cx_subst.sbu_m
#define sb_strend	cx_u.cx_subst.sbu_strend
#define sb_rxres	cx_u.cx_subst.sbu_rxres
#define sb_rx		cx_u.cx_subst.sbu_rx

#ifdef PERL_CORE
#  define PUSHSUBST(cx) CXINC, cx = &cxstack[cxstack_ix],		\
	cx->sb_iters		= iters,				\
	cx->sb_maxiters		= maxiters,				\
	cx->sb_rflags		= r_flags,				\
	cx->sb_oldsave		= oldsave,				\
	cx->sb_rxtainted	= rxtainted,				\
	cx->sb_orig		= orig,					\
	cx->sb_dstr		= dstr,					\
	cx->sb_targ		= targ,					\
	cx->sb_s		= s,					\
	cx->sb_m		= m,					\
	cx->sb_strend		= strend,				\
	cx->sb_rxres		= NULL,					\
	cx->sb_rx		= rx,					\
	cx->cx_type		= CXt_SUBST | (once ? CXp_ONCE : 0);	\
	rxres_save(&cx->sb_rxres, rx);					\
	(void)ReREFCNT_inc(rx)

#  define POPSUBST(cx) cx = &cxstack[cxstack_ix--];			\
	rxres_free(&cx->sb_rxres);					\
	ReREFCNT_dec(cx->sb_rx)
#endif

#define CxONCE(cx)		((cx)->cx_type & CXp_ONCE)

struct context {
	union {
	struct block	cx_blk;
	struct subst	cx_subst;
	} cx_u;
};
#define cx_type cx_u.cx_subst.sbu_type

/* If you re-order these, there is also an array of uppercase names in perl.h
   and a static array of context names in pp_ctl.c  */
#define CXTYPEMASK	0xf
#define CXt_NULL	0
#define CXt_WHEN	1
#define CXt_BLOCK	2
/* When micro-optimising :-) keep GIVEN next to the LOOPs, as these 5 share a
   jump table in pp_ctl.c
   The first 4 don't have a 'case' in at least one switch statement in pp_ctl.c
*/
#define CXt_GIVEN	3
/* This is first so that CXt_LOOP_FOR|CXt_LOOP_LAZYIV is CXt_LOOP_LAZYIV */
#define CXt_LOOP_FOR	4
#define CXt_LOOP_PLAIN	5
#define CXt_LOOP_LAZYSV	6
#define CXt_LOOP_LAZYIV	7
#define CXt_SUB		8
#define CXt_FORMAT      9
#define CXt_EVAL       10
#define CXt_SUBST      11
/* SUBST doesn't feature in all switch statements.  */

/* private flags for CXt_SUB and CXt_NULL
   However, this is checked in many places which do not check the type, so
   this bit needs to be kept clear for most everything else. For reasons I
   haven't investigated, it can coexist with CXp_FOR_DEF */
#define CXp_MULTICALL	0x10	/* part of a multicall (so don't
				   tear down context on exit). */

/* private flags for CXt_SUB and CXt_FORMAT */
#define CXp_HASARGS	0x20

/* private flags for CXt_EVAL */
#define CXp_REAL	0x20	/* truly eval'', not a lookalike */
#define CXp_TRYBLOCK	0x40	/* eval{}, not eval'' or similar */

/* private flags for CXt_LOOP */
#define CXp_FOR_DEF	0x10	/* foreach using $_ */
#ifdef USE_ITHREADS
#  define CxPADLOOP(c)	((c)->blk_loop.targoffset)
#endif

/* private flags for CXt_SUBST */
#define CXp_ONCE	0x10	/* What was sbu_once in struct subst */

#define CxTYPE(c)	((c)->cx_type & CXTYPEMASK)
#define CxTYPE_is_LOOP(c)	(((c)->cx_type & 0xC) == 0x4)
#define CxMULTICALL(c)	(((c)->cx_type & CXp_MULTICALL)			\
			 == CXp_MULTICALL)
#define CxREALEVAL(c)	(((c)->cx_type & (CXTYPEMASK|CXp_REAL))		\
			 == (CXt_EVAL|CXp_REAL))
#define CxTRYBLOCK(c)	(((c)->cx_type & (CXTYPEMASK|CXp_TRYBLOCK))	\
			 == (CXt_EVAL|CXp_TRYBLOCK))
#define CxFOREACH(c)	(CxTYPE_is_LOOP(c) && CxTYPE(c) != CXt_LOOP_PLAIN)
#define CxFOREACHDEF(c)	((CxTYPE_is_LOOP(c) && CxTYPE(c) != CXt_LOOP_PLAIN) \
			 && ((c)->cx_type & CXp_FOR_DEF))

#define CXINC (cxstack_ix < cxstack_max ? ++cxstack_ix : (cxstack_ix = cxinc()))

/*
=head1 "Gimme" Values
*/

/*
=for apidoc AmU||G_SCALAR
Used to indicate scalar context.  See C<GIMME_V>, C<GIMME>, and
L<perlcall>.

=for apidoc AmU||G_ARRAY
Used to indicate list context.  See C<GIMME_V>, C<GIMME> and
L<perlcall>.

=for apidoc AmU||G_VOID
Used to indicate void context.  See C<GIMME_V> and L<perlcall>.

=for apidoc AmU||G_DISCARD
Indicates that arguments returned from a callback should be discarded.  See
L<perlcall>.

=for apidoc AmU||G_EVAL

Used to force a Perl C<eval> wrapper around a callback.  See
L<perlcall>.

=for apidoc AmU||G_NOARGS

Indicates that no arguments are being sent to a callback.  See
L<perlcall>.

=cut
*/

#define G_SCALAR	2
#define G_ARRAY		3
#define G_VOID		1
#define G_WANT		3

/* extra flags for Perl_call_* routines */
#define G_DISCARD	4	/* Call FREETMPS.
				   Don't change this without consulting the
				   hash actions codes defined in hv.h */
#define G_EVAL		8	/* Assume eval {} around subroutine call. */
#define G_NOARGS       16	/* Don't construct a @_ array. */
#define G_KEEPERR      32	/* Append errors to $@, don't overwrite it */
#define G_NODEBUG      64	/* Disable debugging at toplevel.  */
#define G_METHOD      128       /* Calling method. */
#define G_FAKINGEVAL  256	/* Faking an eval context for call_sv or
				   fold_constants. */

/* flag bits for PL_in_eval */
#define EVAL_NULL	0	/* not in an eval */
#define EVAL_INEVAL	1	/* some enclosing scope is an eval */
#define EVAL_WARNONLY	2	/* used by yywarn() when calling yyerror() */
#define EVAL_KEEPERR	4	/* set by Perl_call_sv if G_KEEPERR */
#define EVAL_INREQUIRE	8	/* The code is being required. */

/* Support for switching (stack and block) contexts.
 * This ensures magic doesn't invalidate local stack and cx pointers.
 */

#define PERLSI_UNKNOWN		-1
#define PERLSI_UNDEF		0
#define PERLSI_MAIN		1
#define PERLSI_MAGIC		2
#define PERLSI_SORT		3
#define PERLSI_SIGNAL		4
#define PERLSI_OVERLOAD		5
#define PERLSI_DESTROY		6
#define PERLSI_WARNHOOK		7
#define PERLSI_DIEHOOK		8
#define PERLSI_REQUIRE		9

struct stackinfo {
	AV *		si_stack;	/* stack for current runlevel */
	PERL_CONTEXT *	si_cxstack;	/* context stack for runlevel */
	struct stackinfo *	si_prev;
	struct stackinfo *	si_next;
	I32			si_cxix;	/* current context index */
	I32			si_cxmax;	/* maximum allocated index */
	I32			si_type;	/* type of runlevel */
	I32			si_markoff;	/* offset where markstack begins for us.
					 * currently used only with DEBUGGING,
					 * but not #ifdef-ed for bincompat */
};

typedef struct stackinfo PERL_SI;

#define cxstack		(PL_curstackinfo->si_cxstack)
#define cxstack_ix	(PL_curstackinfo->si_cxix)
#define cxstack_max	(PL_curstackinfo->si_cxmax)

#ifdef DEBUGGING
#  define	SET_MARK_OFFSET \
	PL_curstackinfo->si_markoff = PL_markstack_ptr - PL_markstack
#else
#  define	SET_MARK_OFFSET NOOP
#endif

#define PUSHSTACKi(type) \
	STMT_START {							\
	PERL_SI *next = PL_curstackinfo->si_next;			\
	if (!next) {							\
		next = new_stackinfo(32, 2048/sizeof(PERL_CONTEXT) - 1);	\
		next->si_prev = PL_curstackinfo;				\
		PL_curstackinfo->si_next = next;				\
	}								\
	next->si_type = type;						\
	next->si_cxix = -1;						\
	AvFILLp(next->si_stack) = 0;					\
	SWITCHSTACK(PL_curstack,next->si_stack);			\
	PL_curstackinfo = next;						\
	SET_MARK_OFFSET;						\
	} STMT_END

#define PUSHSTACK PUSHSTACKi(PERLSI_UNKNOWN)

/* POPSTACK works with PL_stack_sp, so it may need to be bracketed by
 * PUTBACK/SPAGAIN to flush/refresh any local SP that may be active */
#define POPSTACK \
	STMT_START {							\
	dSP;								\
	PERL_SI * const prev = PL_curstackinfo->si_prev;		\
	if (!prev) {							\
		PerlIO_printf(Perl_error_log, "panic: POPSTACK\n");		\
		my_exit(1);							\
	}								\
	SWITCHSTACK(PL_curstack,prev->si_stack);			\
	/* don't free prev here, free them all at the END{} */		\
	PL_curstackinfo = prev;						\
	} STMT_END

#define POPSTACK_TO(s) \
	STMT_START {							\
	while (PL_curstack != s) {					\
		dounwind(-1);						\
		POPSTACK;							\
	}								\
	} STMT_END

#define IN_PERL_COMPILETIME	(PL_curcop == &PL_compiling)
#define IN_PERL_RUNTIME		(PL_curcop != &PL_compiling)

/*
=head1 Multicall Functions

=for apidoc Ams||dMULTICALL
Declare local variables for a multicall. See L<perlcall/Lightweight Callbacks>.

=for apidoc Ams||PUSH_MULTICALL
Opening bracket for a lightweight callback.
See L<perlcall/Lightweight Callbacks>.

=for apidoc Ams||MULTICALL
Make a lightweight callback. See L<perlcall/Lightweight Callbacks>.

=for apidoc Ams||POP_MULTICALL
Closing bracket for a lightweight callback.
See L<perlcall/Lightweight Callbacks>.

=cut
*/

#define dMULTICALL \
	SV **newsp;			/* set by POPBLOCK */			\
	PERL_CONTEXT *cx;							\
	CV *multicall_cv;							\
	OP *multicall_cop;							\
	bool multicall_oldcatch; 						\
	U8 hasargs = 0		/* used by PUSHSUB */

#define PUSH_MULTICALL(the_cv) \
	STMT_START {							\
	CV * const _nOnclAshIngNamE_ = the_cv;				\
	CV * const cv = _nOnclAshIngNamE_;				\
	AV * const padlist = CvPADLIST(cv);				\
	ENTER;								\
	multicall_oldcatch = CATCH_GET;					\
	SAVETMPS; SAVEVPTR(PL_op);					\
	CATCH_SET(TRUE);						\
	PUSHSTACKi(PERLSI_SORT);					\
	PUSHBLOCK(cx, CXt_SUB|CXp_MULTICALL, PL_stack_sp);		\
	PUSHSUB(cx);							\
	if (++CvDEPTH(cv) >= 2) {					\
		PERL_STACK_OVERFLOW_CHECK();				\
		Perl_pad_push(aTHX_ padlist, CvDEPTH(cv));			\
	}								\
	SAVECOMPPAD();							\
	PAD_SET_CUR_NOSAVE(padlist, CvDEPTH(cv));			\
	multicall_cv = cv;						\
	multicall_cop = CvSTART(cv);					\
	} STMT_END

#define MULTICALL \
	STMT_START {							\
	PL_op = multicall_cop;						\
	CALLRUNOPS(aTHX);						\
	} STMT_END

#define POP_MULTICALL \
	STMT_START {							\
	LEAVESUB(multicall_cv);						\
	CvDEPTH(multicall_cv)--;					\
	POPBLOCK(cx,PL_curpm);						\
	POPSTACK;							\
	CATCH_SET(multicall_oldcatch);					\
	LEAVE;								\
	SPAGAIN;							\
	} STMT_END

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 *
 * ex: set ts=8 sts=4 sw=4 noet:
 */
