# Generated by h2py from /usr/include/sys/fcntl.h

# Included from sys/feature_tests.h
_POSIX_C_SOURCE = 1

# Included from sys/types.h

# Included from sys/isa_defs.h
_CHAR_ALIGNMENT = 1
_SHORT_ALIGNMENT = 2
_INT_ALIGNMENT = 4
_LONG_ALIGNMENT = 4
_LONG_LONG_ALIGNMENT = 4
_DOUBLE_ALIGNMENT = 4
_LONG_DOUBLE_ALIGNMENT = 4
_POINTER_ALIGNMENT = 4
_MAX_ALIGNMENT = 4
_ALIGNMENT_REQUIRED = 0
_CHAR_ALIGNMENT = 1
_SHORT_ALIGNMENT = 2
_INT_ALIGNMENT = 4
_LONG_ALIGNMENT = 4
_LONG_LONG_ALIGNMENT = 8
_DOUBLE_ALIGNMENT = 8
_LONG_DOUBLE_ALIGNMENT = 16
_POINTER_ALIGNMENT = 4
_MAX_ALIGNMENT = 16
_ALIGNMENT_REQUIRED = 1
_CHAR_ALIGNMENT = 1
_SHORT_ALIGNMENT = 2
_INT_ALIGNMENT = 4
_LONG_ALIGNMENT = 4
_LONG_LONG_ALIGNMENT = 8
_DOUBLE_ALIGNMENT = 8
_LONG_DOUBLE_ALIGNMENT = 8
_POINTER_ALIGNMENT = 4
_MAX_ALIGNMENT = 8
_ALIGNMENT_REQUIRED = 1

# Included from sys/machtypes.h
SHRT_MIN = -32768
SHRT_MAX = 32767
INT_MIN = (-2147483647-1)
INT_MAX = 2147483647
LONG_MIN = (-2147483647-1)
LONG_MAX = 2147483647
P_MYID = (-1)

# Included from sys/select.h

# Included from sys/time.h
DST_NONE = 0
DST_USA = 1
DST_AUST = 2
DST_WET = 3
DST_MET = 4
DST_EET = 5
DST_CAN = 6
DST_GB = 7
DST_RUM = 8
DST_TUR = 9
DST_AUSTALT = 10
ITIMER_REAL = 0
ITIMER_VIRTUAL = 1
ITIMER_PROF = 2
ITIMER_REALPROF = 3
SEC = 1
MILLISEC = 1000
MICROSEC = 1000000
NANOSEC = 1000000000
__CLOCK_REALTIME0 = 0
CLOCK_VIRTUAL = 1
CLOCK_PROF = 2
__CLOCK_REALTIME3 = 3
CLOCK_REALTIME = __CLOCK_REALTIME3
CLOCK_REALTIME = __CLOCK_REALTIME0
TIMER_RELTIME = 0x0
TIMER_ABSTIME = 0x1

# Included from sys/mutex.h

# Included from sys/dki_lkinfo.h

# Included from sys/dl.h
NOSTATS = 1
LSB_NLKDS = 91
def MUTEX_HELD(x): return (mutex_owned(x))


# Included from time.h
NULL = 0
CLOCKS_PER_SEC = 1000000

# Included from sys/siginfo.h
SIGEV_NONE = 1
SIGEV_SIGNAL = 2
SIGEV_THREAD = 3
SI_NOINFO = 32767
SI_USER = 0
SI_LWP = (-1)
SI_QUEUE = (-2)
SI_TIMER = (-3)
SI_ASYNCIO = (-4)
SI_MESGQ = (-5)

# Included from sys/machsig.h
ILL_ILLOPC = 1
ILL_ILLOPN = 2
ILL_ILLADR = 3
ILL_ILLTRP = 4
ILL_PRVOPC = 5
ILL_PRVREG = 6
ILL_COPROC = 7
ILL_BADSTK = 8
NSIGILL = 8
EMT_TAGOVF = 1
NSIGEMT = 1
FPE_INTDIV = 1
FPE_INTOVF = 2
FPE_FLTDIV = 3
FPE_FLTOVF = 4
FPE_FLTUND = 5
FPE_FLTRES = 6
FPE_FLTINV = 7
FPE_FLTSUB = 8
NSIGFPE = 8
SEGV_MAPERR = 1
SEGV_ACCERR = 2
NSIGSEGV = 2
BUS_ADRALN = 1
BUS_ADRERR = 2
BUS_OBJERR = 3
NSIGBUS = 3
TRAP_BRKPT = 1
TRAP_TRACE = 2
NSIGTRAP = 2
CLD_EXITED = 1
CLD_KILLED = 2
CLD_DUMPED = 3
CLD_TRAPPED = 4
CLD_STOPPED = 5
CLD_CONTINUED = 6
NSIGCLD = 6
POLL_IN = 1
POLL_OUT = 2
POLL_MSG = 3
POLL_ERR = 4
POLL_PRI = 5
POLL_HUP = 6
NSIGPOLL = 6
PROF_SIG = 1
NSIGPROF = 1
SI_MAXSZ = 128
def SI_CANQUEUE(c): return ((c) <= SI_QUEUE)

FD_SETSIZE = 1024
NBBY = 8
O_RDONLY = 0
O_WRONLY = 1
O_RDWR = 2
O_NDELAY = 0x04
O_APPEND = 0x08
O_SYNC = 0x10
O_DSYNC = 0x40
O_RSYNC = 0x8000
O_NONBLOCK = 0x80
O_PRIV = 0x1000
O_CREAT = 0x100
O_TRUNC = 0x200
O_EXCL = 0x400
O_NOCTTY = 0x800
F_DUPFD = 0
F_GETFD = 1
F_SETFD = 2
F_GETFL = 3
F_SETFL = 4
F_SETLK = 6
F_SETLKW = 7
F_O_GETLK = 5
F_SETLK = 6
F_SETLKW = 7
F_CHKFL = 8
F_ALLOCSP = 10
F_FREESP = 11
F_ISSTREAM = 13
F_GETLK = 14
F_PRIV = 15
F_NPRIV = 16
F_QUOTACTL = 17
F_BLOCKS = 18
F_BLKSIZE = 19
F_RSETLK = 20
F_RGETLK = 21
F_RSETLKW = 22
F_GETOWN = 23
F_SETOWN = 24
F_REVOKE = 25
F_HASREMOTELOCKS = 26
F_RDLCK = 01
F_WRLCK = 02
F_UNLCK = 03
F_UNLKSYS = 04
O_ACCMODE = 3
FD_CLOEXEC = 1
