/*
 * Defines for things used in mach_dep.c
 *
 * @(#)extern.h X.XX (Berkeley) XX/XX/XX
 */

/*
 * Don't change the constants, since they are used for sizes in many
 * places in the program.
 */

#define MAXSTR		80	/* maximum length of strings */
#define MAXLINES	32	/* maximum number of screen lines used */
#define MAXCOLS		80	/* maximum number of screen columns used */

#define RN		(((Seed = Seed*11109+13849) >> 16) & 0xffff)
#ifdef CTRL
#undef CTRL
#endif
#define CTRL(c)		('c' & 037)

#undef unctrl

/*
 * Now all the global variables
 */

extern bool	Got_ltc, In_shell, Wizard;

extern char	Fruit[], Orig_dsusp, Prbuf[], Whoami[];

extern int	Fd;

#ifdef TIOCGLTC
extern struct ltchars	Ltc;
#endif

/*
 * Function types
 */

char	*brk(), *charge_str(), *choose_str(), *ctime(), *getenv(),
	*inv_name(), *killname(), *malloc(), *nothing(), *nullstr(),
	*num(), *pick_color(), *ring_num(), *sbrk(), *set_mname(),
	*sprintf(), *strcat(), *strcpy(), *type_name(), *vowelstr();

int	auto_save(), come_down(), doctor(), endit(), land(), leave(),
	nohaste(), quit(), rollwand(), runners(), sight(), stomach(),
	swander(), tstp(), turn_see(), unconfuse(), unsee(), visuals();

#ifdef CHECKTIME
int	checkout();
#endif

long	lseek(), time();
