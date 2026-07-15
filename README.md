# Rogue 5.4

Reconstructed source for Rogue 5.4 (Toy, Arnold, Wichman; Berkeley 1985).
Compiled with a modified 4.3BSD toolchain, it byte-exactly reproduces
`4.3/usr/src/games/rogue/main.obj`
(md5 `919954d3025c9431b46eff78bb85fc59`) and the installed binary
`4.3/usr/games/rogue` (md5 `47850cded678b8486a6cc50ed998c745`) from CSRG
Archive CD-ROM Disc 1. `rogue.me` is the guide source
(`4.3/usr/doc/usd/33.rogue/rogue.me`,
md5 `c9e5d8f7dc48f5594ca327b52151bd01`). `rogue.6` is the manual page from
4.3BSD (`usr/man/man6/rogue.6`, md5 `d649e5466ab05203c3478edea3c6b3aa`).

The binary was stripped, so identifier names are inferred (mostly from other
rogue sources); comments and whitespace are likewise inferred from available
sources, as is most of the Makefile.
