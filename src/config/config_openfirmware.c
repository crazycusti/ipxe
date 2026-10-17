FILE_LICENCE ( GPL2_OR_LATER );
#ifdef PLATFORM_openfirmware
PROVIDE_REQUIRING_SYMBOL ();
REQUIRE_OBJECT ( platform );
#if defined ( __powerpc__ ) || defined ( __powerpc64__ )
REQUIRE_OBJECT ( of_linux );
#elif defined ( __sparc__ ) && defined ( __arch64__ )
REQUIRE_OBJECT ( sparc_linux );
#elif defined ( __sparc__ )
REQUIRE_OBJECT ( sparc32_linux );
#endif
#endif
