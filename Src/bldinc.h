#define MAJOR_VERSION $!{FWMAJOR:0}
#define MINOR_VERSION $!{FWMINOR:0}
#define SUITE_REVISION $!{FWREVISION:0}
//#define DN_MAJOR_VERSION $!{DNMAJOR:0}
//#define DN_MINOR_VERSION $!{DNMINOR:0}
//#define DN_REVISION $!{DNREVISION:0}
//#define WP_MAJOR_VERSION $!{WPMAJOR:0}
//#define WP_MINOR_VERSION $!{WPMINOR:0}
//#define WP_REVISION $!{WPREVISION:0}
//#define TLE_MAJOR_VERSION $!{TLEMAJOR:0}
//#define TLE_MINOR_VERSION $!{TLEMINOR:0}
//#define TLE_REVISION $!{TLEREVISION:0}
#define YEAR $YEAR
//#define DAY_MONTH_BUILDLVL $MONTH$DAY$!{BUILD_LEVEL:9}
#define NUMBER_OF_DAYS $NUMBEROFDAYS
#define STR_PRODUCT "$!{FWMAJOR:0}.$!{FWMINOR:0}.$!{FWREVISION:0}.$NUMBEROFDAYS\0"
#define FWSUITE_VERSION "$!{FWMAJOR:0}.$!{FWMINOR:0}.$!{FWREVISION:0}.0\0"
#define COPYRIGHT "Copyright � 2002-$YEAR, SIL International\0"
#define COPYRIGHTRESERVED "Copyright � 2002-$YEAR, SIL International.  All rights reserved."
