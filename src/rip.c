/*
 * File for the fun ends
 * Death or a total win
 *
 * @(#)rip.c	X.XX (Berkeley) XX/XX/XX
 */

#include <curses.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <pwd.h>
#include <ctype.h>
#include "rogue.h"

#include <fcntl.h>

static char *reason[] = {
    "killed",
    "quit",
    "A total winner",
    "killed with Amulet"
};

static char *Replay_prompt = "[Hit any key to quit, 'r' to replay]";

/*
 * score:
 *	Figure score and post it.
 */
/* VARARGS2 */
score(amount, flags, monst)
int amount;
int flags;
char monst;
{
    register SCORE *scp;
    register int i;
    register SCORE *sc2;
    register SCORE *top_ten, *endp;
    register FILE *outf;
    int (*fp)();
    int uid;

    start_score();

    if (flags >= 0)
    {
	if (Hw != NULL)
	    delwin(Hw);
    }

    if (Fd >= 0)
	outf = fdopen(Fd, "w");
    else
    {
	if (flags >= 0
#ifdef MASTER
	    || Wizard
#endif
	    )
	    goto play_again;
	return;
    }

    top_ten = (SCORE *) malloc(Numscores * sizeof (SCORE));
    endp = &top_ten[Numscores];
    for (scp = top_ten; scp < endp; scp++)
    {
	scp->sc_score = 0;
	for (i = 0; i < MAXSTR; i++)
	    scp->sc_name[i] = rnd(255);
	scp->sc_flags = RN;
	scp->sc_level = RN;
	scp->sc_monster = RN;
	scp->sc_uid = RN;
    }
    rd_score(top_ten, Fd);
    /*
     * Insert her in list if need be
     */
    sc2 = NULL;
    if (Noscore)
	goto display;
    uid = getuid();
    for (scp = top_ten; scp < endp; scp++)
	if (amount > scp->sc_score)
	    break;
	else if (!Allscore &&	/* only one score per nowin uid */
	    flags != 2 && scp->sc_uid == uid && scp->sc_flags != 2)
		scp = endp;
    if (scp < endp)
    {
	if (flags != 2 && !Allscore)
	{
	    for (sc2 = scp; sc2 < endp; sc2++)
	    {
		if (sc2->sc_uid == uid && sc2->sc_flags != 2)
		    break;
	    }
	    if (sc2 >= endp)
		sc2 = endp - 1;
	}
	else
	    sc2 = endp - 1;
	while (sc2 > scp)
	{
	    *sc2 = sc2[-1];
	    sc2--;
	}
	scp->sc_score = amount;
	strncpy(scp->sc_name, Whoami, MAXSTR);
	scp->sc_flags = flags;
	if (flags == 2)
	    scp->sc_level = Max_level;
	else
	    scp->sc_level = Level;
	scp->sc_monster = monst;
	scp->sc_uid = uid;
	sc2 = scp;
    }
    fseek(outf, 0L, 0);
    /*
     * Update the list file
     */
    if (sc2 != NULL)
    {
	if (lock_sc())
	{
	    fp = signal(SIGINT, SIG_IGN);
	    wr_score(top_ten, outf);
	    unlock_sc();
	    signal(SIGINT, fp);
	}
    }
    fclose(outf);
display:
    if (flags >= 0 && sc2 == NULL && !Tombstone)
	goto play_again;
    /*
     * Print the list
     */
    if (flags >= 0)
	fp = printw;
    else
	fp = printf;
    if (fp == printw)
	move(LINES - 3 - Numscores, 0);
    (*fp)("Top %s %s:", Numname, Allscore ? "Scores" : "Rogueists");
    if (fp == printw)
	move(LINES - 2 - Numscores, 0);
    else
	putchar('\n');
    (*fp)("   Score Name");
    if (fp == printw)
	move(LINES - 1 - Numscores, 0);
    else
	putchar('\n');
    i = LINES - Numscores - 1;
    for (scp = top_ten; scp < endp; scp++)
    {
	if (!scp->sc_score)
	    break;
	if (fp == printw)
	{
	    move(i++, 0);
	    if (sc2 == scp)
		standout();
	}
	(*fp)("%2d %5d %s: %s on level %d", scp - top_ten + 1,
	    scp->sc_score, scp->sc_name, reason[scp->sc_flags],
	    scp->sc_level);
	if (scp->sc_flags == 0 || scp->sc_flags == 3)
	    (*fp)(" by %s", killname((char) scp->sc_monster, TRUE));
	(*fp)("\n");
	if (sc2 == scp && fp == printw)
	    standend();
    }
    if (flags >= 0)
    {
play_again:
	mvaddstr(LINES - 1, center(Replay_prompt), Replay_prompt);
	wrefresh(stdscr);
	flush_type();
	i = readchar();
	move(LINES - 1, 0);
	clrtoeol();
	wrefresh(stdscr);
	endwin();
#ifdef TIOCGLTC
	if (Got_ltc)
	{
	    Ltc.t_dsuspc = Orig_dsusp;
	    ioctl(1, TIOCSLTC, &Ltc);
	}
#endif
	if (i == 'r' || i == 'R')
	    replay();
	exit(0);
    }
}

