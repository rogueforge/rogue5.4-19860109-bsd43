/*
 * save and restore routines
 *
 * @(#)save.c	X.XX (Berkeley) XX/XX/XX
 */

#include <curses.h>
#include <sys/types.h>
#include <sys/stat.h>
extern int errno;
#include <signal.h>

#include "rogue.h"
#define NSIG 32

typedef struct stat STAT;

extern char *sys_errlist[], version[], encstr[];

extern bool _endwin;

static time_t Time2;

static char Frob;

static STAT Sbuf;

/*
 * save_game:
 *	Implement the "save game" command
 */
save_game()
{
    register FILE *savef;
    register int c;
    char buf[MAXSTR];

    /*
     * get file name
     */
    Mpos = 0;
over:
    if (File_name[0] != '\0')
    {
	for (;;)
	{
	    msg("save file (%s)? ", File_name);
	    c = getchar();
	    Mpos = 0;
	    if (c == ESCAPE)
	    {
		msg("");
		return;
	    }
	    else if (c == 'n' || c == 'N' || c == 'y' || c == 'Y')
		break;
	    else
		msg("please answer Y or N");
	}
	if (c == 'y' || c == 'Y')
	{
	    addstr("Yes\n");
	    refresh();
	    strcpy(buf, File_name);
	    goto gotfile;
	}
    }

    do
    {
	Mpos = 0;
	msg("file name: ");
	buf[0] = '\0';
	if (get_str(buf, stdscr) == QUIT)
	{
quit_it:
	    msg("");
	    return;
	}
	Mpos = 0;
gotfile:
	/*
	 * test to see if the file exists
	 */
	if (stat(buf, &Sbuf) >= 0)
	{
	    for (;;)
	    {
		msg("File exists.  Do you wish to overwrite it?");
		Mpos = 0;
		if ((c = readchar()) == ESCAPE)
		    goto quit_it;
		if (c == 'y' || c == 'Y')
		    break;
		else if (c == 'n' || c == 'N')
		    goto over;
		else
		    msg("Please answer Y or N");
	    }
	    msg("file name: %s", buf);
	    unlink(File_name);
	}
	strcpy(File_name, buf);
	if ((savef = fopen(File_name, "w")) == NULL)
	    msg(sys_errlist[errno]);
    } while (savef == NULL);

    save_file(savef);
    /* NOTREACHED */
}

/*
 * auto_save:
 *	Automatically save a file.  This is used if a HUP signal is
 *	recieved
 */
auto_save(sig)
int sig;
{
    register FILE *savef;
    register int i;

    for (i = 0; i < NSIG; i++)
	signal(i, SIG_IGN);
    if (File_name[0] != '\0' && ((savef = fopen(File_name, "w")) != NULL ||
	(unlink(File_name) >= 0 && (savef = fopen(File_name, "w")) != NULL)))
	    save_file(savef);
    exit(0);
}

/*
 * save_file:
 *	Write the saved game on the file
 */
save_file(savef)
register FILE *savef;
{
    register char *ep;
    int _putchar(), creat();

    mvcur(0, COLS - 1, LINES - 1, 0);
    if (!CE)
	putchar('\n');
    else
	tputs(CE, 0, _putchar);
    endwin();
#ifdef TIOCGLTC
    Ltc.t_dsuspc = Orig_dsusp;
    ioctl(1, TIOCSLTC, &Ltc);
#endif
    chmod(File_name, 0400);
    /*
     * DO NOT DELETE.  This forces stdio to allocate the output buffer
     * so that malloc doesn't get confused on restart
     */
    Frob = rnd(256);
    fwrite(&Frob, sizeof Frob, 1, savef);

    _endwin = TRUE;
    ep = (char *) ((char *) sbrk(0) - version);
    fstat(fileno(savef), &Sbuf);
    encwrite(version, ep, savef);
    fflush(savef);
    ep = version + 1;
    Sbuf.st_ctime = time(0);
    lseek(fileno(savef), (long) ((char *) &Sbuf.st_ctime - ep), 0);
    write(fileno(savef), (char *) &Sbuf.st_ctime, sizeof Sbuf.st_ctime);
    Time2 = time(0);
    lseek(fileno(savef), (long) ((char *) &Time2 - ep), 0);
    write(fileno(savef), (char *) &Time2, sizeof Time2);
    fclose(savef);
    exit(0);
}

/*
 * restore:
 *	Restore a saved game from a file with elaborate checks for file
 *	integrity from cheaters
 */
