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
};

int     h_nerr = sizeof(h_errlist)/sizeof(h_errlist[0]);

char *
hstrerror(int err)
{
	return (int)err < h_nerr ? h_errlist[err] : "Unknown resolver error";
}
