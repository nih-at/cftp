#include "config.h"

#ifdef HAVE_GUILE

#include <guile/gh.h>

#include "display.h"

static char *scm2cstring(SCM object, int *length_p);
static SCM execute_guile(char *s);

static char guile_errstr[8192];
static int guile_error;



void fn_guile(char **args)
{
    SCM ret;
    char *cmd;
    int len;

    if (args)
	cmd = args[0];
    else
	cmd = read_string("<guile>", 1);

    guile_error = 0;
    ret = execute_guile(cmd);
    
    if (!args)
	free(cmd);

    if (guile_error)
	disp_status("%s", guile_errstr);
    else {
	cmd = scm2cstring(ret, &len);
	disp_status("%s", cmd);
	free(cmd);
    }
}



static char *
scm2cstring(SCM object, int *length_p)
{
    SCM stringify;
    char * string;

    stringify = gh_eval_str("(lambda(object)"
			    " (with-output-to-string"
			    "  (lambda() (write object))))");
    string = gh_scm2newstr(gh_call1(stringify, object), length_p);
    return string;
}



static SCM wrapper(void *data, SCM jmpbuf)
{
    char *scheme_code = (char *)data;
    SCM res = gh_eval_str(scheme_code);
    return res;
}

static SCM catcher(void *data, SCM tag, SCM throw_args)
{
    char *s;
    int len;
    
    guile_error = 1;
    strcpy(guile_errstr, "ERROR: ");
    /* strncat(guile_errstr, (char *)data, 8180);*/
    s = scm2cstring(throw_args, &len);
    strncat(guile_errstr, s, 8180);
    free(s);
    
    return SCM_BOOL_F;
}

static SCM execute_guile(char *s)
{
    return gh_catch(SCM_BOOL_T,
		    (scm_catch_body_t)wrapper, s,
		    (scm_catch_handler_t)catcher, s);
}

#endif
