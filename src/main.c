/*
 * Exploring the dungeons of doom
 * Copyright (C) 1981 by Michael Toy, Ken Arnold, and Glenn Wichman
 * All rights reserved
 *
 * @(#)main.c	X.XX (Berkeley) XX/XX/XX
 */

#include <curses.h>
#include <signal.h>
#include <pwd.h>
#include "rogue.h"
long Dnum;

/*
 * main:
 *	The main program, of course
 */
main(argc, argv, envp)
int argc;
char **argv;
char **envp;
{
    register char *env;
    register struct passwd *pw;
    register int lowtime;
    int quit(), exit();
    char *getpass();

#ifndef DUMP
    signal(SIGQUIT, exit);
    signal(SIGILL, exit);
    signal(SIGTRAP, exit);
    signal(SIGIOT, exit);
    signal(SIGEMT, exit);
    signal(SIGFPE, exit);
    signal(SIGBUS, exit);
    signal(SIGSEGV, exit);
    signal(SIGSYS, exit);
#endif

#ifdef MASTER
    /*
     * Check to see if he is a wizard
     */
    if (argc >= 2 && argv[1][0] == '\0')
	if (strcmp(PASSWD, crypt(getpass("Wizard's password: "), "mT")) == 0)
	{
	    Wizard = TRUE;
	    Player.t_flags |= SEEMONST;
	    argv++;
	    argc--;
	}
#endif

    Progname = *argv;

    /*
     * get Home and options from environment
     */
    if ((env = getenv("HOME")) != NULL)
	strcpy(Home, env);
    else if ((pw = getpwuid(getuid())) != NULL)
	strcpy(Home, pw->pw_dir);
    strcat(Home, "/");

    strcpy(File_name, Home);
    strcat(File_name, "rogue.save");

    if ((env = getenv("ROGUEOPTS")) != NULL)
	parse_opts(env);
    if (env == NULL || Whoami[0] == '\0')
	if ((pw = getpwuid(getuid())) == NULL)
	{
	    printf("Say, who the hell are you?\n");
	    exit(1);
	}
	else
	    strucpy(Whoami, pw->pw_name, strlen(pw->pw_name));

    lowtime = (int) time(NULL);
#ifdef MASTER
    if (Wizard && getenv("SEED") != NULL)
	Dnum = atoi(getenv("SEED"));
    else
#endif
	Dnum = lowtime + getpid();
    Seed = Dnum;

    /*
     * check for print-score option
     */
    open_score();
    if (argc == 2)
	if (strcmp(argv[1], "-s") == 0)
	{
	    Noscore = TRUE;
	    score(0, -1);
	    exit(0);
	}
	else if (strcmp(argv[1], "-d") == 0)
	{
	    Dnum = rnd(100);	/* throw away some rnd()s to break patterns */
	    while (--Dnum)
		rnd(100);
	    Purse = rnd(100) + 1;
	    Level = rnd(100) + 1;
	    initscr();
	    crmode();
	    getltchars();
	    death(death_monst());
	    exit(0);
	}

    init_check();			/* check for legal startup */
    if (argc == 2)
	if (!restore(argv[1], envp))	/* Note: restore will never return */
	    my_exit(1);
#ifdef MASTER
    if (Wizard)
	printf("Hello %s, welcome to dungeon #%d", Whoami, Dnum);
    else
#endif
	printf("Hello %s, just a moment while I dig the dungeon...", Whoami);
    fflush(stdout);

    initscr();				/* Start up cursor package */
    init_probs();			/* Set up prob tables for objects */
    init_player();			/* Set up initial Player stats */
    init_names();			/* Set up names of scrolls */
    init_colors();			/* Set up colors of potions */
    init_stones();			/* Set up stone settings of rings */
    init_materials();			/* Set up materials of wands */
    setup();

    /*
     * The screen must be at least NUMLINES x NUMCOLS
     */
    if (LINES < NUMLINES || COLS < NUMCOLS)
    {
	printf("Sorry, the screen must be at least %dx%d\n", NUMLINES, NUMCOLS);
	endwin();
	my_exit(1);
    }

    /*
     * Set up windows
     */
    Hw = newwin(LINES, COLS, 0, 0);
#ifdef MASTER
    Noscore = Wizard;
#endif
    new_level();			/* Draw current level */
    /*
     * Start up daemons and fuses
     */
    daemon(runners, 0, AFTER);
    daemon(doctor, 0, AFTER);
    daemon(stomach, 0, AFTER);
    fuse(swander, 0, WANDERTIME, AFTER);
    playit();
}

/*
 * endit:
 *	Exit the program abnormally.
 */
endit(sig)
int sig;
{
    fatal("Okay, bye bye!");
}

/*
 * fatal:
 *	Exit the program, printing a message.
 */
fatal(s)
char *s;
{
    mvcur(0, COLS - 1, LINES - 1, 0);
    if (CE)
    {
	tputs(CE, 1, _putchar);
	endwin();
    }
    else
    {
	endwin();
	putchar('\n');
    }
    printf("%s\n", s);
    my_exit(0);
}