restore(file, envp)
register char *file;
char **envp;
{
    register int inf;
    register char *sp;
    register bool syml;
    register char *ep;
    char fb;
    extern char **environ;
    char buf[MAXSTR];
    STAT sbuf2;

    if (strcmp(file, "-r") == 0)
	file = File_name;

#ifdef SIGTSTP
    /*
     * If a process can be suspended, this code wouldn't work
     */
#ifdef SIG_HOLD
    signal(SIGTSTP, SIG_HOLD);
#else
    signal(SIGTSTP, SIG_IGN);
#endif
#endif

    if ((inf = open(file, 0)) < 0)
    {
	perror(file);
	return FALSE;
    }
    fstat(inf, &sbuf2);
    syml = symlink(file);
    if (
#ifdef MASTER
	!Wizard &&
#endif
	unlink(file) < 0)
    {
	printf("Cannot unlink file\n");
	return FALSE;
    }

    fflush(stdout);
    read(inf, &Frob, sizeof Frob);
    fb = Frob;
    encread(buf, (unsigned) strlen(version) + 1, inf);
    if (strcmp(buf, version) != 0)
    {
	printf("Sorry, saved game is out of date.\n");
	return FALSE;
    }

    sbuf2.st_size -= sizeof Frob;
    brk(version + sbuf2.st_size);
    lseek(inf, (long) sizeof Frob, 0);
    Frob = fb;
    encread(version, (unsigned int) sbuf2.st_size, inf);
    ep = version + 1;
    lseek(inf, (long) ((char *) &Sbuf.st_ctime - ep), 0);
    read(inf, &Sbuf.st_ctime, sizeof Sbuf.st_ctime);
    lseek(inf, (long) ((char *) &Time2 - ep), 0);
    read(inf, &Time2, sizeof Time2);
    /*
     * we do not close the file so that we will have a hold of the
     * inode for as long as possible
     */
    fuse(close, inf, rnd(25) + 1, BEFORE);

#ifdef MASTER
    if (!Wizard)
#endif
	if (sbuf2.st_ino != Sbuf.st_ino || sbuf2.st_dev != Sbuf.st_dev)
	{
	    printf("Sorry, saved game is not in the same file.\n");
	    return FALSE;
	}
	else if (sbuf2.st_ctime - Time2 > Time2 - Sbuf.st_ctime + 1)
	{
	    printf("Sorry, file has been touched\n");
	    return FALSE;
	}
    Mpos = 0;
    mvprintw(0, 0, "%s: %s", file, ctime(&sbuf2.st_mtime));

    /*
     * defeat multiple restarting from the same place
     */
#ifdef MASTER
    if (!Wizard)
#endif
	if (sbuf2.st_nlink != 1 || syml)
	{
	    printf("Cannot restore from a linked file\n");
	    return FALSE;
	}

    if (Pstats.s_hpt <= 0)
    {
	printf("\"He's dead, Jim\"\n");
	return FALSE;
    }
#ifdef SIGTSTP
    signal(SIGTSTP, tstp);
#endif

    environ = envp;
    gettmode();
    if ((sp = getenv("TERM")) == NULL)
	sp = Def_term;
    setterm(sp);
    strcpy(File_name, file);
    setup();
    clearok(curscr, TRUE);
    srand(getpid());
    msg("file name: %s", file);
    playit();
    /*NOTREACHED*/
}

/*
 * encwrite:
 *	Perform an encrypted write
 */
encwrite(start, size, outf)
register char *start;
unsigned int size;
FILE *outf;
{
    register char *e1, *e2;
    register int temp;
    char fb;
    extern char statlist[];

    e1 = encstr;
    e2 = statlist;
    fb = Frob;

    while (size--)
    {
	putc(*start++ ^ *e1 ^ *e2 ^ fb, outf);
	temp = *e1++;
	fb += *e2++ * temp;
	if (*e1 == '\0')
	    e1 = encstr;
	if (*e2 == '\0')
	    e2 = statlist;
    }
}

/*
 * encread:
 *	Perform an encrypted read
 */
encread(start, size, inf)
register char *start;
unsigned int size;
int inf;
{
    register char *e1, *e2;
    register int temp;
    register int read_size;
    char fb;
    extern char statlist[];

    fb = Frob;

    if ((read_size = read(inf, start, size)) == 0 || read_size == -1)
	return;

    e1 = encstr;
    e2 = statlist;

    while (size--)
    {
	*start++ ^= *e1 ^ *e2 ^ fb;
	temp = *e1++;
	fb += *e2++ * temp;
	if (*e1 == '\0')
	    e1 = encstr;
	if (*e2 == '\0')
	    e2 = statlist;
    }
}

/*
 * read_scrore
 *	Read in the score file
 */
rd_score(top_ten, fd)
SCORE *top_ten;
int fd;
{
    encread((char *) top_ten, Numscores * sizeof (SCORE), fd);
}

/*
 * write_scrore
 *	Read in the score file
 */
wr_score(top_ten, outf)
SCORE *top_ten;
FILE *outf;
{
    encwrite((char *) top_ten, Numscores * sizeof (SCORE), outf);
}
