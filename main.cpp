#include <stdio.h>
#include <stdlib.h>
#include <pzcl/pzcl.h>

#define MAX_DEVICE_IDS 8

int main ( int argc, char *argv [ ] )
{
  pzcl_platform_id platform;
  int stats = pzclGetPlatformIDs ( 1, &platform, NULL );
  if ( stats != 0 ) { fprintf ( stderr, "pzclGetPlatformIDs: %d\n", stats ); }
 
  pzcl_device_id device [ MAX_DEVICE_IDS ];
  pzcl_uint num_devices;
  stats = pzclGetDeviceIDs ( platform, PZCL_DEVICE_TYPE_DEFAULT, MAX_DEVICE_IDS, device, &num_devices );
  if ( stats != 0 ) { fprintf ( stderr, "pzclGetDeviceIDs: %d\n", stats ); }

  for ( int i = 0; i < num_devices; i++ ) {
    char name_data [ 128 ];
    pzcl_int err = pzclGetDeviceInfo ( device [ i ], PZCL_DEVICE_NAME, sizeof ( name_data ), name_data, NULL );
    printf ( "%s%s", name_data, (i == num_devices - 1) ? "\n" : " "  );
  }

  return 0;
}
