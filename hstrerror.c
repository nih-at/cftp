char    *h_errlist[] = {
	"Error 0",
	"Unknown host",                         /* 1
						   HOST_NOT_FOUND */
	"Host name lookup failure",             /* 2
						   TRY_AGAIN */
	"Unknown server error",                 /* 3
						   NO_RECOVERY */
	"No address associated with name",      /* 4
						   NO_ADDRESS */
	"Service unavailable",			/* 5
						   AIX: SERVICE_UNAVAILABLE */
};

int     h_nerr = sizeof(h_errlist)/sizeof(h_errlist[0]);

char *
hstrerror(int err)
{
    static char b[40];

    if (err < 0 || err > h_nerr) {
	sprintf(b, "Unknown resolver error %d", err);
	return b;
    }

    return h_errlist[err];
}
