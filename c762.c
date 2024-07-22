g_fork(void)
{
#if defined(_WIN32)
  return 0;
#else
  int rv;

  rv = fork();
  if (rv == 0) /* child */
  {
    g_strcpy_s(g_temp_base, 127, g_temp_base_org);
    if (mkdtemp(g_temp_base) == 0)
    {
      printf("g_fork: mkdtemp failed [%s]\n", g_temp_base);
    }
  }
  return rv;
#endif
}