/*
 * death:
 *	Do something really fun when he dies
 */
death(monst)
char monst;
{
    register int dum;
    register char *killer;
    register struct tm *lt;
    register int col;
    static time_t date;
    struct tm *localtime();

    signal(SIGINT, SIG_IGN);
    Purse -= Purse / 10;
    signal(SIGINT, leave);
    clear();
    if (!Tombstone)
    {
	sprintf(Prbuf, "Killed by %s with %d gold",
	killname(monst, TRUE), Purse);
	mvprintw(1, center(Prbuf), Prbuf);
    }
    else
    {
	killer = killname(monst, FALSE);
	col = center("________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______");
	mvaddstr(0, col, "              __________");
	mvaddstr(1, col, "             /          \\");
	mvaddstr(2, col, "            /    REST    \\");
	mvaddstr(3, col, "           /      IN      \\");
	mvaddstr(4, col, "          /     PEACE      \\");
	mvaddstr(5, col, "         /                  \\");
	mvaddstr(6, col, "         |                  |");
	mvaddstr(7, col, "         |                  |");
	if (monst == 's' || monst == 'h')
	    mvaddstr(8, col, "         |     killed by    |");
	else
	    mvprintw(8, col, "         |    killed by a%-1.1s  |",
		vowelstr(killer));
	mvaddstr(9, col, "         |                  |");
	mvaddstr(10, col, "         |                  |");
	mvaddstr(11, col, "        *|     *  *  *      | *");
	mvaddstr(12, col, "________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______");
	mvaddstr(6, center(Whoami), Whoami);
	sprintf(Prbuf, "%d Au", Purse);
	mvaddstr(7, center(Prbuf), Prbuf);
	time(&date);
	lt = localtime(&date);
	mvaddstr(9, center(killer), killer);
	sprintf(Prbuf, "19%02d", lt->tm_year);
	mvaddstr(10, center(Prbuf), Prbuf);
    }
    move(LINES - 1, 0);
    refresh();
    score(Purse, Amulet ? 3 : 0, monst);
    /*NOTREACHED*/
}

/*
 * center:
 *	Return the index to center the given string
 */
int
center(str)
char *str;
{
    int len;

    len = strlen(str) / 2;
    return 40 - len;
}

/*
 * total_winner:
 *	Code for a winner
 */