/*
 * rnd:
 *	Pick a very random number.
 */
unsigned
rnd(range)
register unsigned range;
{
    return range == 0 ? 0 : RN % range;
}

/*
 * roll:
 *	Roll a number of dice
 */
roll(number, sides)
register int number, sides;
{
    register int dtotal = 0;

    while (number--)
	dtotal += rnd(sides)+1;
    return dtotal;
}

#ifdef SIGTSTP
/*
 * tstp:
 *	Handle stop and start signals
 */
tstp(ignored)
int ignored;
{
    struct sgttyb save;

    save = _tty;
    /*
     * leave nicely
     */
    mvcur(0, COLS - 1, LINES - 1, 0);
    endwin();
    fflush(stdout);
    if (Got_ltc && Orig_dsusp == ('Y' & 037))
    {
	Ltc.t_dsuspc = Orig_dsusp;
	ioctl(0, TIOCSLTC, &Ltc);
    }
    kill(0, SIGSTOP);		/* send actual signal and suspend process */

    /*
     * start back up again
     */
    signal(SIGTSTP, tstp);
    _tty = save;
    stty(_tty_ch, &_tty);
    if (Got_ltc && Orig_dsusp == ('Y' & 037))
    {
	Ltc.t_dsuspc = Ltc.t_suspc;
	ioctl(0, TIOCSLTC, &Ltc);
    }
    wrefresh(curscr);
}
#endif

/*
 * playit:
 *	The main loop of the program.  Loop until the game is over,
 *	refreshing things and looking at the proper times.
 */
playit()
{
    register char *opts;

    /*
     * set up defaults for slow terminals
     */

    if (_tty.sg_ospeed <= B1200)
    {
	Terse = TRUE;
	Jump = TRUE;
	See_floor = FALSE;
    }
    if (!CE)
	Inv_type = INV_CLEAR;

    /*
     * parse environment declaration of options
     */
    if ((opts = getenv("ROGUEOPTS")) != NULL)
	parse_opts(opts);

    Oldpos = Hero;
    Oldrp = roomin(&Hero);
    while (Playing)
	command();			/* Command execution */
    endit();
}

/*
 * quit:
 *	Have player make certain, then exit.
 */
quit(sig)
int sig;
{
    register int oy, ox;

    /*
     * Reset the signal in case we got here via an interrupt
     */
    if (!Q_comm)
	Mpos = 0;
    getyx(curscr, oy, ox);
    msg("really quit?");
    if (readchar() == 'y')
    {
	if (!Q_comm)
	    sigsetmask(0);
	signal(SIGINT, leave);
	clear();
	sprintf(Prbuf, "You quit with %d gold pieces", Purse);
	mvaddstr(0, center(Prbuf), Prbuf);
	score(Purse, 1);
    }
    else
    {
	move(0, 0);
	clrtoeol();
	status();
	move(oy, ox);
	refresh();
	Mpos = 0;
	Count = 0;
	To_death = FALSE;
    }
}

/*
 * leave:
 *	Leave quickly, but curteously
 */
leave(sig)
int sig;
{
    static char buf[BUFSIZ];

    setbuf(stdout, buf);	/* throw away pending output */
    if (!_endwin)
    {
	mvcur(0, COLS - 1, LINES - 1, 0);
	endwin();
    }
    putchar('\n');
    my_exit(0);
}

/*
 * shell:
 *	Let them escape for a while
 */
shell()
{
    register int pid;
    register char *sh;
    int ret_status;

    /*
     * Set the terminal back to original mode
     */
    move(LINES-1, 0);
    refresh();
    endwin();
#ifdef SIGTSTP
    Ltc.t_dsuspc = Orig_dsusp;
    ioctl(1, TIOCSLTC, &Ltc);
#endif
    putchar('\n');
    In_shell = TRUE;
    After = FALSE;
    sh = getenv("SHELL");
    fflush(stdout);
    /*
     * Fork and do a shell
     */
    if ((pid = fork()) < 0)
	perror("forking shell");
    else if (pid == 0)
    {
	execl(sh == NULL ? "/bin/sh" : sh, "shell", "-i", 0);
	perror("No shelly");
	exit(-1);
    }
    else
    {
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	while (wait(&ret_status) != pid)
	    continue;
	signal(SIGINT, quit);
	signal(SIGQUIT, endit);
	printf("\n[Press return to continue]");
	fflush(stdout);
	noecho();
	crmode();
#ifdef SIGTSTP
	Ltc.t_dsuspc = Ltc.t_suspc;
	ioctl(1, TIOCSLTC, &Ltc);
#endif
	In_shell = FALSE;
	wait_for('\n');
	wrefresh(curscr);
    }
}

/*
 * my_exit:
 *	Leave the process properly
 */
my_exit(st)
int st;
{
#ifdef TIOCGLTC
    if (Got_ltc)
    {
	Ltc.t_dsuspc = Orig_dsusp;
	ioctl(1, TIOCSLTC, &Ltc);
    }
#endif
    exit(st);
}
