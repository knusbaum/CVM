
void run_testcode(char *code);

#ifdef HAVE_TCASE_NAME
 #define PRINT_TEST_NAME fprintf(stderr, "TEST %s\n", tcase_name());
#else
 #define PRINT_TEST_NAME
#endif
