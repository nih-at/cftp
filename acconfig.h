/* Define if netdb.h declares h_errno */
#undef H_ERRNO_DECLARED

/* Define if guile support should be compiled */
#undef HAVE_GUILE

@BOTTOM@
/* Define if we have any scripting language */
#if defined(HAVE_GUILE)
#define HAVE_SCRIPTING
#endif
