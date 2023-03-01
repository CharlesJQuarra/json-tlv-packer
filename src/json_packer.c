
#include <apr.h>
#include <apr_hash.h>


int main(int                argc,
         const char* const *argv)
{
  apr_status_t rv;
  apr_pool_t  *p = NULL;

  apr_app_initialize(&argc, &argv, NULL);
  atexit(apr_terminate);

  apr_pool_create(&p, NULL);

  apr_terminate();
  return rv;
};
