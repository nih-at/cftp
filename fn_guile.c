#include "config.h"

#ifdef HAVE_GUILE

#include <guile/gh.h>

#include "display.h"

char *scm2cstring(SCM object, int *length_p);



void fn_guile(char **args)
{
    SCM ret;
    char *cmd;
    int len;

    if (args)
	cmd = args[0];
    else
	cmd = read_string("<guile>", 1);

    ret = gh_eval_str(cmd);

    if (!args)
	free(cmd);

    cmd = scm2cstring(ret, &len);
    disp_status("%s", cmd);
    free(cmd);
}



char *
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



#if 0
static SCM wrapper(void *data, SCM jmpbuf)
{
  char *scheme_code = (char *)data;
  SCM res = gh_eval_str(scheme_code);
  return res;
}

static SCM catcher(void *data, SCM tag, SCM throw_args)
{
	char b[256];

	strcpy(b, "ERROR: ");
	strncat(b, (char *)data, 200);
	return SCM_BOOL_F;
}

void execute_guile(char *s)
{
	gh_catch(SCM_BOOL_T,
			(scm_catch_body_t)wrapper, s,
			(scm_catch_handler_t)catcher, s);
}
#endif

#endif