total_winner()
{
    register THING *obj;
    register struct obj_info *op;
    register int worth;
    register int oldpurse;

    clear();
    standout();
    addstr("                                                               \n");
    addstr("  @   @               @   @           @          @@@  @     @  \n");
    addstr("  @   @               @@ @@           @           @   @     @  \n");
    addstr("  @   @  @@@  @   @   @ @ @  @@@   @@@@  @@@      @  @@@    @  \n");
    addstr("   @@@@ @   @ @   @   @   @     @ @   @ @   @     @   @     @  \n");
    addstr("      @ @   @ @   @   @   @  @@@@ @   @ @@@@@     @   @     @  \n");
    addstr("  @   @ @   @ @  @@   @   @ @   @ @   @ @         @   @  @     \n");
    addstr("   @@@   @@@   @@ @   @   @  @@@@  @@@@  @@@     @@@   @@   @  \n");
    addstr("                                                               \n");
    addstr("     Congratulations, you have made it to the light of day!    \n");
    standend();
    addstr("\nYou have joined the elite ranks of those who have escaped the\n");
    addstr("Dungeons of Doom alive.  You journey home and sell all your loot at\n");
    addstr("a great profit and are admitted to the Fighters' Guild.\n");
    mvaddstr(LINES - 1, 0, "--Press space to continue--");
    refresh();
    wait_for(' ');
    clear();
    mvaddstr(0, 0, "   Worth  Item\n");
    oldpurse = Purse;
    for (obj = Pack; obj != NULL; obj = next(obj))
    {
	switch (obj->o_type)
	{
	    when FOOD:
		worth = 2 * obj->o_count;
	    when WEAPON:
		worth = Weap_info[obj->o_which].oi_worth;
		worth *= 3 * (obj->o_hplus + obj->o_dplus) + obj->o_count;
		obj->o_flags |= ISKNOW;
	    when ARMOR:
		worth = Arm_info[obj->o_which].oi_worth;
		worth += (9 - obj->o_arm) * 100;
		worth += (10 * (A_class[obj->o_which] - obj->o_arm));
		obj->o_flags |= ISKNOW;
	    when SCROLL:
		worth = Scr_info[obj->o_which].oi_worth;
		worth *= obj->o_count;
		op = &Scr_info[obj->o_which];
		if (!op->oi_know)
		    worth /= 2;
		op->oi_know = TRUE;
	    when POTION:
		worth = Pot_info[obj->o_which].oi_worth;
		worth *= obj->o_count;
		op = &Pot_info[obj->o_which];
		if (!op->oi_know)
		    worth /= 2;
		op->oi_know = TRUE;
	    when RING:
		op = &Ring_info[obj->o_which];
		worth = op->oi_worth;
		if (obj->o_which == R_ADDSTR || obj->o_which == R_ADDDAM ||
		    obj->o_which == R_PROTECT || obj->o_which == R_ADDHIT)
			if (obj->o_arm > 0)
			    worth += obj->o_arm * 100;
			else
			    worth = 10;
		if (!(obj->o_flags & ISKNOW))
		    worth /= 2;
		obj->o_flags |= ISKNOW;
		op->oi_know = TRUE;
	    when STICK:
		op = &Ws_info[obj->o_which];
		worth = op->oi_worth;
		worth += 20 * obj->o_charges;
		if (!(obj->o_flags & ISKNOW))
		    worth /= 2;
		obj->o_flags |= ISKNOW;
		op->oi_know = TRUE;
	    when AMULET:
		worth = 1000;
	}
	if (worth < 0)
	    worth = 0;
	printw("%c) %5d  %s\n", obj->o_packch, worth, inv_name(obj, FALSE));
	Purse += worth;
    }
    printw("   %5d  Gold Pieces          ", oldpurse);
    refresh();
    score(Purse, 2);
    my_exit(0);
}

/*
 * killname:
 *	Convert a code to a monster name
 */
char *
killname(monst, doart)
char monst;
bool doart;
{
    register struct h_list *hp;
    register char *sp;
    bool article;
    static char Prbuf[MAXSTR];
    static struct h_list nlist[] = {
	'a',	"arrow",		TRUE,
	'b',	"bolt",			TRUE,
	'd',	"dart",			TRUE,
	'h',	"hypothermia",		FALSE,
	's',	"starvation",		FALSE,
	'\0'
    };

    if (isupper(monst))
    {
	sp = Monsters[monst-'A'].m_name;
	article = TRUE;
    }
    else
    {
	sp = "Wally the Wonder Badger";
	article = FALSE;
	for (hp = nlist; hp->h_ch; hp++)
	    if (hp->h_ch == monst)
	    {
		sp = hp->h_desc;
		article = hp->h_print;
		break;
	    }
    }
    if (doart && article)
	sprintf(Prbuf, "a%s ", vowelstr(sp));
    else
	Prbuf[0] = '\0';
    strcat(Prbuf, sp);
    return Prbuf;
}

/*
 * replay:
 *	Play the game again
 */
replay()
{
    execlp(Progname, Progname, 0);
    perror(Progname);
    exit(0);
}

/*
 * death_monst:
 *	Return a monster appropriate for a random death.
 */
char
death_monst()
{
    static char poss[] =
    {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'h', 'd', 's',
	' '	/* This is provided to generate the "Wally the Wonder Badger"
		 * message for killer */
    };

    return poss[rnd(sizeof poss / sizeof (char))];
}
